// MainWindow.cpp - Main application window implementation

#include "MainWindow.h"
#include "BattleWidget.h"
#include "DatabaseManager.h"
#include "GameManager.h"
#include "GamePage.h"
#include "GameOverPage.h"
#include "PausePage.h"
#include "LeaderboardPage.h"
#include "LevelTransitionChroniclePage.h"
#include "LanArenaPage.h"
#include "LanSessionManager.h"
#include "SettingsPage.h"
#include "ProfileLobbyWidget.h"
#include "ProfilePage.h"
#include "SaveKingIntroPage.h"
#include "SoundManager.h"
#include "ExhibitionSetupPage.h"
#include "WebsiteSyncClient.h"
#include "InputHandler.h"
#include "Player.h"
#include "Enemy.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QListWidget>
#include <QStackedWidget>
#include <QWidget>
#include <QComboBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QFont>
#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QCoreApplication>
#include <QDir>
#include <QStatusBar>
#include <QFileInfo>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QVariantAnimation>

namespace {
class CoverBackgroundWidget : public QWidget {
public:
    explicit CoverBackgroundWidget(const QString& imagePath, QWidget* parent = nullptr)
        : QWidget(parent),
          background_(imagePath) {
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.fillRect(rect(), QColor("#090604"));
        if (background_.isNull()) {
            return;
        }

        const QSize targetSize = size();
        const QPixmap scaled = background_.scaled(targetSize,
                                                  Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation);
        const QRect sourceRect((scaled.width() - targetSize.width()) / 2,
                               (scaled.height() - targetSize.height()) / 2,
                               targetSize.width(),
                               targetSize.height());
        painter.drawPixmap(rect(), scaled, sourceRect);
    }

private:
    QPixmap background_;
};

QString resolveAssetPath(const QString& relativePath) {
    if (relativePath.isEmpty()) {
        return QString();
    }

    const QFileInfo directInfo(relativePath);
    if (directInfo.isAbsolute() && directInfo.exists()) {
        return directInfo.absoluteFilePath();
    }

    const QStringList candidates = {
        QDir::current().filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../") + relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../") + relativePath)
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }

    return QDir::cleanPath(relativePath);
}

QString characterImagePath(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN:
            return resolveAssetPath("assets/players/Arcen/image.png");
        case PlayerType::DEMON_SLAYER:
            return resolveAssetPath("assets/players/Demon_Slayer/image.png");
        case PlayerType::FANTASY_WARRIOR:
            return resolveAssetPath("assets/players/Fantasy_Warrior/image.png");
        case PlayerType::HUNTRESS:
            return resolveAssetPath("assets/players/Huntress/image.png");
        case PlayerType::KNIGHT:
            return resolveAssetPath("assets/players/Knight/image.png");
        case PlayerType::MARTIAL:
            return resolveAssetPath("assets/players/Martial/image.png");
        case PlayerType::MARTIAL_HERO:
            return resolveAssetPath("assets/players/Martial_Hero/image.png");
        case PlayerType::MEDIEVAL_WARRIOR:
            return resolveAssetPath("assets/players/Medieval_Warrior/image.png");
        case PlayerType::WIZARD:
            return resolveAssetPath("assets/players/Wizard/image.png");
        default:
            return QString();
    }
}

QString playerSpecialMoveText(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN:
            return "Long-range bow pressure with precise ranged control.";
        case PlayerType::DEMON_SLAYER:
            return "Heavy blade rushdown with clean melee finishers.";
        case PlayerType::FANTASY_WARRIOR:
            return "Balanced sword stance with steady frontline pressure.";
        case PlayerType::HUNTRESS:
            return "Quick footwork and agile strike-and-retreat spacing.";
        case PlayerType::KNIGHT:
            return "Disciplined armored offense with reliable close combat.";
        case PlayerType::MARTIAL:
            return "Fast chained hits built around relentless close pressure.";
        case PlayerType::MARTIAL_HERO:
            return "Heroic combo style with explosive burst windows.";
        case PlayerType::MEDIEVAL_WARRIOR:
            return "Grounded weapon control with measured heavy swings.";
        case PlayerType::WIZARD:
            return "Mystic pacing with calculated ranged and mid-range control.";
        default:
            return "Adaptive combat style ready for the arena.";
    }
}

PlayerType playerTypeFromStoredValue(int rawValue) {
    switch (rawValue) {
        case static_cast<int>(PlayerType::ARCEN):
            return PlayerType::ARCEN;
        case static_cast<int>(PlayerType::DEMON_SLAYER):
            return PlayerType::DEMON_SLAYER;
        case static_cast<int>(PlayerType::FANTASY_WARRIOR):
            return PlayerType::FANTASY_WARRIOR;
        case static_cast<int>(PlayerType::HUNTRESS):
            return PlayerType::HUNTRESS;
        case static_cast<int>(PlayerType::MARTIAL):
            return PlayerType::MARTIAL;
        case static_cast<int>(PlayerType::MARTIAL_HERO):
            return PlayerType::MARTIAL_HERO;
        case static_cast<int>(PlayerType::MEDIEVAL_WARRIOR):
            return PlayerType::MEDIEVAL_WARRIOR;
        case static_cast<int>(PlayerType::WIZARD):
            return PlayerType::WIZARD;
        case static_cast<int>(PlayerType::KNIGHT):
        default:
            return PlayerType::KNIGHT;
    }
}

QString logoImagePath() {
    return resolveAssetPath("assets/backgrounds/logo.png");
}

QPixmap transparentLogoPixmap(int maxHeight) {
    const QPixmap source(logoImagePath());
    if (source.isNull()) {
        return QPixmap();
    }

    QImage image = source.toImage().convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < image.height(); ++y) {
        QRgb* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = QColor::fromRgb(row[x]);
            if (color.red() < 28 && color.green() < 28 && color.blue() < 28) {
                row[x] = qRgba(color.red(), color.green(), color.blue(), 0);
            }
        }
    }

    return QPixmap::fromImage(image).scaledToHeight(maxHeight, Qt::SmoothTransformation);
}

QLabel* createLogoLabel(QWidget* parent, int maxHeight = 140) {
    auto* label = new QLabel(parent);
    label->setAlignment(Qt::AlignCenter);
    const QPixmap logo = transparentLogoPixmap(maxHeight);
    if (!logo.isNull()) {
        label->setPixmap(logo);
    } else {
        label->setText("GLADIATORS");
        label->setStyleSheet("color:#F2C86B; font:900 34px 'Segoe UI'; letter-spacing:2px;");
    }
    return label;
}

QString authPageStyle() {
    return QString(
        "QWidget { background-color: #110D0B; color: #F5E6D3; }"
        "QFrame#authHero {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(46,31,20,0.92), stop:1 rgba(24,17,13,0.96));"
        " border:1px solid rgba(212,160,23,0.20);"
        " border-radius:24px;"
        "}"
        "QFrame#authPanel {"
        " background:rgba(26,20,15,0.92);"
        " border:1px solid rgba(212,160,23,0.24);"
        " border-radius:24px;"
        "}"
        "QLabel#eyebrow { color:#F4D895; font:700 12px 'Segoe UI'; letter-spacing:1px; }"
        "QLabel#heroTitle { color:#FFF0C6; font:900 36px 'Segoe UI'; }"
        "QLabel#heroBody { color:rgba(245,230,184,0.80); font:14px 'Segoe UI'; }"
        "QLabel#panelTitle { color:#FFF0C6; font:800 28px 'Segoe UI'; }"
        "QLabel#panelBody { color:rgba(245,230,184,0.72); font:12px 'Segoe UI'; }"
        "QLabel#featureChip {"
        " color:#F7E6BD;"
        " background:rgba(212,160,23,0.10);"
        " border:1px solid rgba(212,160,23,0.20);"
        " border-radius:12px;"
        " padding:8px 10px;"
        " font:700 12px 'Segoe UI';"
        "}"
        "QLineEdit {"
        " background-color: rgba(43,31,22,0.94);"
        " color: #F5E6D3;"
        " border: 1px solid #7C5A24;"
        " border-radius: 12px;"
        " padding: 12px 14px;"
        " font: 13px 'Segoe UI';"
        "}"
        "QLineEdit:focus { border: 1px solid #D4A017; }"
        "QPushButton#primaryAction {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #7A1010, stop:1 #D43B24);"
        " color: #FFF1E8;"
        " border: 2px solid #E47C60;"
        " border-radius: 14px;"
        " padding: 12px 18px;"
        " font: 800 16px 'Segoe UI';"
        "}"
        "QPushButton#primaryAction:hover { border-color:#FFC19D; }"
        "QPushButton#secondaryAction {"
        " background: rgba(72,50,31,0.92);"
        " color: #F5E6D3;"
        " border: 1px solid #D4A017;"
        " border-radius: 14px;"
        " padding: 12px 18px;"
        " font: 700 14px 'Segoe UI';"
        "}"
        "QPushButton#secondaryAction:hover { background: rgba(98,68,41,0.98); }");
}

