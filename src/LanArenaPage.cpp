#include "LanArenaPage.h"

#include "LanSessionManager.h"

#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString sectionFrameStyle() {
    return QStringLiteral(
        "QFrame {"
        " background: rgba(18,14,12,0.82);"
        " border: 1px solid rgba(212,160,23,0.20);"
        " border-radius: 20px;"
        "}"
    );
}

QString secondaryButtonStyle() {
    return QStringLiteral(
        "QPushButton {"
        " background: rgba(58,42,27,0.92);"
        " border: 1px solid rgba(212,160,23,0.30);"
        " border-radius: 14px;"
        " color: #F0DEB2;"
        " font: 700 14px 'Segoe UI';"
        " padding: 11px 18px;"
        "}"
        "QPushButton:hover { background: rgba(82,58,35,0.95); }"
        "QPushButton:disabled { color: rgba(240,222,178,0.42); background: rgba(58,42,27,0.38); }"
    );
}

QString primaryButtonStyle() {
    return QStringLiteral(
        "QPushButton {"
        " background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #6E0D0D, stop:0.45 #A81616, stop:1 #D84321);"
        " border: 2px solid #D86A55;"
        " border-radius: 16px;"
        " color: #FFE6E6;"
        " font: 800 16px 'Segoe UI';"
        " padding: 13px 22px;"
        "}"
        "QPushButton:hover { border-color: #FFB299; }"
        "QPushButton:disabled { color: rgba(255,230,230,0.44); background: rgba(110,13,13,0.34); border-color: rgba(216,106,85,0.35); }"
    );
}

} // namespace

LanArenaPage::LanArenaPage(QWidget* parent)
    : QWidget(parent),
      sessionManager_(nullptr),
      fighterType_(PlayerType::KNIGHT),
      identityLabel_(nullptr),
      addressHintLabel_(nullptr),
      statusBadgeLabel_(nullptr),
      statusTextLabel_(nullptr),
      sessionMetaLabel_(nullptr),
      localPlayerLabel_(nullptr),
      remotePlayerLabel_(nullptr),
      duelHintLabel_(nullptr),
      hostAddressEdit_(nullptr),
      portEdit_(nullptr),
      hostButton_(nullptr),
      joinButton_(nullptr),
      disconnectButton_(nullptr),
      readyButton_(nullptr),
      primeButton_(nullptr),
      eventLog_(nullptr) {
    setupUi();
}

void LanArenaPage::setSessionManager(LanSessionManager* manager) {
    if (sessionManager_ == manager) {
        return;
    }

    if (sessionManager_) {
        QObject::disconnect(sessionManager_, nullptr, this, nullptr);
    }

    sessionManager_ = manager;
    refreshAddressHints();

    if (!sessionManager_) {
        return;
    }

    connect(sessionManager_, &LanSessionManager::snapshotChanged, this, &LanArenaPage::applySnapshot);
    connect(sessionManager_, &LanSessionManager::eventLogged, this, &LanArenaPage::appendLog);
    connect(sessionManager_, &LanSessionManager::errorRaised, this, &LanArenaPage::appendLog);

    sessionManager_->setLocalProfile(username_, fighterName_, static_cast<int>(fighterType_));
    applySnapshot(sessionManager_->snapshot());
}

void LanArenaPage::setIdentity(const QString& username, const QString& fighterName, PlayerType fighterType) {
    username_ = username.trimmed();
    fighterName_ = fighterName.trimmed();
    fighterType_ = fighterType;

    refreshIdentityLabels();
    if (sessionManager_) {
        sessionManager_->setLocalProfile(username_, fighterName_, static_cast<int>(fighterType_));
    }
}

void LanArenaPage::hostSession() {
    if (!sessionManager_) {
        return;
    }

    sessionManager_->startHosting(username_, fighterName_, static_cast<int>(fighterType_), selectedPort());
}

void LanArenaPage::joinSession() {
    if (!sessionManager_) {
        return;
    }

    sessionManager_->joinSession(hostAddressEdit_->text(), username_, fighterName_, static_cast<int>(fighterType_), selectedPort());
}

void LanArenaPage::disconnectSession() {
    if (sessionManager_) {
        sessionManager_->disconnectSession();
    }
}

