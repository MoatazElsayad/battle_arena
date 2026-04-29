#include "LanArenaPage.h"

#include "LanSessionManager.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
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
        " background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 rgba(33,23,17,0.92), stop:1 rgba(16,11,9,0.97));"
        " border: 1px solid rgba(212,160,23,0.10);"
        " border-radius: 24px;"
        "}"
    );
}

QString insetPanelStyle() {
    return QStringLiteral(
        "QFrame {"
        " background: rgba(58,39,23,0.34);"
        " border: 1px solid rgba(212,160,23,0.08);"
        " border-radius: 18px;"
        "}"
    );
}

QString playerCardStyle() {
    return QStringLiteral(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(67,45,26,0.55), stop:1 rgba(28,19,13,0.92));"
        "border:1px solid rgba(212,160,23,0.10);"
        "border-radius:18px;"
        "padding:14px 16px;"
        "color:#F5E6D3;"
        "font: 12px 'Segoe UI';"
    );
}

QString secondaryButtonStyle() {
    return QStringLiteral(
        "QPushButton {"
        " background: rgba(67,47,30,0.82);"
        " border: 1px solid rgba(212,160,23,0.18);"
        " border-radius: 16px;"
        " color: #F4DFB6;"
        " font: 700 14px 'Segoe UI';"
        " padding: 12px 18px;"
        "}"
        "QPushButton:hover { background: rgba(87,60,36,0.92); border-color: rgba(255,214,128,0.28); }"
        "QPushButton:checked { background: rgba(121,82,31,0.92); border-color: rgba(224,178,74,0.30); color: #FFF2CE; }"
        "QPushButton:disabled { color: rgba(240,222,178,0.42); background: rgba(58,42,27,0.30); border-color: rgba(212,160,23,0.08); }"
    );
}