QString playerTypeApiKey(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN: return QStringLiteral("ARCEN");
        case PlayerType::DEMON_SLAYER: return QStringLiteral("DEMON_SLAYER");
        case PlayerType::FANTASY_WARRIOR: return QStringLiteral("FANTASY_WARRIOR");
        case PlayerType::HUNTRESS: return QStringLiteral("HUNTRESS");
        case PlayerType::KNIGHT: return QStringLiteral("KNIGHT");
        case PlayerType::MARTIAL: return QStringLiteral("MARTIAL");
        case PlayerType::MARTIAL_HERO: return QStringLiteral("MARTIAL_HERO");
        case PlayerType::MEDIEVAL_WARRIOR: return QStringLiteral("MEDIEVAL_WARRIOR");
        case PlayerType::WIZARD: return QStringLiteral("WIZARD");
    }

    return QStringLiteral("UNKNOWN");
}

QString enemyTypeApiKey(EnemyType type) {
    switch (type) {
        case EnemyType::FIRE_WORM: return QStringLiteral("FIRE_WORM");
        case EnemyType::FIRE_WIZARD: return QStringLiteral("FIRE_WIZARD");
        case EnemyType::FLYING_DEMON: return QStringLiteral("FLYING_DEMON");
        case EnemyType::NIGHTWEAVER: return QStringLiteral("NIGHTWEAVER");
        case EnemyType::EVIL_WIZARD: return QStringLiteral("EVIL_WIZARD");
        case EnemyType::BLACK_WEREWOLF: return QStringLiteral("BLACK_WEREWOLF");
        case EnemyType::RED_WEREWOLF: return QStringLiteral("RED_WEREWOLF");
        case EnemyType::WHITE_WEREWOLF: return QStringLiteral("WHITE_WEREWOLF");
    }

    return QStringLiteral("UNKNOWN");
}

PlayerType playerTypeFromName(const QString& value) {
    const QString key = value.trimmed().toCaseFolded();
    if (key == QStringLiteral("arcen")) return PlayerType::ARCEN;
    if (key == QStringLiteral("demon slayer")) return PlayerType::DEMON_SLAYER;
    if (key == QStringLiteral("fantasy warrior")) return PlayerType::FANTASY_WARRIOR;
    if (key == QStringLiteral("huntress")) return PlayerType::HUNTRESS;
    if (key == QStringLiteral("martial")) return PlayerType::MARTIAL;
    if (key == QStringLiteral("martial hero")) return PlayerType::MARTIAL_HERO;
    if (key == QStringLiteral("medieval warrior")) return PlayerType::MEDIEVAL_WARRIOR;
    if (key == QStringLiteral("wizard")) return PlayerType::WIZARD;
    return PlayerType::KNIGHT;
}

EnemyType enemyTypeFromName(const QString& value) {
    const QString key = value.trimmed().toCaseFolded();
    if (key == QStringLiteral("fire worm")) return EnemyType::FIRE_WORM;
    if (key == QStringLiteral("fire wizard")) return EnemyType::FIRE_WIZARD;
    if (key == QStringLiteral("flying demon")) return EnemyType::FLYING_DEMON;
    if (key == QStringLiteral("nightweaver")) return EnemyType::NIGHTWEAVER;
    if (key == QStringLiteral("evil wizard")) return EnemyType::EVIL_WIZARD;
    if (key == QStringLiteral("black werewolf")) return EnemyType::BLACK_WEREWOLF;
    if (key == QStringLiteral("red werewolf")) return EnemyType::RED_WEREWOLF;
    if (key == QStringLiteral("white werewolf")) return EnemyType::WHITE_WEREWOLF;
    return EnemyType::FIRE_WORM;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      stack_(nullptr),
    loginPage_(nullptr),
    registrationPage_(nullptr),
      welcomePage_(nullptr),
      setupPage_(nullptr),
      exhibitionSetupPage_(nullptr),
      profilePage_(nullptr),
      lanArenaPage_(nullptr),
      saveKingIntroPage_(nullptr),
      chroniclePage_(nullptr),
      battlePage_(nullptr),
      gameOverPage_(nullptr),
      pausePage_(nullptr),
      leaderboardPage_(nullptr),
      settingsPage_(nullptr),
    profileLobbyWidget_(nullptr),
      nameEdit_(nullptr),
      characterTypeCombo_(nullptr),
      profileNameValue_(nullptr),
      profileLevelValue_(nullptr),
      profileExpValue_(nullptr),
      profileStateValue_(nullptr),
      scoreList_(nullptr),
    loginUsernameEdit_(nullptr),
    loginPasswordEdit_(nullptr),
    regEmailEdit_(nullptr),
      regUsernameEdit_(nullptr),
    regPasswordEdit_(nullptr),
    regPasswordRepeatEdit_(nullptr),
      welcomeLogoLabel_(nullptr),
      welcomeLoadingFill_(nullptr),
      welcomeLogoFadeAnimation_(nullptr),
      welcomeLoadingAnimation_(nullptr),
      battleWidget_(nullptr),
      gamePage_(nullptr),
      battleTitleLabel_(nullptr),
      gameOverTitleLabel_(nullptr),
      gameOverSummaryLabel_(nullptr),
      welcomeTransitionTimer_(new QTimer(this)),
      gameManager_(nullptr),
      databaseManager_(nullptr),
      lanSessionManager_(nullptr),
      websiteSyncClient_(nullptr),
      soundManager_(nullptr),
    currentLobbyUsername_("Player_01"),
    currentCharacterIndex_(0),
      selectedPlayerType_(PlayerType::KNIGHT),
      saveKingSceneAction_(SaveKingSceneAction::None),
      chronicleCampaignComplete_(false) {
    setWindowTitle("Battle Arena");
    setWindowState(Qt::WindowMaximized);
    setStyleSheet("QMainWindow { background-color: #3D2817; color: #F5E6D3; }");
    
    // Initialize managers
    databaseManager_ = new DatabaseManager();
    gameManager_ = new GameManager();
    websiteSyncClient_ = new WebsiteSyncClient(this);
    connect(websiteSyncClient_, &WebsiteSyncClient::uploadSucceeded, this, [this](const QString&, const QString& message) {
        if (statusBar()) {
            statusBar()->showMessage(message, 4500);
        }
    });
    connect(websiteSyncClient_, &WebsiteSyncClient::uploadFailed, this, [this](const QString&, const QString& message) {
        if (statusBar()) {
            statusBar()->showMessage(message, 5500);
        }
    });
    // Ranking teammate:
    // MainWindow is a good place to coordinate:
    // - reading stored progression
    // - updating progression after battle results
    // - refreshing lobby/profile display
    // Sound teammate:
    // Re-enable and initialize SoundManager here.
    // MainWindow is the best place to switch music for welcome/login/lobby/battle/game-over.
    soundManager_ = new SoundManager();
    soundManager_->initialize();

    welcomeTransitionTimer_->setSingleShot(true);
    connect(welcomeTransitionTimer_, &QTimer::timeout, this, &MainWindow::showLoginPage);
    
    // Build the UI
    buildUi();
    
    // Connect game manager signals
    connect(gameManager_, &GameManager::battleFinished, this, &MainWindow::handleBattleFinished);
}

MainWindow::~MainWindow() {
    if (gameManager_) delete gameManager_;
    if (databaseManager_) delete databaseManager_;
    if (soundManager_) delete soundManager_;
}

void MainWindow::setLoggedInUsername(const QString &username) {
    const QString trimmed = username.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    currentLobbyUsername_ = trimmed;
    if (profileLobbyWidget_) {
        ProfileLobbyWidget::UserProfile profile = profileLobbyWidget_->userProfile();
        profile.username = currentLobbyUsername_;
        profileLobbyWidget_->setUserProfile(profile);
    }
    if (lanArenaPage_) {
        lanArenaPage_->setIdentity(
            currentLobbyUsername_,
            QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_)),
            selectedPlayerType_);
    }
}