void LanArenaPage::readyToggled(bool checked) {
    if (sessionManager_) {
        sessionManager_->setLocalReady(checked);
    }
}

void LanArenaPage::primeMatch() {
    if (sessionManager_) {
        sessionManager_->primeMatch();
    }
}

void LanArenaPage::applySnapshot(const LanSessionSnapshot& snapshot) {
    statusBadgeLabel_->setText(lanSessionStateDisplayName(snapshot.state).toUpper());
    statusBadgeLabel_->setStyleSheet(badgeStyleForState(snapshot.state));
    statusTextLabel_->setText(snapshot.statusLine);
    sessionMetaLabel_->setText(
        QString("Role: %1  |  Arena: %2  |  Port: %3  |  Ping: %4")
            .arg(lanRoleDisplayName(snapshot.localRole))
            .arg(snapshot.arenaName)
            .arg(snapshot.port)
            .arg(snapshot.lastPingMs >= 0 ? QString("%1 ms").arg(snapshot.lastPingMs) : QStringLiteral("--"))
    );

    localPlayerLabel_->setText(buildPlayerSummary(QStringLiteral("LOCAL GLADIATOR"), snapshot.localPlayer, true));
    remotePlayerLabel_->setText(buildPlayerSummary(QStringLiteral("REMOTE GLADIATOR"), snapshot.remotePlayer, snapshot.remoteConnected));

    const bool busy = snapshot.state == LanSessionState::HOSTING
        || snapshot.state == LanSessionState::CONNECTING
        || snapshot.state == LanSessionState::LINKED
        || snapshot.state == LanSessionState::READY_CHECK
        || snapshot.state == LanSessionState::MATCH_PRIMED;

    hostButton_->setEnabled(!busy || snapshot.localRole == LanRole::NONE || snapshot.state == LanSessionState::ERROR);
    joinButton_->setEnabled(!busy || snapshot.localRole == LanRole::NONE || snapshot.state == LanSessionState::ERROR);
    disconnectButton_->setEnabled(busy || snapshot.state == LanSessionState::ERROR);
    readyButton_->setEnabled(snapshot.remoteConnected);
    readyButton_->setChecked(snapshot.localPlayer.ready);
    primeButton_->setEnabled(snapshot.canStartMatch && snapshot.localRole == LanRole::HOST);

    if (snapshot.canStartMatch && snapshot.localRole == LanRole::HOST) {
        duelHintLabel_->setText(QStringLiteral("Both sides are locked in. Press Prime Match Sync to launch the live duel bridge."));
    } else if (snapshot.canStartMatch) {
        duelHintLabel_->setText(QStringLiteral("Both sides are ready. Waiting for the host to launch the live duel bridge."));
    } else if (snapshot.remoteConnected) {
        duelHintLabel_->setText(QStringLiteral("Connection is live. Sync the fighter and mark ready on both PCs. For one-machine rehearsal, the second copy can join 127.0.0.1."));
    } else {
        duelHintLabel_->setText(QStringLiteral("This page handles host/join, handshake, ready states, ping, and the live duel bridge for LAN play."));
    }
}

void LanArenaPage::appendLog(const QString& message) {
    if (!eventLog_ || message.trimmed().isEmpty()) {
        return;
    }

    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
    eventLog_->appendPlainText(QString("[%1] %2").arg(timestamp, message));
}

