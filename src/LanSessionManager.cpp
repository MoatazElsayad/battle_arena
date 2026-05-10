#include "LanSessionManager.h"

#include "LanProtocol.h"

#include <algorithm>
#include <QDateTime>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

namespace {

constexpr qint64 kLanInputHeartbeatMs = 50;
constexpr qint64 kLanStatePublishIntervalMs = 20;

int lanAddressPriority(const QString& address) {
    if (address.startsWith(QStringLiteral("192.168."))) {
        return 0;
    }

    if (address.startsWith(QStringLiteral("10."))) {
        return 1;
    }

    if (address.startsWith(QStringLiteral("172."))) {
        const QStringList parts = address.split('.');
        if (parts.size() >= 2) {
            bool ok = false;
            const int secondOctet = parts.at(1).toInt(&ok);
            if (ok && secondOctet >= 16 && secondOctet <= 31) {
                return 2;
            }
        }
    }

    return 3;
}

bool combatStateNeedsImmediatePublish(const LanCombatState& previous, const LanCombatState& next) {
    return previous.matchFinished != next.matchFinished
        || previous.winner != next.winner
        || previous.hostHp != next.hostHp
        || previous.guestHp != next.guestHp
        || previous.hostProjectileActive != next.hostProjectileActive
        || previous.guestProjectileActive != next.guestProjectileActive
        || previous.hostAnimation != next.hostAnimation
        || previous.guestAnimation != next.guestAnimation
        || previous.statusMessage != next.statusMessage;
}

} // namespace

LanSessionManager::LanSessionManager(QObject* parent)
    : QObject(parent),
      server_(new QTcpServer(this)),
      socket_(nullptr),
      pingTimer_(new QTimer(this)),
      pendingPingToken_(0),
      pendingPingStartedMs_(0),
      outgoingCombatSequence_(0),
      lastCombatInputSentMs_(0),
      lastCombatInputBits_(0),
      lastCombatStateSentMs_(0),
      combatBridgeActive_(false),
      latestRemoteCombatInputReceivedMs_(0) {
    qRegisterMetaType<LanSessionSnapshot>("LanSessionSnapshot");
    qRegisterMetaType<LanCombatInputFrame>("LanCombatInputFrame");
    qRegisterMetaType<LanCombatState>("LanCombatState");

    connect(server_, &QTcpServer::newConnection, this, &LanSessionManager::handleNewConnection);

    pingTimer_->setInterval(2500);
    connect(pingTimer_, &QTimer::timeout, this, &LanSessionManager::sendPing);

    snapshot_.hostAddress = bestLocalAddress();
}

LanSessionManager::~LanSessionManager() {
    releaseSocket(true);
}

void LanSessionManager::setLocalProfile(const QString& username, const QString& fighterName, int fighterType) {
    snapshot_.localPlayer.username = username.trimmed();
    snapshot_.localPlayer.fighterName = fighterName.trimmed();
    snapshot_.localPlayer.fighterType = fighterType;
    updateDerivedState();
    emitSnapshot();

    if (hasActiveSocket()) {
        sendHello();
        sendCharacterSelection();
        sendReadyState();
    }
}

void LanSessionManager::setArenaName(const QString& arenaName) {
    const QString trimmed = arenaName.trimmed();
    if (!trimmed.isEmpty()) {
        snapshot_.arenaName = trimmed;
        updateStatusLine();
        emitSnapshot();

        if (hasActiveSocket() && snapshot_.localRole == LanRole::HOST) {
            sendArenaSelection();
        }
    }
}

bool LanSessionManager::startHosting(const QString& username, const QString& fighterName, int fighterType, quint16 port) {
    resetSession(LanRole::HOST, LanSessionState::HOSTING);
    snapshot_.port = port;
    snapshot_.hostAddress = bestLocalAddress();
    snapshot_.localPlayer.username = username.trimmed();
    snapshot_.localPlayer.fighterName = fighterName.trimmed();
    snapshot_.localPlayer.fighterType = fighterType;

    if (!server_->listen(QHostAddress::AnyIPv4, port)) {
        raiseError(QString("Could not host on port %1: %2").arg(port).arg(server_->errorString()));
        return false;
    }

    logEvent(QString("Hosting Arena Link on %1:%2").arg(snapshot_.hostAddress).arg(port));
    updateDerivedState();
    emitSnapshot();
    return true;
}