void MainWindow::buildUi() {
    // Create stacked widget to hold all pages
    stack_ = new QStackedWidget(this);
    setCentralWidget(stack_);

    // Create the pages
    loginPage_ = createLoginPage();
    registrationPage_ = createRegistrationPage();
    welcomePage_ = createWelcomePage();
    setupPage_ = createSetupPage();
    exhibitionSetupPage_ = qobject_cast<ExhibitionSetupPage*>(createExhibitionSetupPage());
    profilePage_ = createProfilePage();
    lanArenaPage_ = qobject_cast<LanArenaPage*>(createLanArenaPage());
    saveKingIntroPage_ = new SaveKingIntroPage(this);
    chroniclePage_ = new LevelTransitionChroniclePage(this);
    battlePage_ = createBattlePage();
    gameOverPage_ = new GameOverPage();
    pausePage_ = new PausePage(battlePage_);
    leaderboardPage_ = new LeaderboardPage();
    leaderboardPage_->setDatabaseManager(databaseManager_);
    settingsPage_ = new SettingsPage();
    if (soundManager_ && settingsPage_) {
        soundManager_->setMusicVolume(settingsPage_->getMusicVolume());
        soundManager_->setSoundVolume(settingsPage_->getSfxVolume());
    }

    // Add pages to stacked widget
    stack_->addWidget(loginPage_);
    stack_->addWidget(registrationPage_);
    stack_->addWidget(welcomePage_);
    stack_->addWidget(setupPage_);
    stack_->addWidget(exhibitionSetupPage_);
    stack_->addWidget(profilePage_);
    stack_->addWidget(lanArenaPage_);
    stack_->addWidget(saveKingIntroPage_);
    stack_->addWidget(chroniclePage_);
    stack_->addWidget(battlePage_);
    stack_->addWidget(gameOverPage_);
    stack_->addWidget(leaderboardPage_);
    stack_->addWidget(settingsPage_);

    // Connect page signals
    connect(gameOverPage_, &GameOverPage::playAgain, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        if (gameManager_ && gameManager_->isLanDuel()) {
            showLanArenaPage();
            return;
        }
        if (gameManager_ && gameManager_->isDuelMode()) {
            showExhibitionSetupPage();
            return;
        }
        startDemo();
    });
    connect(gameOverPage_, &GameOverPage::backToMenu, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        if (gameManager_ && gameManager_->isLanDuel()) {
            showLanArenaPage();
            return;
        }
        if (gameManager_ && gameManager_->isDuelMode()) {
            showExhibitionSetupPage();
            return;
        }
        showSetupPage();
    });
    connect(gamePage_, &GamePage::pauseRequested, this, [this]() {
        if (!pausePage_ || !gamePage_ || stack_->currentWidget() != battlePage_ || !gamePage_->hasActiveBattle()) {
            return;
        }

        gamePage_->pauseBattle();
        pausePage_->showPauseOverlay();
    });
    connect(pausePage_, &PausePage::resumeClicked, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        if (pausePage_) {
            pausePage_->hide();
        }
        if (gamePage_) {
            gamePage_->resumeBattle();
        }
    });
    connect(pausePage_, &PausePage::menuClicked, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        if (pausePage_) {
            pausePage_->hide();
        }
        showSetupPage();
    });
    connect(pausePage_, &PausePage::settingsClicked, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        if (pausePage_) {
            pausePage_->hide();
        }
        showSettingsPage();
    });
    connect(leaderboardPage_, &LeaderboardPage::backClicked, this, &MainWindow::showSetupPage);
    connect(settingsPage_, &SettingsPage::backClicked, this, &MainWindow::showSetupPage);
    connect(settingsPage_, &SettingsPage::settingsChanged, this,
            [this](int musicVolume, int sfxVolume, DifficultyLevel) {
                if (!soundManager_) {
                    return;
                }
                soundManager_->setMusicVolume(musicVolume);
                soundManager_->setSoundVolume(sfxVolume);
                soundManager_->playUIConfirm();
            });
    connect(saveKingIntroPage_, &SaveKingIntroPage::sceneFinished, this, &MainWindow::handleSaveKingSceneFinished);
    connect(chroniclePage_, &LevelTransitionChroniclePage::continueRequested, this, &MainWindow::continueAfterChronicle);

    // Show intro splash first
    showWelcomePage();
}

QWidget* MainWindow::createLoginPage() {
    QWidget *page = new QWidget();
    page->setStyleSheet(authPageStyle());

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(42, 32, 42, 32);
    layout->setSpacing(18);
    layout->addStretch(1);

    layout->addWidget(createLogoLabel(page, 235), 0, Qt::AlignHCenter);

    QFrame *panel = new QFrame(page);
    panel->setObjectName("authPanel");
    panel->setMaximumWidth(620);
    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(38, 34, 38, 34);
    panelLayout->setSpacing(16);

    QLabel *title = new QLabel("Login", panel);
    title->setObjectName("panelTitle");
    title->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(title);

    loginUsernameEdit_ = new QLineEdit(panel);
    loginUsernameEdit_->setPlaceholderText("Username or Email");
    panelLayout->addWidget(loginUsernameEdit_);

    loginPasswordEdit_ = new QLineEdit(panel);
    loginPasswordEdit_->setPlaceholderText("Password");
    loginPasswordEdit_->setEchoMode(QLineEdit::Password);
    panelLayout->addWidget(loginPasswordEdit_);

    connect(loginUsernameEdit_, &QLineEdit::returnPressed, this, &MainWindow::attemptLogin);
    connect(loginPasswordEdit_, &QLineEdit::returnPressed, this, &MainWindow::attemptLogin);

    QPushButton *loginButton = new QPushButton("Login", panel);
    loginButton->setObjectName("primaryAction");
    connect(loginButton, &QPushButton::clicked, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        attemptLogin();
    });
    panelLayout->addWidget(loginButton);

    QPushButton *registerButton = new QPushButton("Create Account", panel);
    registerButton->setObjectName("secondaryAction");
    connect(registerButton, &QPushButton::clicked, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        showRegistrationPage();
    });
    panelLayout->addWidget(registerButton);

    layout->addWidget(panel, 0, Qt::AlignHCenter);
    layout->addStretch(1);
    return page;
}