void LanArenaPage::setupUi() {
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(
        "QWidget { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #140F0C, stop:0.48 #21160F, stop:1 #0D0B0A); color: #F5E6D3; }"
        "QLabel#headline { color:#FFF0C6; font: 900 28px 'Segoe UI'; }"
        "QLabel#subheadline { color: rgba(245,230,184,0.78); font: 13px 'Segoe UI'; }"
        "QLabel#sectionTitle { color:#FFF0C6; font: 800 18px 'Segoe UI'; }"
        "QLabel#sectionBody { color: rgba(245,230,184,0.72); font: 12px 'Segoe UI'; }"
        "QLineEdit {"
        " background-color: rgba(43,31,22,0.94);"
        " color: #F5E6D3;"
        " border: 1px solid #7C5A24;"
        " border-radius: 12px;"
        " padding: 10px 12px;"
        " font: 13px 'Segoe UI';"
        "}"
        "QLineEdit:focus { border-color: #D4A017; }"
        "QPlainTextEdit {"
        " background: rgba(14,10,8,0.90);"
        " color: #EEDCB3;"
        " border: 1px solid rgba(212,160,23,0.20);"
        " border-radius: 16px;"
        " font: 12px 'Consolas';"
        " padding: 8px;"
        "}"
    );

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(28, 24, 28, 24);
    rootLayout->setSpacing(18);

    auto* headerRow = new QHBoxLayout();
    headerRow->setSpacing(14);

    auto* titleWrap = new QVBoxLayout();
    titleWrap->setSpacing(4);

    auto* title = new QLabel(QStringLiteral("ARENA LINK"), this);
    title->setObjectName("headline");
    titleWrap->addWidget(title);

    auto* subtitle = new QLabel(
        QStringLiteral("Live LAN duel route for two nearby PCs. It now handles hosting, joining, ready sync, ping, realtime handoff, and same-machine rehearsal over localhost."),
        this);
    subtitle->setObjectName("subheadline");
    subtitle->setWordWrap(true);
    titleWrap->addWidget(subtitle);

    headerRow->addLayout(titleWrap, 1);

    auto* backButton = new QPushButton(QStringLiteral("Back to Lobby"), this);
    backButton->setStyleSheet(secondaryButtonStyle());
    connect(backButton, &QPushButton::clicked, this, &LanArenaPage::backRequested);
    headerRow->addWidget(backButton, 0, Qt::AlignTop);
    rootLayout->addLayout(headerRow);

    auto* identityFrame = new QFrame(this);
    identityFrame->setStyleSheet(sectionFrameStyle());
    auto* identityLayout = new QVBoxLayout(identityFrame);
    identityLayout->setContentsMargins(18, 16, 18, 16);
    identityLayout->setSpacing(6);

    identityLabel_ = new QLabel(identityFrame);
    identityLabel_->setStyleSheet("color:#FFF0C6; font:700 16px 'Segoe UI';");
    identityLayout->addWidget(identityLabel_);

    addressHintLabel_ = new QLabel(identityFrame);
    addressHintLabel_->setObjectName("sectionBody");
    addressHintLabel_->setWordWrap(true);
    identityLayout->addWidget(addressHintLabel_);
    rootLayout->addWidget(identityFrame);

    auto* bodyRow = new QHBoxLayout();
    bodyRow->setSpacing(18);

    auto* controlFrame = new QFrame(this);
    controlFrame->setStyleSheet(sectionFrameStyle());
    auto* controlLayout = new QVBoxLayout(controlFrame);
    controlLayout->setContentsMargins(20, 18, 20, 18);
    controlLayout->setSpacing(12);

    auto* controlTitle = new QLabel(QStringLiteral("Session Controls"), controlFrame);
    controlTitle->setObjectName("sectionTitle");
    controlLayout->addWidget(controlTitle);

    auto* controlBody = new QLabel(
        QStringLiteral("Host a room on your LAN or join a nearby host by IP. For same-PC testing, launch a second copy and join 127.0.0.1. Your selected fighter from the lobby is pushed into the session automatically."),
        controlFrame);
    controlBody->setObjectName("sectionBody");
    controlBody->setWordWrap(true);
    controlLayout->addWidget(controlBody);

    hostAddressEdit_ = new QLineEdit(controlFrame);
    hostAddressEdit_->setPlaceholderText(QStringLiteral("Host IP, for example 192.168.1.14 or 127.0.0.1"));
    controlLayout->addWidget(hostAddressEdit_);

    portEdit_ = new QLineEdit(QString::number(kDefaultLanPort), controlFrame);
    portEdit_->setValidator(new QIntValidator(1024, 65535, portEdit_));
    portEdit_->setPlaceholderText(QStringLiteral("Port"));
    controlLayout->addWidget(portEdit_);

    hostButton_ = new QPushButton(QStringLiteral("Host Arena Link"), controlFrame);
    hostButton_->setStyleSheet(primaryButtonStyle());
    connect(hostButton_, &QPushButton::clicked, this, &LanArenaPage::hostSession);
    controlLayout->addWidget(hostButton_);

    joinButton_ = new QPushButton(QStringLiteral("Join Host"), controlFrame);
    joinButton_->setStyleSheet(secondaryButtonStyle());
    connect(joinButton_, &QPushButton::clicked, this, &LanArenaPage::joinSession);
    controlLayout->addWidget(joinButton_);

    readyButton_ = new QPushButton(QStringLiteral("Broadcast Ready"), controlFrame);
    readyButton_->setCheckable(true);
    readyButton_->setStyleSheet(secondaryButtonStyle());
    connect(readyButton_, &QPushButton::toggled, this, &LanArenaPage::readyToggled);
    controlLayout->addWidget(readyButton_);

    primeButton_ = new QPushButton(QStringLiteral("Prime Match Sync"), controlFrame);
    primeButton_->setStyleSheet(primaryButtonStyle());
    connect(primeButton_, &QPushButton::clicked, this, &LanArenaPage::primeMatch);
    controlLayout->addWidget(primeButton_);

    disconnectButton_ = new QPushButton(QStringLiteral("Disconnect"), controlFrame);
    disconnectButton_->setStyleSheet(secondaryButtonStyle());
    connect(disconnectButton_, &QPushButton::clicked, this, &LanArenaPage::disconnectSession);
    controlLayout->addWidget(disconnectButton_);
    controlLayout->addStretch(1);

    bodyRow->addWidget(controlFrame, 4);

    auto* overviewFrame = new QFrame(this);
    overviewFrame->setStyleSheet(sectionFrameStyle());
    auto* overviewLayout = new QVBoxLayout(overviewFrame);
    overviewLayout->setContentsMargins(20, 18, 20, 18);
    overviewLayout->setSpacing(12);

    auto* statusRow = new QHBoxLayout();
    statusRow->setSpacing(10);

    statusBadgeLabel_ = new QLabel(QStringLiteral("IDLE"), overviewFrame);
    statusBadgeLabel_->setAlignment(Qt::AlignCenter);
    statusRow->addWidget(statusBadgeLabel_, 0, Qt::AlignTop);

    auto* statusCopyWrap = new QVBoxLayout();
    statusCopyWrap->setSpacing(4);
    statusTextLabel_ = new QLabel(overviewFrame);
    statusTextLabel_->setObjectName("sectionBody");
    statusTextLabel_->setWordWrap(true);
    statusCopyWrap->addWidget(statusTextLabel_);

    sessionMetaLabel_ = new QLabel(overviewFrame);
    sessionMetaLabel_->setObjectName("sectionBody");
    sessionMetaLabel_->setWordWrap(true);
    statusCopyWrap->addWidget(sessionMetaLabel_);
    statusRow->addLayout(statusCopyWrap, 1);
    overviewLayout->addLayout(statusRow);

    auto* playerRow = new QHBoxLayout();
    playerRow->setSpacing(12);

    localPlayerLabel_ = new QLabel(overviewFrame);
    localPlayerLabel_->setStyleSheet(
        "background: rgba(212,160,23,0.08); border:1px solid rgba(212,160,23,0.20); border-radius:14px; padding:12px; color:#F5E6D3; font: 12px 'Segoe UI';"
    );
    localPlayerLabel_->setWordWrap(true);
    playerRow->addWidget(localPlayerLabel_, 1);

    remotePlayerLabel_ = new QLabel(overviewFrame);
    remotePlayerLabel_->setStyleSheet(
        "background: rgba(212,160,23,0.08); border:1px solid rgba(212,160,23,0.20); border-radius:14px; padding:12px; color:#F5E6D3; font: 12px 'Segoe UI';"
    );
    remotePlayerLabel_->setWordWrap(true);
    playerRow->addWidget(remotePlayerLabel_, 1);
    overviewLayout->addLayout(playerRow);

    duelHintLabel_ = new QLabel(overviewFrame);
    duelHintLabel_->setObjectName("sectionBody");
    duelHintLabel_->setWordWrap(true);
    overviewLayout->addWidget(duelHintLabel_);

    auto* logTitle = new QLabel(QStringLiteral("Session Log"), overviewFrame);
    logTitle->setObjectName("sectionTitle");
    overviewLayout->addWidget(logTitle);

    eventLog_ = new QPlainTextEdit(overviewFrame);
    eventLog_->setReadOnly(true);
    eventLog_->setMaximumBlockCount(220);
    overviewLayout->addWidget(eventLog_, 1);

    bodyRow->addWidget(overviewFrame, 6);
    rootLayout->addLayout(bodyRow, 1);

    refreshIdentityLabels();
    refreshAddressHints();
    applySnapshot(LanSessionSnapshot());
    appendLog(QStringLiteral("Arena Link page loaded. Choose Host or Join to establish the LAN session. For same-PC rehearsal, launch a second copy and join 127.0.0.1."));
}