bool LanSessionManager::joinSession(const QString& hostAddress, const QString& username, const QString& fighterName, int fighterType, quint16 port) {
    const QString trimmedHost = hostAddress.trimmed();
    if (trimmedHost.isEmpty()) {
        raiseError(QStringLiteral("Enter the host IP before joining."));
        return false;
    }

    resetSession(LanRole::GUEST, LanSessionState::CONNECTING);
    snapshot_.hostAddress = trimmedHost;
    snapshot_.port = port;
    snapshot_.localPlayer.username = username.trimmed();
    snapshot_.localPlayer.fighterName = fighterName.trimmed();
    snapshot_.localPlayer.fighterType = fighterType;

    auto* socket = new QTcpSocket(this);
    attachSocket(socket);
    socket_->connectToHost(trimmedHost, port);

    logEvent(QString("Connecting to %1:%2").arg(trimmedHost).arg(port));
    updateDerivedState();
    emitSnapshot();
    return true;
}

void LanSessionManager::setLocalReady(bool ready) {
    snapshot_.localPlayer.ready = ready;
    updateDerivedState();
    emitSnapshot();

    if (hasActiveSocket()) {
        sendReadyState();
    }
}

void LanSessionManager::primeMatch() {
    if (snapshot_.localRole != LanRole::HOST) {
        raiseError(QStringLiteral("Only the host can prime the duel handoff."));
        return;
    }

    if (!snapshot_.canStartMatch) {
        raiseError(QStringLiteral("Both gladiators must be connected and ready before priming the duel."));
        return;
    }

    snapshot_.state = LanSessionState::MATCH_PRIMED;
    updateStatusLine();
    emitSnapshot();
    sendStartMatch();
    logEvent(QStringLiteral("Match handoff primed. Realtime battle bridge can attach next."));
    emit matchPrimed(snapshot_);
}

void LanSessionManager::disconnectSession() {
    if (hasActiveSocket()) {
        sendMessage(LanPacketType::DISCONNECT, QJsonObject{});
    }

    resetSession();
    logEvent(QStringLiteral("Arena Link session closed."));
    emitSnapshot();
}

void LanSessionManager::beginCombatBridge() {
    combatBridgeActive_ = true;
    latestRemoteCombatInput_ = LanCombatInputFrame();
    latestCombatState_ = LanCombatState();
    latestRemoteCombatInputReceivedMs_ = 0;
    outgoingCombatSequence_ = 0;
    lastCombatInputSentMs_ = 0;
    lastCombatInputBits_ = 0;
    lastCombatStateSentMs_ = 0;
    lastPublishedCombatState_ = LanCombatState();

    if (snapshot_.state == LanSessionState::MATCH_PRIMED) {
        snapshot_.statusLine = QStringLiteral("Arena Link realtime duel bridge is live.");
        emitSnapshot();
    }
}

void LanSessionManager::endCombatBridge() {
    combatBridgeActive_ = false;
    latestRemoteCombatInput_ = LanCombatInputFrame();
    latestCombatState_ = LanCombatState();
    latestRemoteCombatInputReceivedMs_ = 0;
    lastCombatInputSentMs_ = 0;
    lastCombatInputBits_ = 0;
    lastCombatStateSentMs_ = 0;
    lastPublishedCombatState_ = LanCombatState();

    if (snapshot_.state == LanSessionState::MATCH_PRIMED) {
        updateStatusLine();
        emitSnapshot();
    }
}