QWidget* MainWindow::createRegistrationPage() {
    QWidget *page = new QWidget();
    page->setStyleSheet(authPageStyle());

    QHBoxLayout *layout = new QHBoxLayout(page);
    layout->setContentsMargins(42, 32, 42, 32);
    layout->setSpacing(26);

    QFrame *hero = new QFrame(page);
    hero->setObjectName("authHero");
    QVBoxLayout *heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(34, 34, 34, 34);
    heroLayout->setSpacing(16);

    QLabel *heroEyebrow = new QLabel("CREATE YOUR PROFILE", hero);
    heroEyebrow->setObjectName("eyebrow");
    heroLayout->addWidget(heroEyebrow);

    heroLayout->addWidget(createLogoLabel(hero, 150), 0, Qt::AlignLeft);

    QLabel *heroTitle = new QLabel("Build a new challenger and unlock the gates of the arena.", hero);
    heroTitle->setObjectName("heroTitle");
    heroTitle->setWordWrap(true);
    heroLayout->addWidget(heroTitle);

    QLabel *heroBody = new QLabel("Create your account to save your results, return to the lobby faster, and keep progressing through the enemy stages.", hero);
    heroBody->setObjectName("heroBody");
    heroBody->setWordWrap(true);
    heroLayout->addWidget(heroBody);

    QLabel *featureOne = new QLabel("Register once, then jump straight into the lobby", hero);
    featureOne->setObjectName("featureChip");
    heroLayout->addWidget(featureOne, 0, Qt::AlignLeft);

    QLabel *featureTwo = new QLabel("Keep your identity and score attached to each run", hero);
    featureTwo->setObjectName("featureChip");
    heroLayout->addWidget(featureTwo, 0, Qt::AlignLeft);
    heroLayout->addStretch(1);

    layout->addWidget(hero, 5);

    QFrame *panel = new QFrame(page);
    panel->setObjectName("authPanel");
    panel->setMaximumWidth(520);
    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(30, 30, 30, 30);
    panelLayout->setSpacing(14);

    QLabel *title = new QLabel("Registration", panel);
    title->setObjectName("panelTitle");
    title->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(title);

    QLabel *body = new QLabel("Set up a quick profile and continue to login.", panel);
    body->setObjectName("panelBody");
    body->setWordWrap(true);
    body->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(body);

    regEmailEdit_ = new QLineEdit(panel);
    regEmailEdit_->setPlaceholderText("Email");
    panelLayout->addWidget(regEmailEdit_);

    regUsernameEdit_ = new QLineEdit(panel);
    regUsernameEdit_->setPlaceholderText("Username");
    panelLayout->addWidget(regUsernameEdit_);

    regPasswordEdit_ = new QLineEdit(panel);
    regPasswordEdit_->setPlaceholderText("Password");
    regPasswordEdit_->setEchoMode(QLineEdit::Password);
    panelLayout->addWidget(regPasswordEdit_);

    regPasswordRepeatEdit_ = new QLineEdit(panel);
    regPasswordRepeatEdit_->setPlaceholderText("Repeat Password");
    regPasswordRepeatEdit_->setEchoMode(QLineEdit::Password);
    panelLayout->addWidget(regPasswordRepeatEdit_);

    connect(regEmailEdit_, &QLineEdit::returnPressed, this, &MainWindow::attemptRegistration);
    connect(regUsernameEdit_, &QLineEdit::returnPressed, this, &MainWindow::attemptRegistration);
    connect(regPasswordEdit_, &QLineEdit::returnPressed, this, &MainWindow::attemptRegistration);
    connect(regPasswordRepeatEdit_, &QLineEdit::returnPressed, this, &MainWindow::attemptRegistration);

    QPushButton *registerButton = new QPushButton("Register", panel);
    registerButton->setObjectName("primaryAction");
    connect(registerButton, &QPushButton::clicked, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        attemptRegistration();
    });
    panelLayout->addWidget(registerButton);

    QPushButton *backButton = new QPushButton("Back to Login", panel);
    backButton->setObjectName("secondaryAction");
    connect(backButton, &QPushButton::clicked, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        showLoginPage();
    });
    panelLayout->addWidget(backButton);
    panelLayout->addStretch(1);

    layout->addWidget(panel, 4, Qt::AlignVCenter);
    return page;
}

QWidget* MainWindow::createWelcomePage() {
    auto *page = new CoverBackgroundWidget(resolveAssetPath(QStringLiteral("assets/backgrounds/intro.png")), this);
    page->setObjectName("welcomeRoot");

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 24);
    layout->setSpacing(0);
    layout->addStretch(1);

    welcomeLogoLabel_ = createLogoLabel(page, 470);
    auto *logoOpacity = new QGraphicsOpacityEffect(welcomeLogoLabel_);
    logoOpacity->setOpacity(0.34);
    welcomeLogoLabel_->setGraphicsEffect(logoOpacity);
    welcomeLogoFadeAnimation_ = new QVariantAnimation(page);
    welcomeLogoFadeAnimation_->setStartValue(0.34);
    welcomeLogoFadeAnimation_->setEndValue(1.0);
    welcomeLogoFadeAnimation_->setDuration(1400);
    connect(welcomeLogoFadeAnimation_, &QVariantAnimation::valueChanged, page, [logoOpacity](const QVariant& value) {
        logoOpacity->setOpacity(value.toReal());
    });

    layout->addWidget(welcomeLogoLabel_, 0, Qt::AlignHCenter);
    layout->addStretch(1);

    QFrame *loadingTrack = new QFrame(page);
    loadingTrack->setFixedSize(220, 8);
    loadingTrack->setStyleSheet(
        "background: rgba(12, 8, 6, 170);"
        "border: 1px solid rgba(212,160,23,0.32);"
        "border-radius: 4px;");

    QHBoxLayout *loadingLayout = new QHBoxLayout(loadingTrack);
    loadingLayout->setContentsMargins(1, 1, 1, 1);
    loadingLayout->setSpacing(0);

    welcomeLoadingFill_ = new QFrame(loadingTrack);
    welcomeLoadingFill_->setFixedWidth(0);
    welcomeLoadingFill_->setStyleSheet(
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #8A1A12, stop:0.55 #D14228, stop:1 #D4AF37);"
        "border-radius: 3px;");
    loadingLayout->addWidget(welcomeLoadingFill_, 0, Qt::AlignLeft);
    loadingLayout->addStretch(1);

    welcomeLoadingAnimation_ = new QVariantAnimation(page);
    welcomeLoadingAnimation_->setStartValue(0);
    welcomeLoadingAnimation_->setEndValue(loadingTrack->width() - 2);
    welcomeLoadingAnimation_->setDuration(3000);
    connect(welcomeLoadingAnimation_, &QVariantAnimation::valueChanged, page, [this](const QVariant& value) {
        if (welcomeLoadingFill_) {
            welcomeLoadingFill_->setFixedWidth(value.toInt());
        }
    });

    layout->addWidget(loadingTrack, 0, Qt::AlignHCenter | Qt::AlignBottom);
    return page;
}