QString primaryButtonStyle() {
    return QStringLiteral(
        "QPushButton {"
        " background: qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #6E0D0D, stop:0.45 #A81616, stop:1 #D84321);"
        " border: 1px solid rgba(255,178,153,0.38);"
        " border-radius: 16px;"
        " color: #FFE6E6;"
        " font: 800 16px 'Segoe UI';"
        " padding: 13px 22px;"
        "}"
        "QPushButton:hover { border-color: rgba(255,178,153,0.62); }"
        "QPushButton:disabled { color: rgba(255,230,230,0.44); background: rgba(110,13,13,0.34); border-color: rgba(216,106,85,0.18); }"
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
    readyButton_->setText(snapshot.localPlayer.ready ? QStringLiteral("Ready Broadcasted") : QStringLiteral("Broadcast Ready"));
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
        "QWidget { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #120D0B, stop:0.35 #1E140F, stop:0.72 #15100D, stop:1 #0A0908); color: #F5E6D3; }"
        "QLabel#eyebrow { color: rgba(245,213,143,0.62); font: 700 11px 'Segoe UI'; letter-spacing: 2px; }"
        "QLabel#headline { color:#FFF0C6; font: 900 28px 'Segoe UI'; }"
        "QLabel#subheadline { color: rgba(245,230,184,0.78); font: 13px 'Segoe UI'; }"
        "QLabel#sectionTitle { color:#FFF0C6; font: 800 18px 'Segoe UI'; }"
        "QLabel#microLabel { color: rgba(245,213,143,0.68); font: 700 10px 'Segoe UI'; letter-spacing: 1px; }"
        "QLabel#sectionBody { color: rgba(245,230,184,0.72); font: 12px 'Segoe UI'; }"
        "QLineEdit {"
        " background-color: rgba(49,35,23,0.94);"
        " color: #F5E6D3;"
        " border: 1px solid rgba(124,90,36,0.34);"
        " border-radius: 14px;"
        " padding: 11px 12px;"
        " font: 13px 'Segoe UI';"
        "}"
        "QLineEdit:focus { border-color: rgba(212,160,23,0.40); background: rgba(58,40,24,0.98); }"
        "QPlainTextEdit {"
        " background: rgba(14,10,8,0.90);"
        " color: #EEDCB3;"
        " border: 1px solid rgba(212,160,23,0.10);"
        " border-radius: 18px;"
        " font: 12px 'Consolas';"
        " padding: 10px;"
        "}"
    );

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(30, 24, 30, 24);
    rootLayout->setSpacing(20);

    auto* headerRow = new QHBoxLayout();
    headerRow->setSpacing(18);

    auto* titleWrap = new QVBoxLayout();
    titleWrap->setSpacing(4);

    auto* eyebrow = new QLabel(QStringLiteral("LAN OPERATIONS"), this);
    eyebrow->setObjectName("eyebrow");
    titleWrap->addWidget(eyebrow);

    auto* title = new QLabel(QStringLiteral("ARENA LINK"), this);
    title->setObjectName("headline");
    titleWrap->addWidget(title);

    auto* subtitle = new QLabel(
        QStringLiteral("Host, join, sync, and launch a respectful LAN duel flow without leaving the lobby identity behind. This route also supports same-machine rehearsal over localhost."),
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
    auto* identityLayout = new QHBoxLayout(identityFrame);
    identityLayout->setContentsMargins(22, 18, 22, 18);
    identityLayout->setSpacing(18);

    auto* identityWrap = new QVBoxLayout();
    identityWrap->setSpacing(8);

    auto* identityCopyWrap = new QVBoxLayout();
    identityCopyWrap->setSpacing(2);

    auto* identityEyebrow = new QLabel(QStringLiteral("LINK BRIEF"), identityFrame);
    identityEyebrow->setObjectName("eyebrow");
    identityCopyWrap->addWidget(identityEyebrow);

    identityLabel_ = new QLabel(identityFrame);
    identityLabel_->setStyleSheet("color:#FFF0C6; font:700 16px 'Segoe UI';");
    identityLabel_->setWordWrap(true);
    identityCopyWrap->addWidget(identityLabel_);
    identityWrap->addLayout(identityCopyWrap);

    addressHintLabel_ = new QLabel(identityFrame);
    addressHintLabel_->setObjectName("sectionBody");
    addressHintLabel_->setWordWrap(true);
    identityWrap->addWidget(addressHintLabel_);
    identityLayout->addLayout(identityWrap, 2);

    auto* statusPanel = new QFrame(identityFrame);
    statusPanel->setStyleSheet(insetPanelStyle());
    auto* statusPanelLayout = new QVBoxLayout(statusPanel);
    statusPanelLayout->setContentsMargins(16, 14, 16, 14);
    statusPanelLayout->setSpacing(6);

    auto* statusEyebrow = new QLabel(QStringLiteral("SESSION STATUS"), statusPanel);
    statusEyebrow->setObjectName("eyebrow");
    statusPanelLayout->addWidget(statusEyebrow);

    auto* badgeRow = new QHBoxLayout();
    badgeRow->setSpacing(10);

    statusBadgeLabel_ = new QLabel(QStringLiteral("IDLE"), statusPanel);
    statusBadgeLabel_->setAlignment(Qt::AlignCenter);
    badgeRow->addWidget(statusBadgeLabel_, 0, Qt::AlignLeft | Qt::AlignVCenter);

    statusTextLabel_ = new QLabel(statusPanel);
    statusTextLabel_->setObjectName("sectionBody");
    statusTextLabel_->setWordWrap(true);
    badgeRow->addWidget(statusTextLabel_, 1);
    statusPanelLayout->addLayout(badgeRow);

    sessionMetaLabel_ = new QLabel(statusPanel);
    sessionMetaLabel_->setObjectName("sectionBody");
    sessionMetaLabel_->setWordWrap(true);
    statusPanelLayout->addWidget(sessionMetaLabel_);

    identityLayout->addWidget(statusPanel, 1);
    rootLayout->addWidget(identityFrame);

    auto* bodyRow = new QHBoxLayout();
    bodyRow->setSpacing(20);

    auto* controlFrame = new QFrame(this);
    controlFrame->setStyleSheet(sectionFrameStyle());
    auto* controlLayout = new QVBoxLayout(controlFrame);
    controlLayout->setContentsMargins(22, 20, 22, 20);
    controlLayout->setSpacing(14);

    auto* controlEyebrow = new QLabel(QStringLiteral("HOST OR JOIN"), controlFrame);
    controlEyebrow->setObjectName("eyebrow");
    controlLayout->addWidget(controlEyebrow);

    auto* controlTitle = new QLabel(QStringLiteral("Session Controls"), controlFrame);
    controlTitle->setObjectName("sectionTitle");
    controlLayout->addWidget(controlTitle);

    auto* controlBody = new QLabel(
        QStringLiteral("Pick a host address, confirm the duel port, then either open the arena for your LAN or join a nearby machine. Your selected fighter from the lobby travels into the session automatically."),
        controlFrame);
    controlBody->setObjectName("sectionBody");
    controlBody->setWordWrap(true);
    controlLayout->addWidget(controlBody);

    auto* inputPanel = new QFrame(controlFrame);
    inputPanel->setStyleSheet(insetPanelStyle());
    auto* inputGrid = new QGridLayout(inputPanel);
    inputGrid->setContentsMargins(16, 14, 16, 14);
    inputGrid->setHorizontalSpacing(12);
    inputGrid->setVerticalSpacing(8);

    auto* hostLabel = new QLabel(QStringLiteral("HOST ADDRESS"), inputPanel);
    hostLabel->setObjectName("microLabel");
    inputGrid->addWidget(hostLabel, 0, 0);

    auto* portLabel = new QLabel(QStringLiteral("DUEL PORT"), inputPanel);
    portLabel->setObjectName("microLabel");
    inputGrid->addWidget(portLabel, 0, 1);

    hostAddressEdit_ = new QLineEdit(inputPanel);
    hostAddressEdit_->setPlaceholderText(QStringLiteral("Host IP, for example 192.168.1.14 or 127.0.0.1"));
    inputGrid->addWidget(hostAddressEdit_, 1, 0);

    portEdit_ = new QLineEdit(QString::number(kDefaultLanPort), inputPanel);
    portEdit_->setValidator(new QIntValidator(1024, 65535, portEdit_));
    portEdit_->setPlaceholderText(QStringLiteral("Port"));
    inputGrid->addWidget(portEdit_, 1, 1);
    inputGrid->setColumnStretch(0, 3);
    inputGrid->setColumnStretch(1, 1);
    controlLayout->addWidget(inputPanel);

    hostButton_ = new QPushButton(QStringLiteral("Host Arena Link"), controlFrame);
    hostButton_->setStyleSheet(primaryButtonStyle());
    connect(hostButton_, &QPushButton::clicked, this, &LanArenaPage::hostSession);

    joinButton_ = new QPushButton(QStringLiteral("Join Host"), controlFrame);
    joinButton_->setStyleSheet(secondaryButtonStyle());
    connect(joinButton_, &QPushButton::clicked, this, &LanArenaPage::joinSession);

    readyButton_ = new QPushButton(QStringLiteral("Broadcast Ready"), controlFrame);
    readyButton_->setCheckable(true);
    readyButton_->setStyleSheet(secondaryButtonStyle());
    connect(readyButton_, &QPushButton::toggled, this, &LanArenaPage::readyToggled);

    primeButton_ = new QPushButton(QStringLiteral("Prime Match Sync"), controlFrame);
    primeButton_->setStyleSheet(primaryButtonStyle());
    connect(primeButton_, &QPushButton::clicked, this, &LanArenaPage::primeMatch);

    disconnectButton_ = new QPushButton(QStringLiteral("Disconnect"), controlFrame);
    disconnectButton_->setStyleSheet(secondaryButtonStyle());
    connect(disconnectButton_, &QPushButton::clicked, this, &LanArenaPage::disconnectSession);
    
    auto* actionGrid = new QGridLayout();
    actionGrid->setHorizontalSpacing(12);
    actionGrid->setVerticalSpacing(12);
    actionGrid->addWidget(hostButton_, 0, 0);
    actionGrid->addWidget(joinButton_, 0, 1);
    actionGrid->addWidget(readyButton_, 1, 0);
    actionGrid->addWidget(primeButton_, 1, 1);
    actionGrid->addWidget(disconnectButton_, 2, 0, 1, 2);
    controlLayout->addLayout(actionGrid);

    auto* controlFootnote = new QLabel(
        QStringLiteral("For one-machine rehearsal, launch a second copy of the game and join 127.0.0.1."),
        controlFrame);
    controlFootnote->setObjectName("sectionBody");
    controlFootnote->setWordWrap(true);
    controlLayout->addWidget(controlFootnote);

    controlLayout->addStretch(1);

    bodyRow->addWidget(controlFrame, 4);

    auto* overviewFrame = new QFrame(this);
    overviewFrame->setStyleSheet(sectionFrameStyle());
    auto* overviewLayout = new QVBoxLayout(overviewFrame);
    overviewLayout->setContentsMargins(22, 20, 22, 20);
    overviewLayout->setSpacing(14);

    auto* overviewEyebrow = new QLabel(QStringLiteral("DUEL BRIDGE"), overviewFrame);
    overviewEyebrow->setObjectName("eyebrow");
    overviewLayout->addWidget(overviewEyebrow);

    auto* overviewTitle = new QLabel(QStringLiteral("Connected Gladiators"), overviewFrame);
    overviewTitle->setObjectName("sectionTitle");
    overviewLayout->addWidget(overviewTitle);

    auto* playerRow = new QHBoxLayout();
    playerRow->setSpacing(12);

    localPlayerLabel_ = new QLabel(overviewFrame);
    localPlayerLabel_->setStyleSheet(playerCardStyle());
    localPlayerLabel_->setWordWrap(true);
    playerRow->addWidget(localPlayerLabel_, 1);

    remotePlayerLabel_ = new QLabel(overviewFrame);
    remotePlayerLabel_->setStyleSheet(playerCardStyle());
    remotePlayerLabel_->setWordWrap(true);
    playerRow->addWidget(remotePlayerLabel_, 1);
    overviewLayout->addLayout(playerRow);

    duelHintLabel_ = new QLabel(overviewFrame);
    duelHintLabel_->setStyleSheet(
        "background: rgba(74,52,30,0.40);"
        "border:1px solid rgba(212,160,23,0.08);"
        "border-radius:14px;"
        "padding:12px 14px;"
        "color: rgba(245,230,184,0.78);"
        "font: 12px 'Segoe UI';"
    );
    duelHintLabel_->setWordWrap(true);
    overviewLayout->addWidget(duelHintLabel_);

    auto* logTitle = new QLabel(QStringLiteral("Session Log"), overviewFrame);
    logTitle->setObjectName("sectionTitle");
    overviewLayout->addWidget(logTitle);

    eventLog_ = new QPlainTextEdit(overviewFrame);
    eventLog_->setReadOnly(true);
    eventLog_->setMaximumBlockCount(220);
    eventLog_->setMinimumHeight(320);
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
    identityLabel_->setText(QString("Lobby identity synced for <b>%1</b> using <b>%2</b>.").arg(safeUser, safeFighter));
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

    addressHintLabel_->setText(QString("Reach this arena from nearby PCs with: %1. For same-machine rehearsal, use 127.0.0.1.").arg(hints.join(QStringLiteral("  |  "))));
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

    return QString(
        "<span style='color:#FFF0C6; font-size:16px; font-weight:800;'>%1</span>"
        "<br><span style='color:#F6D98E;'>Name</span>  %2"
        "<br><span style='color:#F6D98E;'>Fighter</span>  %3"
        "<br><span style='color:#F6D98E;'>Connection</span>  %4"
        "<br><span style='color:#F6D98E;'>State</span>  %5")
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
        "padding:8px 14px; border-radius:14px;"
        "background:%1; border:1px solid %2;"
    ).arg(background, border);
}