void LanSessionManager::sendCombatInput(quint8 inputBits) {
    if (!combatBridgeActive_ || snapshot_.localRole != LanRole::GUEST || !hasActiveSocket()) {
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const bool changed = inputBits != lastCombatInputBits_;
    const bool heartbeatDue = lastCombatInputSentMs_ <= 0
        || (nowMs - lastCombatInputSentMs_) >= kLanInputHeartbeatMs;
    if (!changed && !heartbeatDue) {
        return;
    }

    LanCombatInputFrame frame;
    frame.sequence = ++outgoingCombatSequence_;
    frame.inputBits = inputBits;
    sendMessage(LanPacketType::COMBAT_INPUT, lanCombatInputToJson(frame));
    lastCombatInputBits_ = inputBits;
    lastCombatInputSentMs_ = nowMs;
}

void LanSessionManager::publishCombatState(const LanCombatState& state) {
    if (!combatBridgeActive_ || snapshot_.localRole != LanRole::HOST || !hasActiveSocket()) {
        return;
    }

    latestCombatState_ = state;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const bool firstPublish = lastCombatStateSentMs_ <= 0;
    const bool intervalDue = firstPublish || (nowMs - lastCombatStateSentMs_) >= kLanStatePublishIntervalMs;
    const bool immediatePublish = firstPublish
        || state.matchFinished
        || combatStateNeedsImmediatePublish(lastPublishedCombatState_, state);
    if (!intervalDue && !immediatePublish) {
        return;
    }

    sendMessage(LanPacketType::COMBAT_STATE, lanCombatStateToJson(state));
    lastPublishedCombatState_ = state;
    lastCombatStateSentMs_ = nowMs;
}

QStringList LanSessionManager::localAddressHints() const {
    QStringList addresses;
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp) || !(iface.flags() & QNetworkInterface::IsRunning)) {
            continue;
        }
        if (iface.flags() & QNetworkInterface::IsLoopBack) {
            continue;
        }

        const QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                addresses.append(entry.ip().toString());
            }
        }
    }

    addresses.removeDuplicates();
    std::sort(addresses.begin(), addresses.end(), [](const QString& left, const QString& right) {
        const int leftPriority = lanAddressPriority(left);
        const int rightPriority = lanAddressPriority(right);
        if (leftPriority != rightPriority) {
            return leftPriority < rightPriority;
        }
        return left < right;
    });

    if (addresses.isEmpty()) {
        addresses.append(QStringLiteral("127.0.0.1"));
    }
    return addresses;
}

void LanSessionManager::handleNewConnection() {
    if (!server_->hasPendingConnections()) {
        return;
    }

    QTcpSocket* incoming = server_->nextPendingConnection();
    if (!incoming) {
        return;
    }

    if (hasActiveSocket()) {
        incoming->disconnectFromHost();
        incoming->deleteLater();
        logEvent(QStringLiteral("Ignored an extra LAN connection because the room already has a guest."));
        return;
    }

    attachSocket(incoming);
    snapshot_.remoteConnected = true;
    logEvent(QStringLiteral("A challenger joined the LAN room."));
    updateDerivedState();
    emitSnapshot();

    sendHello();
    sendCharacterSelection();
    sendArenaSelection();
    sendReadyState();
    pingTimer_->start();
}

void LanSessionManager::handleSocketConnected() {
    snapshot_.remoteConnected = true;
    logEvent(QString("Linked to host %1:%2").arg(snapshot_.hostAddress).arg(snapshot_.port));
    updateDerivedState();
    emitSnapshot();

    sendHello();
    sendCharacterSelection();
    sendArenaSelection();
    sendReadyState();
    pingTimer_->start();
}

void LanSessionManager::handleSocketReadyRead() {
    if (!socket_) {
        return;
    }

    readBuffer_.append(socket_->readAll());
    int newlineIndex = readBuffer_.indexOf('\n');
    while (newlineIndex >= 0) {
        const QByteArray line = readBuffer_.left(newlineIndex).trimmed();
        readBuffer_.remove(0, newlineIndex + 1);
        if (!line.isEmpty()) {
            processIncomingLine(line);
        }
        newlineIndex = readBuffer_.indexOf('\n');
    }
}

void LanSessionManager::handleSocketDisconnected() {
    readBuffer_.clear();
    pingTimer_->stop();

    if (snapshot_.localRole == LanRole::HOST && server_->isListening()) {
        releaseSocket(false);
        snapshot_.remoteConnected = false;
        snapshot_.remotePlayer = LanPlayerInfo();
        snapshot_.canStartMatch = false;
        snapshot_.lastPingMs = -1;
        snapshot_.state = LanSessionState::HOSTING;
        updateStatusLine();
        logEvent(QStringLiteral("Guest disconnected. Hosting stays open for the next challenger."));
        emitSnapshot();
        return;
    }

    resetSession();
    logEvent(QStringLiteral("LAN link ended."));
    emitSnapshot();
}