QWidget* MainWindow::createSetupPage() {
    // LAN teammate:
    // Read duel setup from ProfileLobbyWidget here.
    // LAN Battle now routes into a real LAN session page instead of a placeholder alert.
    profileLobbyWidget_ = new ProfileLobbyWidget(this);

    sortedPlayerTypes_ = InputHandler::getCharactersSortedByFeatures();
    if (sortedPlayerTypes_.empty()) {
        sortedPlayerTypes_.push_back(PlayerType::KNIGHT);
    }

    currentCharacterIndex_ = 0;
    selectedPlayerType_ = sortedPlayerTypes_[currentCharacterIndex_];

    ProfileLobbyWidget::UserProfile profile;
    profile.username = currentLobbyUsername_;
    profile.score = 0;
    profile.badge = "Rookie";
    profile.avatarPath = QString();
    profileLobbyWidget_->setUserProfile(profile);

    ProfileLobbyWidget::Character character;
    character.name = QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_));
    character.imagePath = characterImagePath(selectedPlayerType_);
    character.specialMoves = playerSpecialMoveText(selectedPlayerType_);
    profileLobbyWidget_->setSelectedCharacter(character);

    connect(profileLobbyWidget_, &ProfileLobbyWidget::enterArenaClicked, this, [this](const QString& modeName) {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        if (modeName.compare(QStringLiteral("1v1 Exhibition"), Qt::CaseInsensitive) == 0) {
            showExhibitionSetupPage();
            return;
        }
        if (modeName.compare(QStringLiteral("Save the King"), Qt::CaseInsensitive) == 0
            || modeName.compare(QStringLiteral("Save the Kings"), Qt::CaseInsensitive) == 0) {
            startDemo();
            return;
        }

        if (modeName.compare(QStringLiteral("LAN Battle"), Qt::CaseInsensitive) == 0
            || modeName.compare(QStringLiteral("Arena Link"), Qt::CaseInsensitive) == 0) {
            showLanArenaPage();
            return;
        }

        QMessageBox::information(this,
                                 "Coming Soon",
                                 QString("%1 is coming soon.\n\nSelect 1v1 Exhibition, Save the Kings, or LAN Battle to enter the current ready combat themes.")
                                     .arg(modeName));
        if (soundManager_) {
            soundManager_->playUIError();
        }
    });

    connect(profileLobbyWidget_, &ProfileLobbyWidget::changeCharacterClicked, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        if (sortedPlayerTypes_.empty()) {
            return;
        }
        currentCharacterIndex_ = (currentCharacterIndex_ + 1) % static_cast<int>(sortedPlayerTypes_.size());
        selectedPlayerType_ = sortedPlayerTypes_[currentCharacterIndex_];

        ProfileLobbyWidget::Character updated;
        updated.name = QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_));
        updated.imagePath = characterImagePath(selectedPlayerType_);
        updated.specialMoves = playerSpecialMoveText(selectedPlayerType_);
        profileLobbyWidget_->setSelectedCharacter(updated);
        if (lanArenaPage_) {
            lanArenaPage_->setIdentity(currentLobbyUsername_, updated.name, selectedPlayerType_);
        }
    });

    connect(profileLobbyWidget_, &ProfileLobbyWidget::usernameEditRequested, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        bool ok = false;
        const QString entered = QInputDialog::getText(
            this,
            "Edit Username",
            "Username:",
            QLineEdit::Normal,
            currentLobbyUsername_,
            &ok
        );

        if (ok && !entered.trimmed().isEmpty()) {
            const QString updatedUsername = entered.trimmed();
            if (updatedUsername.compare(currentLobbyUsername_, Qt::CaseInsensitive) != 0 && databaseManager_) {
                QString errorMessage;
                if (!databaseManager_->renameUser(currentLobbyUsername_, updatedUsername, &errorMessage)) {
                    QMessageBox::warning(this,
                                         "Edit Username",
                                         errorMessage.isEmpty()
                                             ? QStringLiteral("Could not update the username.")
                                             : errorMessage);
                    return;
                }
            }

            currentLobbyUsername_ = updatedUsername;
            refreshProfile();
            if (lanArenaPage_) {
                lanArenaPage_->setIdentity(
                    currentLobbyUsername_,
                    QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_)),
                    selectedPlayerType_);
            }
        }
    });

    connect(profileLobbyWidget_, &ProfileLobbyWidget::settingsActionTriggered, this, [this](const QString& actionName) {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        if (actionName == "Settings") {
            showSettingsPage();
        } else if (actionName == "Crew") {
            stack_->setCurrentWidget(leaderboardPage_);
        }
    });

    return profileLobbyWidget_;
}

QWidget* MainWindow::createLanArenaPage() {
    lanSessionManager_ = new LanSessionManager(this);
    if (gamePage_) {
        gamePage_->setLanSessionManager(lanSessionManager_);
    }

    auto* page = new LanArenaPage(this);
    page->setSessionManager(lanSessionManager_);
    page->setIdentity(
        currentLobbyUsername_.trimmed().isEmpty() ? QStringLiteral("Player_01") : currentLobbyUsername_,
        QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_)),
        selectedPlayerType_);

    connect(page, &LanArenaPage::backRequested, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        if (lanSessionManager_) {
            lanSessionManager_->disconnectSession();
        }
        showSetupPage();
    });

    connect(lanSessionManager_, &LanSessionManager::matchPrimed, this, [this](const LanSessionSnapshot& snapshot) {
        if (!gameManager_ || !gamePage_ || !battlePage_) {
            return;
        }

        const QString localName = snapshot.localPlayer.username.trimmed().isEmpty()
            ? QStringLiteral("Player_01")
            : snapshot.localPlayer.username.trimmed();
        const QString remoteName = snapshot.remotePlayer.username.trimmed().isEmpty()
            ? QStringLiteral("Linked Rival")
            : snapshot.remotePlayer.username.trimmed();

        const PlayerType localType = playerTypeFromStoredValue(snapshot.localPlayer.fighterType);
        const PlayerType remoteType = playerTypeFromStoredValue(snapshot.remotePlayer.fighterType);

        currentLobbyUsername_ = localName;
        selectedPlayerType_ = localType;
        gameManager_->startLanDuel(localName.toStdString(),
                                   localType,
                                   remoteName.toStdString(),
                                   remoteType,
                                   snapshot.arenaName.toStdString());
        if (soundManager_) {
            soundManager_->playUIConfirm();
            soundManager_->playBattleMusic();
        }

        if (pausePage_) {
            pausePage_->hide();
        }
        stack_->setCurrentWidget(battlePage_);
        gamePage_->startBattle();
    });

    return page;
}

QWidget* MainWindow::createExhibitionSetupPage() {
    auto* page = new ExhibitionSetupPage(this);

    ProfileLobbyWidget::DuelSetup setup;
    setup.opponentMode = QStringLiteral("Manual");
    setup.opponentCategory = QStringLiteral("Player");
    setup.selectedOpponent = QStringLiteral("Arcen");
    setup.selectedArena = QStringLiteral("Colosseum");
    page->applySessionState(currentLobbyUsername_, sortedPlayerTypes_, selectedPlayerType_, setup);

    connect(page, &ExhibitionSetupPage::backRequested, this, [this]() {
        if (soundManager_) {
            soundManager_->playUIClick();
        }
        syncExhibitionSelectionToLobby();
        showSetupPage();
    });

    connect(page, &ExhibitionSetupPage::launchRequested, this, [this]() {
        syncExhibitionSelectionToLobby();
        startDuelMode();
    });

    return page;
}

QWidget* MainWindow::createProfilePage() {
    auto *page = new ProfilePage(this);

    connect(page, &ProfilePage::playClicked, this, &MainWindow::showSetupPage);
    connect(page, &ProfilePage::difficultyChanged, this, [](const QString &) {
        // TODO: integrate difficulty into GameManager when available
    });

    return page;
}

QWidget* MainWindow::createBattlePage() {
    // LAN teammate:
    // Reuse the same battle page for duel mode once the realtime duel controller is attached.
    gamePage_ = new GamePage();
    gamePage_->setGameManager(gameManager_);
    gamePage_->setLanSessionManager(lanSessionManager_);
    gamePage_->setSoundManager(soundManager_);
    connect(gamePage_, &GamePage::battleFinished, this, &MainWindow::handleBattleFinished);
    connect(gamePage_, &GamePage::chronicleRequested, this, &MainWindow::handleChronicleRequested);
    return gamePage_;
}