void LanArenaPage::refreshIdentityLabels() {
    const QString safeUser = username_.isEmpty() ? QStringLiteral("Player_01") : username_;
    const QString safeFighter = fighterName_.isEmpty() ? QStringLiteral("Knight") : fighterName_;
    identityLabel_->setText(QString("Identity synced from lobby: %1 using %2").arg(safeUser, safeFighter));
}

void LanArenaPage::refreshAddressHints() {
    if (!addressHintLabel_) {
        return;
    }

    QStringList hints;
    if (sessionManager_) {
        hints = sessionManager_->localAddressHints();
    }
    if (hints.isEmpty()) {
        hints << QStringLiteral("127.0.0.1");
    }

    addressHintLabel_->setText(QString("Nearby PCs should join one of these LAN addresses: %1. Same-machine rehearsal can use 127.0.0.1.").arg(hints.join(QStringLiteral("  |  "))));
}

quint16 LanArenaPage::selectedPort() const {
    bool ok = false;
    const int portValue = portEdit_->text().toInt(&ok);
    if (!ok || portValue < 1024 || portValue > 65535) {
        return kDefaultLanPort;
    }
    return static_cast<quint16>(portValue);
}

QString LanArenaPage::buildPlayerSummary(const QString& title, const LanPlayerInfo& player, bool connected) const {
    const QString safeName = player.username.trimmed().isEmpty() ? QStringLiteral("--") : player.username;
    const QString safeFighter = player.fighterName.trimmed().isEmpty() ? QStringLiteral("--") : player.fighterName;
    const QString linkState = connected ? QStringLiteral("Linked") : QStringLiteral("Waiting");
    const QString readyState = player.ready ? QStringLiteral("READY") : QStringLiteral("STAGING");

    return QString("<b>%1</b><br>Name: %2<br>Fighter: %3<br>Connection: %4<br>State: %5")
        .arg(title, safeName, safeFighter, linkState, readyState);
}