void LanSessionManager::handleSocketError(QAbstractSocket::SocketError socketError) {
    if (socketError == QAbstractSocket::RemoteHostClosedError) {
        return;
    }
    if (!socket_) {
        return;
    }

    raiseError(QString("LAN socket error: %1").arg(socket_->errorString()));
}

void LanSessionManager::sendPing() {
    if (!hasActiveSocket()) {
        return;
    }

    pendingPingToken_ = QDateTime::currentMSecsSinceEpoch();
    pendingPingStartedMs_ = pendingPingToken_;
    sendMessage(LanPacketType::PING, QJsonObject{
        {QStringLiteral("token"), QString::number(pendingPingToken_)}
    });
}

void LanSessionManager::attachSocket(QTcpSocket* socket) {
    releaseSocket(false);
    socket_ = socket;
    if (!socket_) {
        return;
    }

    socket_->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    socket_->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    socket_->setReadBufferSize(64 * 1024);

    connect(socket_, &QTcpSocket::connected, this, &LanSessionManager::handleSocketConnected);
    connect(socket_, &QTcpSocket::readyRead, this, &LanSessionManager::handleSocketReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &LanSessionManager::handleSocketDisconnected);
    connect(socket_, &QTcpSocket::errorOccurred, this, &LanSessionManager::handleSocketError);
}

void LanSessionManager::releaseSocket(bool abortConnection) {
    if (!socket_) {
        return;
    }

    socket_->disconnect(this);
    if (abortConnection) {
        socket_->abort();
    }
    socket_->deleteLater();
    socket_ = nullptr;
}

void LanSessionManager::resetSession(LanRole role, LanSessionState state) {
    const QString arenaName = snapshot_.arenaName.isEmpty() ? QStringLiteral("Molten Gate") : snapshot_.arenaName;
    LanPlayerInfo localPlayer = snapshot_.localPlayer;
    localPlayer.ready = false;

    pingTimer_->stop();
    readBuffer_.clear();
    pendingPingToken_ = 0;
    pendingPingStartedMs_ = 0;
    outgoingCombatSequence_ = 0;
    lastCombatInputSentMs_ = 0;
    lastCombatInputBits_ = 0;
    lastCombatStateSentMs_ = 0;
    combatBridgeActive_ = false;
    latestRemoteCombatInput_ = LanCombatInputFrame();
    latestCombatState_ = LanCombatState();
    latestRemoteCombatInputReceivedMs_ = 0;
    lastPublishedCombatState_ = LanCombatState();

    if (server_->isListening()) {
        server_->close();
    }
    releaseSocket(true);

    snapshot_ = LanSessionSnapshot();
    snapshot_.arenaName = arenaName;
    snapshot_.localRole = role;
    snapshot_.state = state;
    snapshot_.hostAddress = bestLocalAddress();
    snapshot_.localPlayer = localPlayer;
    updateStatusLine();
}

void LanSessionManager::emitSnapshot() {
    emit snapshotChanged(snapshot_);
}

void LanSessionManager::logEvent(const QString& message) {
    emit eventLogged(message);
}

void LanSessionManager::raiseError(const QString& message) {
    snapshot_.state = LanSessionState::ERROR;
    snapshot_.statusLine = message;
    emit errorRaised(message);
    emitSnapshot();
}

void LanSessionManager::sendHello() {
    sendMessage(LanPacketType::HELLO, QJsonObject{
        {QStringLiteral("player"), lanPlayerToJson(snapshot_.localPlayer)},
        {QStringLiteral("arenaName"), snapshot_.arenaName},
        {QStringLiteral("role"), lanRoleDisplayName(snapshot_.localRole)}
    });
}

void LanSessionManager::sendCharacterSelection() {
    sendMessage(LanPacketType::CHARACTER_SELECT, QJsonObject{
        {QStringLiteral("fighterName"), snapshot_.localPlayer.fighterName},
        {QStringLiteral("fighterType"), snapshot_.localPlayer.fighterType}
    });
}

void LanSessionManager::sendArenaSelection() {
    if (snapshot_.localRole != LanRole::HOST) {
        return;
    }

    sendMessage(LanPacketType::ARENA_SELECT, QJsonObject{
        {QStringLiteral("arenaName"), snapshot_.arenaName}
    });
}