void MainWindow::showWelcomePage() {
    // Sound teammate:
    // Start welcome/intro music here.
    if (soundManager_) {
        soundManager_->playWelcomeMusic();
    }
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->stop();
        welcomeTransitionTimer_->start(3000);
    }
    if (welcomeLogoFadeAnimation_) {
        welcomeLogoFadeAnimation_->stop();
        welcomeLogoFadeAnimation_->start();
    }
    if (welcomeLoadingFill_) {
        welcomeLoadingFill_->setFixedWidth(0);
    }
    if (welcomeLoadingAnimation_) {
        welcomeLoadingAnimation_->stop();
        welcomeLoadingAnimation_->start();
    }
    saveKingSceneAction_ = SaveKingSceneAction::None;
    stack_->setCurrentWidget(welcomePage_);
}

void MainWindow::showLoginPage() {
    // Sound teammate:
    // Switch to login/register music or stop intro music here.
    if (soundManager_) {
        soundManager_->playWelcomeMusic();
    }
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->stop();
    }
    if (saveKingIntroPage_) {
        saveKingIntroPage_->stopScene();
    }
    chronicleCampaignComplete_ = false;
    saveKingSceneAction_ = SaveKingSceneAction::None;
    stack_->setCurrentWidget(loginPage_);
}

void MainWindow::showRegistrationPage() {
    if (soundManager_) {
        soundManager_->playWelcomeMusic();
    }
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->stop();
    }
    if (saveKingIntroPage_) {
        saveKingIntroPage_->stopScene();
    }
    saveKingSceneAction_ = SaveKingSceneAction::None;
    stack_->setCurrentWidget(registrationPage_);
}

void MainWindow::showSetupPage() {
    // Sound teammate:
    // Start lobby music here.
    if (soundManager_) {
        soundManager_->playLobbyMusic();
    }
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->stop();
    }
    if (saveKingIntroPage_) {
        saveKingIntroPage_->stopScene();
    }
    saveKingSceneAction_ = SaveKingSceneAction::None;
    stack_->setCurrentWidget(setupPage_);
}

void MainWindow::showLanArenaPage() {
    if (!lanArenaPage_) {
        return;
    }
    if (soundManager_) {
        soundManager_->playLobbyMusic();
    }

    if (pausePage_) {
        pausePage_->hide();
    }

    QString playerName = currentLobbyUsername_.trimmed();
    if (profileLobbyWidget_) {
        playerName = profileLobbyWidget_->userProfile().username.trimmed();
    }
    if (playerName.isEmpty()) {
        playerName = QStringLiteral("Player_01");
    }

    lanArenaPage_->setIdentity(
        playerName,
        QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_)),
        selectedPlayerType_);
    stack_->setCurrentWidget(lanArenaPage_);
}

void MainWindow::showExhibitionSetupPage() {
    if (!exhibitionSetupPage_) {
        return;
    }
    if (soundManager_) {
        soundManager_->playLobbyMusic();
    }
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->stop();
    }
    if (saveKingIntroPage_) {
        saveKingIntroPage_->stopScene();
    }

    QString playerName = currentLobbyUsername_.trimmed();
    if (profileLobbyWidget_) {
        playerName = profileLobbyWidget_->userProfile().username.trimmed();
    }
    if (playerName.isEmpty()) {
        playerName = QStringLiteral("Player_01");
    }

    ProfileLobbyWidget::DuelSetup setup;
    if (profileLobbyWidget_) {
        setup = profileLobbyWidget_->duelConfig();
    } else {
        setup.opponentMode = QStringLiteral("Manual");
        setup.opponentCategory = QStringLiteral("Player");
        setup.selectedOpponent = QStringLiteral("Arcen");
        setup.selectedArena = QStringLiteral("Colosseum");
    }
    exhibitionSetupPage_->applySessionState(playerName, sortedPlayerTypes_, selectedPlayerType_, setup);

    saveKingSceneAction_ = SaveKingSceneAction::None;
    stack_->setCurrentWidget(exhibitionSetupPage_);
}

void MainWindow::showSettingsPage() {
    if (soundManager_) {
        soundManager_->playLobbyMusic();
    }
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->stop();
    }
    if (saveKingIntroPage_) {
        saveKingIntroPage_->stopScene();
    }
    saveKingSceneAction_ = SaveKingSceneAction::None;
    stack_->setCurrentWidget(settingsPage_);
}

void MainWindow::syncExhibitionSelectionToLobby() {
    if (!exhibitionSetupPage_) {
        return;
    }

    const PlayerType pageSelection = exhibitionSetupPage_->selectedPlayerType();
    selectedPlayerType_ = pageSelection;
    for (int i = 0; i < static_cast<int>(sortedPlayerTypes_.size()); ++i) {
        if (sortedPlayerTypes_[static_cast<size_t>(i)] == pageSelection) {
            currentCharacterIndex_ = i;
            break;
        }
    }

    if (profileLobbyWidget_) {
        ProfileLobbyWidget::Character updated;
        updated.name = QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_));
        updated.imagePath = characterImagePath(selectedPlayerType_);
        updated.specialMoves = playerSpecialMoveText(selectedPlayerType_);
        profileLobbyWidget_->setSelectedCharacter(updated);
        profileLobbyWidget_->setSelectedMode(QStringLiteral("1v1 Exhibition"));
        profileLobbyWidget_->setDuelSetup(exhibitionSetupPage_->duelSetup());
    }

    if (lanArenaPage_) {
        lanArenaPage_->setIdentity(
            currentLobbyUsername_.trimmed().isEmpty() ? QStringLiteral("Player_01") : currentLobbyUsername_,
            QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_)),
            selectedPlayerType_);
    }
}

void MainWindow::attemptLogin() {
    if (!loginUsernameEdit_ || !loginPasswordEdit_) {
        return;
    }

    const QString identity = loginUsernameEdit_->text().trimmed();
    const QString password = loginPasswordEdit_->text();

    if (identity.isEmpty() || password.isEmpty()) {
        if (soundManager_) {
            soundManager_->playUIError();
        }
        QMessageBox::warning(this, "Login", "Please enter username/email and password.");
        return;
    }

    QString resolvedUsername;
    QString errorMessage;
    if (!databaseManager_ || !databaseManager_->authenticateUser(identity, password, &resolvedUsername, &errorMessage)) {
        if (soundManager_) {
            soundManager_->playUIError();
        }
        QMessageBox::warning(this,
                             "Login",
                             errorMessage.isEmpty()
                                 ? QStringLiteral("Invalid username/email or password.")
                                 : errorMessage);
        return;
    }

    setLoggedInUsername(resolvedUsername);
    refreshProfile();
    loginPasswordEdit_->clear();
    if (soundManager_) {
        soundManager_->playUIConfirm();
    }
    showSetupPage();
}

void MainWindow::attemptRegistration() {
    if (!regEmailEdit_ || !regUsernameEdit_ || !regPasswordEdit_ || !regPasswordRepeatEdit_) {
        return;
    }

    const QString email = regEmailEdit_->text().trimmed();
    const QString username = regUsernameEdit_->text().trimmed();
    const QString password = regPasswordEdit_->text();
    const QString passwordRepeat = regPasswordRepeatEdit_->text();

    if (email.isEmpty() || username.isEmpty() || password.isEmpty() || passwordRepeat.isEmpty()) {
        if (soundManager_) {
            soundManager_->playUIError();
        }
        QMessageBox::warning(this, "Registration", "Please fill in all fields.");
        return;
    }

    if (password != passwordRepeat) {
        if (soundManager_) {
            soundManager_->playUIError();
        }
        QMessageBox::warning(this, "Registration", "Passwords do not match.");
        return;
    }

    QString errorMessage;
    if (!databaseManager_ || !databaseManager_->registerUser(email, username, password, &errorMessage)) {
        if (soundManager_) {
            soundManager_->playUIError();
        }
        QMessageBox::warning(this,
                             "Registration",
                             errorMessage.isEmpty()
                                 ? QStringLiteral("Could not create the account.")
                                 : errorMessage);
        return;
    }

    loginUsernameEdit_->setText(username);
    loginPasswordEdit_->clear();
    regEmailEdit_->clear();
    regUsernameEdit_->clear();
    regPasswordEdit_->clear();
    regPasswordRepeatEdit_->clear();
    if (soundManager_) {
        soundManager_->playUIConfirm();
    }
    showLoginPage();
    QMessageBox::information(this, "Registration", "Account created. You can log in now.");
}