QString LanArenaPage::badgeStyleForState(LanSessionState state) const {
    QString background = QStringLiteral("rgba(108,122,137,0.30)");
    QString border = QStringLiteral("#6C7A89");

    switch (state) {
        case LanSessionState::HOSTING:
            background = QStringLiteral("rgba(52,152,219,0.24)");
            border = QStringLiteral("#2E86DE");
            break;
        case LanSessionState::CONNECTING:
            background = QStringLiteral("rgba(142,68,173,0.24)");
            border = QStringLiteral("#8E44AD");
            break;
        case LanSessionState::LINKED:
            background = QStringLiteral("rgba(39,174,96,0.24)");
            border = QStringLiteral("#2ECC71");
            break;
        case LanSessionState::READY_CHECK:
            background = QStringLiteral("rgba(212,160,23,0.24)");
            border = QStringLiteral("#D4A017");
            break;
        case LanSessionState::MATCH_PRIMED:
            background = QStringLiteral("rgba(230,126,34,0.26)");
            border = QStringLiteral("#E67E22");
            break;
        case LanSessionState::ERROR:
            background = QStringLiteral("rgba(192,57,43,0.28)");
            border = QStringLiteral("#E74C3C");
            break;
        case LanSessionState::IDLE:
        default:
            break;
    }

    return QString(
        "color:#FFF0C6; font:800 11px 'Segoe UI'; letter-spacing:1px;"
        "padding:7px 12px; border-radius:12px;"
        "background:%1; border:1px solid %2;"
    ).arg(background, border);
}