void LanSessionManager::sendReadyState() {
    sendMessage(LanPacketType::READY_STATE, QJsonObject{
        {QStringLiteral("ready"), snapshot_.localPlayer.ready}
    });
}

void LanSessionManager::sendStartMatch() {
    sendMessage(LanPacketType::START_MATCH, QJsonObject{
        {QStringLiteral("arenaName"), snapshot_.arenaName},
        {QStringLiteral("startedAt"), QString::number(QDateTime::currentMSecsSinceEpoch())}
    });
}

void LanSessionManager::sendMessage(LanPacketType type, const QJsonObject& payload) {
    if (!hasActiveSocket()) {
        return;
    }

    socket_->write(encodeLanMessage(type, payload));
    socket_->flush();
}

void LanSessionManager::processIncomingLine(const QByteArray& line) {
    LanPacketType type = LanPacketType::HELLO;
    QJsonObject payload;
    QString errorMessage;
    if (!decodeLanMessage(line, &type, &payload, &errorMessage)) {
        logEvent(QString("Ignored malformed LAN packet: %1").arg(errorMessage));
        return;
    }

    switch (type) {
        case LanPacketType::HELLO: {
            snapshot_.remotePlayer = lanPlayerFromJson(payload.value(QStringLiteral("player")).toObject());
            const QString arenaName = payload.value(QStringLiteral("arenaName")).toString();
            const QString remoteRole = payload.value(QStringLiteral("role")).toString().trimmed();
            const bool shouldApplyRemoteArena = snapshot_.localRole == LanRole::GUEST
                || remoteRole.compare(QStringLiteral("Host"), Qt::CaseInsensitive) == 0;
            if (shouldApplyRemoteArena && !arenaName.trimmed().isEmpty()) {
                snapshot_.arenaName = arenaName.trimmed();
            }
            snapshot_.remoteConnected = true;
            logEvent(QString("Remote gladiator linked: %1").arg(snapshot_.remotePlayer.username.isEmpty()
                         ? QStringLiteral("Unknown")
                         : snapshot_.remotePlayer.username));
            break;
        }
        case LanPacketType::CHARACTER_SELECT:
            snapshot_.remotePlayer.fighterName = payload.value(QStringLiteral("fighterName")).toString();
            snapshot_.remotePlayer.fighterType = payload.value(QStringLiteral("fighterType")).toInt(-1);
            logEvent(QString("Remote fighter locked: %1").arg(snapshot_.remotePlayer.fighterName.isEmpty()
                         ? QStringLiteral("Unknown")
                         : snapshot_.remotePlayer.fighterName));
            break;
        case LanPacketType::ARENA_SELECT: {
            const QString arenaName = payload.value(QStringLiteral("arenaName")).toString().trimmed();
            if (!arenaName.isEmpty()) {
                snapshot_.arenaName = arenaName;
                logEvent(QString("Battleground changed to %1.").arg(snapshot_.arenaName));
            }
            break;
        }
        case LanPacketType::READY_STATE:
            snapshot_.remotePlayer.ready = payload.value(QStringLiteral("ready")).toBool(false);
            logEvent(QString("Remote ready state changed to %1.")
                         .arg(snapshot_.remotePlayer.ready ? QStringLiteral("READY") : QStringLiteral("WAITING")));
            break;
        case LanPacketType::START_MATCH:
            snapshot_.state = LanSessionState::MATCH_PRIMED;
            updateStatusLine();
            logEvent(QStringLiteral("Host primed the duel handoff."));
            emitSnapshot();
            emit matchPrimed(snapshot_);
            return;
        case LanPacketType::COMBAT_INPUT:
            latestRemoteCombatInput_ = lanCombatInputFromJson(payload);
            latestRemoteCombatInputReceivedMs_ = QDateTime::currentMSecsSinceEpoch();
            return;
        case LanPacketType::COMBAT_STATE:
            latestCombatState_ = lanCombatStateFromJson(payload);
            return;
        case LanPacketType::PING: {
            const QString token = payload.value(QStringLiteral("token")).toString();
            sendMessage(LanPacketType::PONG, QJsonObject{{QStringLiteral("token"), token}});
            return;
        }
        case LanPacketType::PONG: {
            const qint64 token = payload.value(QStringLiteral("token")).toString().toLongLong();
            if (token == pendingPingToken_ && pendingPingStartedMs_ > 0) {
                snapshot_.lastPingMs = static_cast<int>(QDateTime::currentMSecsSinceEpoch() - pendingPingStartedMs_);
            }
            emitSnapshot();
            return;
        }
        case LanPacketType::DISCONNECT:
            logEvent(QStringLiteral("Remote gladiator requested disconnect."));
            return;
    }

    updateDerivedState();
    emitSnapshot();
}