void MainWindow::startDemo() {
    // Campaign flow:
    // Start the run immediately, but restore the story intro handoff so the
    // king kidnapping scene plays before the first battle begins.
    QString playerName = currentLobbyUsername_.trimmed();
    if (profileLobbyWidget_) {
        playerName = profileLobbyWidget_->userProfile().username.trimmed();
    }
    if (playerName.isEmpty()) {
        playerName = "Player_01";
    }

    // Start the game with character type
    gameManager_->startGame(playerName.toStdString(), selectedPlayerType_);
    if (soundManager_) {
        soundManager_->playUIConfirm();
    }

    if (saveKingIntroPage_) {
        saveKingSceneAction_ = SaveKingSceneAction::StartCampaignBattle;
        saveKingIntroPage_->setSelectedGladiator(
            selectedPlayerType_,
            QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_)));
        saveKingIntroPage_->startIntroScene();
        stack_->setCurrentWidget(saveKingIntroPage_);
        return;
    }

    if (soundManager_) {
        soundManager_->playBattleMusic();
    }

    stack_->setCurrentWidget(battlePage_);
    if (gamePage_) {
        gamePage_->startBattle();
    }
}

void MainWindow::startDuelMode() {
    if (stack_ && exhibitionSetupPage_ && stack_->currentWidget() == exhibitionSetupPage_) {
        syncExhibitionSelectionToLobby();
    }

    QString playerName = currentLobbyUsername_.trimmed();
    if (profileLobbyWidget_) {
        playerName = profileLobbyWidget_->userProfile().username.trimmed();
    }

    if (playerName.isEmpty()) {
        playerName = QStringLiteral("Player_01");
    }

    DuelConfig config;
    ProfileLobbyWidget::DuelSetup setup;
    if (exhibitionSetupPage_ && stack_ && stack_->currentWidget() == exhibitionSetupPage_) {
        setup = exhibitionSetupPage_->duelSetup();
        selectedPlayerType_ = exhibitionSetupPage_->selectedPlayerType();
    } else if (profileLobbyWidget_) {
        setup = profileLobbyWidget_->duelConfig();
    }

    if (!setup.opponentMode.trimmed().isEmpty() || !setup.opponentCategory.trimmed().isEmpty()) {
        config.opponentMode = setup.opponentMode.compare(QStringLiteral("Manual"), Qt::CaseInsensitive) == 0
            ? DuelOpponentMode::MANUAL
            : DuelOpponentMode::RANDOM;
        config.category = setup.opponentCategory.compare(QStringLiteral("Player"), Qt::CaseInsensitive) == 0
            ? DuelOpponentCategory::PLAYER_TYPE
            : DuelOpponentCategory::ENEMY_TYPE;
        config.selectedArena = setup.selectedArena.trimmed().isEmpty()
            ? QStringLiteral("Default")
            : setup.selectedArena.trimmed();

        if (config.category == DuelOpponentCategory::PLAYER_TYPE) {
            config.manualPlayerOpponent = playerTypeFromName(setup.selectedOpponent);
        } else {
            config.manualEnemyOpponent = enemyTypeFromName(setup.selectedOpponent);
        }
    }

    gameManager_->startDuel(playerName.toStdString(), selectedPlayerType_, config);
    if (soundManager_) {
        soundManager_->playUIConfirm();
        soundManager_->playBattleMusic();
    }
    stack_->setCurrentWidget(battlePage_);

    if (gamePage_) {
        gamePage_->startBattle();
    }
}

void MainWindow::handleBattleFinished() {
    if (!gamePage_ || !databaseManager_ || !gameManager_) {
        return;
    }

    QString playerName = currentLobbyUsername_.trimmed();
    if (profileLobbyWidget_) {
        playerName = profileLobbyWidget_->userProfile().username.trimmed();
    }
    if (playerName.isEmpty()) {
        playerName = QStringLiteral("Player_01");
    }

    ChronicleBattleReport report = gamePage_->lastChronicleReport();
    if (report.playerName.trimmed().isEmpty()) {
        report.playerName = playerName;
    }

    const bool isDuelMatch = gameManager_->isDuelMode();
    const bool isLanDuel = gameManager_->isLanDuel();
    int reward = 0;
    applyBattleProgression(playerName, report, isDuelMatch, &reward);

    if (gameOverPage_) {
        gameOverPage_->setSummaryTheme(
            isLanDuel
                ? GameOverPage::SummaryTheme::LanDuel
                : (isDuelMatch ? GameOverPage::SummaryTheme::ExhibitionDuel
                               : GameOverPage::SummaryTheme::SaveTheKing));
        gameOverPage_->setResult(report.victory);
        gameOverPage_->setBattleStats(report.damageDealt, report.damageTaken, reward);
    }

    refreshProfile();

    if (!playerName.isEmpty()) {
        databaseManager_->saveResult(playerName.toStdString(), report.currentScore);
    }

    uploadBattleResult(report, isDuelMatch);
    showGameOverPage();
}

void MainWindow::handleChronicleRequested(int completedLevel, bool campaignComplete) {
    if (!chroniclePage_ || !gamePage_) {
        return;
    }

    ChronicleBattleReport report = gamePage_->lastChronicleReport();
    const QString playerName = currentLobbyUsername_.trimmed().isEmpty()
        ? report.playerName
        : currentLobbyUsername_.trimmed();

    chronicleCampaignComplete_ = campaignComplete;
    chroniclePage_->configure(
        completedLevel,
        report.totalLevels > 0 ? report.totalLevels : (gameManager_ ? gameManager_->getTotalLevels() : completedLevel),
        selectedPlayerType_,
        playerName,
        report);
    stack_->setCurrentWidget(chroniclePage_);
    chroniclePage_->startChronicle();
}

void MainWindow::continueAfterChronicle() {
    if (!gamePage_) {
        return;
    }

    if (!chronicleCampaignComplete_) {
        stack_->setCurrentWidget(battlePage_);
        gamePage_->startBattle();
        return;
    }

    ChronicleBattleReport report = gamePage_->lastChronicleReport();
    QString playerName = report.playerName.trimmed();
    if (playerName.isEmpty()) {
        playerName = currentLobbyUsername_.trimmed();
    }
    if (playerName.isEmpty()) {
        playerName = QStringLiteral("Player_01");
    }

    int reward = 0;
    applyBattleProgression(playerName, report, false, &reward);

    if (gameOverPage_) {
        gameOverPage_->setSummaryTheme(GameOverPage::SummaryTheme::SaveTheKing);
        gameOverPage_->setResult(report.victory);
        gameOverPage_->setBattleStats(report.damageDealt, report.damageTaken, reward);
    }

    refreshProfile();

    if (!playerName.isEmpty()) {
        databaseManager_->saveResult(playerName.toStdString(), report.currentScore);
    }

    uploadBattleResult(report, false);

    if (saveKingIntroPage_) {
        saveKingSceneAction_ = SaveKingSceneAction::ShowVictoryResult;
        saveKingIntroPage_->setSelectedGladiator(
            selectedPlayerType_,
            QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_)));
        saveKingIntroPage_->startRescueEndingScene();
        stack_->setCurrentWidget(saveKingIntroPage_);
        return;
    }

    showGameOverPage();
}