void LanSessionManager::updateDerivedState() {
    snapshot_.canStartMatch = snapshot_.remoteConnected && bothPlayersReady() && bothPlayersNamed();

    if (snapshot_.state == LanSessionState::ERROR || snapshot_.state == LanSessionState::MATCH_PRIMED) {
        updateStatusLine();
        return;
    }

    if (snapshot_.localRole == LanRole::HOST && server_->isListening() && !snapshot_.remoteConnected) {
        snapshot_.state = LanSessionState::HOSTING;
    } else if (snapshot_.localRole == LanRole::GUEST && socket_ && socket_->state() == QAbstractSocket::ConnectingState) {
        snapshot_.state = LanSessionState::CONNECTING;
    } else if (snapshot_.remoteConnected && snapshot_.canStartMatch) {
        snapshot_.state = LanSessionState::READY_CHECK;
    } else if (snapshot_.remoteConnected) {
        snapshot_.state = LanSessionState::LINKED;
    } else if (snapshot_.localRole == LanRole::NONE) {
        snapshot_.state = LanSessionState::IDLE;
    }

    updateStatusLine();
}

void LanSessionManager::updateStatusLine() {
    switch (snapshot_.state) {
        case LanSessionState::HOSTING:
            snapshot_.statusLine = QString("Room is open on %1:%2. Waiting for another gladiator to join.")
                                       .arg(snapshot_.hostAddress)
                                       .arg(snapshot_.port);
            return;
        case LanSessionState::CONNECTING:
            snapshot_.statusLine = QString("Joining %1:%2...")
                                       .arg(snapshot_.hostAddress)
                                       .arg(snapshot_.port);
            return;
        case LanSessionState::LINKED:
            snapshot_.statusLine = QStringLiteral("The room is linked. Choose fighters, then get both sides ready.");
            return;
        case LanSessionState::READY_CHECK:
            snapshot_.statusLine = snapshot_.localRole == LanRole::HOST
                ? QStringLiteral("Both gladiators are ready. Start the duel whenever you want.")
                : QStringLiteral("Both gladiators are ready. Waiting for the host to start the duel.");
            return;
        case LanSessionState::MATCH_PRIMED:
            snapshot_.statusLine = combatBridgeActive_
                ? QStringLiteral("The duel is live.")
                : QStringLiteral("The duel is starting now.");
            return;
        case LanSessionState::ERROR:
            if (snapshot_.statusLine.trimmed().isEmpty()) {
                snapshot_.statusLine = QStringLiteral("LAN session error.");
            }
            return;
        case LanSessionState::IDLE:
        default:
            snapshot_.statusLine = QStringLiteral("Open a room or join one to prepare a nearby duel.");
            return;
    }
}

bool LanSessionManager::hasActiveSocket() const {
    return socket_ && socket_->state() == QAbstractSocket::ConnectedState;
}

bool LanSessionManager::bothPlayersReady() const {
    return snapshot_.localPlayer.ready && snapshot_.remotePlayer.ready;
}

bool LanSessionManager::bothPlayersNamed() const {
    return !snapshot_.localPlayer.username.trimmed().isEmpty()
        && !snapshot_.localPlayer.fighterName.trimmed().isEmpty()
        && !snapshot_.remotePlayer.username.trimmed().isEmpty()
        && !snapshot_.remotePlayer.fighterName.trimmed().isEmpty();
}

QString LanSessionManager::bestLocalAddress() const {
    const QStringList hints = localAddressHints();
    return hints.isEmpty() ? QStringLiteral("127.0.0.1") : hints.first();
}