void MainWindow::handleSaveKingSceneFinished() {
    switch (saveKingSceneAction_) {
        case SaveKingSceneAction::ShowVictoryResult:
            showGameOverPage();
            break;
        case SaveKingSceneAction::StartCampaignBattle:
            saveKingSceneAction_ = SaveKingSceneAction::None;
            if (soundManager_) {
                soundManager_->playBattleMusic();
            }
            stack_->setCurrentWidget(battlePage_);
            if (gamePage_) {
                gamePage_->startBattle();
            }
            break;
        case SaveKingSceneAction::None:
        default:
            break;
    }
}

void MainWindow::restartDemo() {
    showSetupPage();
}

void MainWindow::refreshProfile() {
    const QString playerName = currentLobbyUsername_.trimmed();
    const PlayerProgression stats = playerName.isEmpty()
        ? databaseManager_->loadProgression()
        : databaseManager_->loadProgressionForUser(playerName);

    UserProfileRecord userRecord;
    const bool hasUserRecord = !playerName.isEmpty()
        && databaseManager_->getUserProfile(playerName, &userRecord);

    if (profileLobbyWidget_) {
        ProfileLobbyWidget::UserProfile profile = profileLobbyWidget_->userProfile();
        if (!playerName.isEmpty()) {
            profile.username = playerName;
        }
        profile.score = stats.totalScore;
        if (hasUserRecord) {
            profile.badge = userRecord.badge;
            profile.avatarPath = userRecord.avatarPath;
        }
        profileLobbyWidget_->setUserProfile(profile);
        profileLobbyWidget_->updateProgression(stats);
    }

    if (auto* pPage = qobject_cast<ProfilePage*>(profilePage_)) {
        pPage->setUsername(playerName);
        pPage->updateProgression(stats);
    }

    updateHighScores();
}

void MainWindow::refreshBattleView() {
    if (battleTitleLabel_) {
        battleTitleLabel_->setText(QString::fromStdString(gameManager_->getBattleTitle()));
    }

    if (battleWidget_) {
        battleWidget_->update();
    }
}

void MainWindow::showGameOverPage() {
    if (pausePage_) {
        pausePage_->hide();
    }
    if (saveKingIntroPage_) {
        saveKingIntroPage_->stopScene();
    }
    saveKingSceneAction_ = SaveKingSceneAction::None;
    if (gameOverSummaryLabel_) {
        int finalScore = gameManager_->getCurrentScore();
        gameOverSummaryLabel_->setText(QString("Battle Score: %1").arg(finalScore));
    }
    stack_->setCurrentWidget(gameOverPage_);
}

void MainWindow::updateHighScores() {
    if (!scoreList_) return;

    scoreList_->clear();
    auto leaderboard = databaseManager_->getTopScores();
    for (const auto& score : leaderboard) {
        QString item = QString("%1 - Score: %2")
            .arg(QString::fromStdString(score.playerName))
            .arg(score.score);
        scoreList_->addItem(item);
    }
}

void MainWindow::uploadBattleResult(const ChronicleBattleReport& report, bool isDuelMatch) const {
    if (!websiteSyncClient_ || !websiteSyncClient_->isConfigured()) {
        return;
    }

    QString username = report.playerName.trimmed();
    if (username.isEmpty()) {
        username = currentLobbyUsername_.trimmed();
    }
    if (username.isEmpty()) {
        return;
    }

    WebsiteBattleUpload payload;
    payload.username = username;
    payload.characterType = playerTypeApiKey(selectedPlayerType_);
    payload.characterName = QString::fromStdString(InputHandler::playerTypeToDisplayName(selectedPlayerType_));
    payload.mode = !isDuelMatch
        ? QStringLiteral("save_the_king")
        : (gameManager_ && gameManager_->isLanDuel() ? QStringLiteral("lan_duel") : QStringLiteral("exhibition_duel"));
    payload.levelIndex = report.completedLevel > 0 ? report.completedLevel : (gameManager_ ? gameManager_->getCurrentLevel() : 0);
    payload.levelName = report.defeatedEnemyName.trimmed();
    payload.enemyType = QString();
    payload.enemyName = report.defeatedEnemyName.trimmed();
    payload.opponentUsername = QString::fromStdString(gameManager_ ? gameManager_->getLanOpponentName() : std::string());
    payload.victory = report.victory;
    payload.score = report.currentScore;
    payload.damageDealt = report.damageDealt;
    payload.damageTaken = report.damageTaken;
    payload.playerHpEnd = report.playerHp;
    payload.playerMaxHp = report.playerMaxHp;
    payload.battleDurationSeconds = report.battleDurationSeconds;
    payload.campaignComplete = report.campaignComplete;

    if (isDuelMatch && gameManager_ && gameManager_->isLanDuel()) {
        payload.levelIndex = 1;
        payload.levelName = QStringLiteral("Arena Link Duel");
        payload.enemyType = playerTypeApiKey(gameManager_ ? gameManager_->getLanOpponentPlayerType() : PlayerType::KNIGHT);
        payload.enemyName = payload.opponentUsername.trimmed().isEmpty()
            ? QStringLiteral("Linked Rival")
            : payload.opponentUsername.trimmed();
    } else if (isDuelMatch) {
        payload.levelIndex = 1;
        payload.levelName = QStringLiteral("1v1 Exhibition");
        if (gameManager_ && gameManager_->getDuelConfig().category == DuelOpponentCategory::PLAYER_TYPE) {
            payload.enemyType = playerTypeApiKey(gameManager_->getLanOpponentPlayerType());
        }
    } else if (gameManager_ && gameManager_->getCurrentEnemy()) {
        const Enemy* currentEnemy = gameManager_->getCurrentEnemy();
        payload.enemyType = enemyTypeApiKey(currentEnemy->getEnemyType());
        if (payload.enemyName.isEmpty()) {
            payload.enemyName = QString::fromStdString(currentEnemy->getName());
        }
    }

    if (payload.levelName.isEmpty()) {
        payload.levelName = isDuelMatch ? QStringLiteral("1v1 Exhibition") : QStringLiteral("Save the King");
    }

    websiteSyncClient_->uploadBattleResult(payload);
}

PlayerProgression MainWindow::applyBattleProgression(const QString& username,
                                                     const ChronicleBattleReport& report,
                                                     bool isDuelMatch,
                                                     int* outReward) {
    if (!databaseManager_) {
        return PlayerProgression();
    }

    PlayerProgression stats = username.trimmed().isEmpty()
        ? databaseManager_->loadProgression()
        : databaseManager_->loadProgressionForUser(username);

    const bool fullClear = !isDuelMatch && report.victory && report.campaignComplete;
    const int stagesCleared = isDuelMatch
        ? 1
        : (report.victory
               ? qMax(1, report.completedLevel)
               : qMax(0, report.completedLevel - 1));
    const RunMode mode = isDuelMatch ? RunMode::DUEL : RunMode::CAMPAIGN;
    const int reward = GameManager::calculateRewardForMatch(mode, report.victory, stagesCleared, fullClear);

    stats.totalScore += reward;
    stats.totalMatches += 1;
    if (report.victory) {
        stats.wins += 1;
    } else {
        stats.losses += 1;
    }
    stats.currentRank = GameManager::calculateRankFromScore(stats.totalScore);
    stats.currentRating = GameManager::calculateRatingFromStats(stats.wins, stats.totalMatches);

    QString errorMessage;
    if (username.trimmed().isEmpty()
        || !databaseManager_->saveProgressionForUser(username, stats, &errorMessage)) {
        databaseManager_->saveProgression(stats);
    }

    if (outReward) {
        *outReward = reward;
    }
    return stats;
}

QString MainWindow::stateTitle() const {
    if (!gameManager_) return "Unknown";
    
    // Map game state to string
    // This would depend on GameManager implementation
    return "Idle";
}
