// MainWindow.cpp - Main application window implementation

#include "MainWindow.h"
#include "BattleWidget.h"
#include "DatabaseManager.h"
#include "GameManager.h"
#include "GamePage.h"
#include "GameOverPage.h"
#include "PausePage.h"
#include "LeaderboardPage.h"
#include "SettingsPage.h"
#include "ProfileLobbyWidget.h"
#include "ProfilePage.h"
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
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QEasingCurve>
#include <QTimer>
#include <QVariantAnimation>

namespace {
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
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      stack_(nullptr),
    loginPage_(nullptr),
    registrationPage_(nullptr),
      welcomePage_(nullptr),
      setupPage_(nullptr),
      profilePage_(nullptr),
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
      battleWidget_(nullptr),
      gamePage_(nullptr),
      battleTitleLabel_(nullptr),
      gameOverTitleLabel_(nullptr),
      gameOverSummaryLabel_(nullptr),
      welcomeTransitionTimer_(new QTimer(this)),
      gameManager_(nullptr),
      databaseManager_(nullptr),
      soundManager_(nullptr),
    currentLobbyUsername_("Player_01"),
    currentCharacterIndex_(0),
      selectedPlayerType_(PlayerType::KNIGHT) {
    setWindowTitle("Battle Arena");
    setWindowState(Qt::WindowMaximized);
    setStyleSheet("QMainWindow { background-color: #3D2817; color: #F5E6D3; }");
    
    // Initialize managers
    databaseManager_ = new DatabaseManager();
    gameManager_ = new GameManager();
    // Ranking teammate:
    // MainWindow is a good place to coordinate:
    // - reading stored progression
    // - updating progression after battle results
    // - refreshing lobby/profile display
    // Sound teammate:
    // Re-enable and initialize SoundManager here.
    // MainWindow is the best place to switch music for welcome/login/lobby/battle/game-over.
    // soundManager_ = new SoundManager(); // Disabled for now
    // soundManager_->initialize();

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
    // if (soundManager_) delete soundManager_; // Disabled for now
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
    profilePage_ = createProfilePage();
    battlePage_ = createBattlePage();
    gameOverPage_ = new GameOverPage();
    pausePage_ = new PausePage();
    leaderboardPage_ = new LeaderboardPage();
    leaderboardPage_->setDatabaseManager(databaseManager_);
    settingsPage_ = new SettingsPage();

    // Add pages to stacked widget
    stack_->addWidget(loginPage_);
    stack_->addWidget(registrationPage_);
    stack_->addWidget(welcomePage_);
    stack_->addWidget(setupPage_);
    stack_->addWidget(profilePage_);
    stack_->addWidget(battlePage_);
    stack_->addWidget(gameOverPage_);
    stack_->addWidget(pausePage_);
    stack_->addWidget(leaderboardPage_);
    stack_->addWidget(settingsPage_);

    // Connect page signals
    connect(gameOverPage_, &GameOverPage::playAgain, this, &MainWindow::startDemo);
    connect(gameOverPage_, &GameOverPage::backToMenu, this, &MainWindow::showSetupPage);
    connect(pausePage_, &PausePage::resumeClicked, this, [this]() {
        if (battlePage_) {
            stack_->setCurrentWidget(battlePage_);
        }
    });
    connect(pausePage_, &PausePage::menuClicked, this, &MainWindow::showSetupPage);
    connect(pausePage_, &PausePage::settingsClicked, this, &MainWindow::showSettingsPage);
    connect(leaderboardPage_, &LeaderboardPage::backClicked, this, &MainWindow::showSetupPage);
    connect(settingsPage_, &SettingsPage::backClicked, this, &MainWindow::showSetupPage);

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

    layout->addWidget(createLogoLabel(page, 170), 0, Qt::AlignHCenter);

    QFrame *panel = new QFrame(page);
    panel->setObjectName("authPanel");
    panel->setMaximumWidth(500);
    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(30, 30, 30, 30);
    panelLayout->setSpacing(16);

    QLabel *title = new QLabel("Login", panel);
    title->setObjectName("panelTitle");
    title->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(title);

    loginUsernameEdit_ = new QLineEdit(panel);
    loginUsernameEdit_->setPlaceholderText("Username");
    panelLayout->addWidget(loginUsernameEdit_);

    loginPasswordEdit_ = new QLineEdit(panel);
    loginPasswordEdit_->setPlaceholderText("Password");
    loginPasswordEdit_->setEchoMode(QLineEdit::Password);
    panelLayout->addWidget(loginPasswordEdit_);

    QPushButton *loginButton = new QPushButton("Login", panel);
    loginButton->setObjectName("primaryAction");
    connect(loginButton, &QPushButton::clicked, this, &MainWindow::attemptLogin);
    panelLayout->addWidget(loginButton);

    QPushButton *registerButton = new QPushButton("Create Account", panel);
    registerButton->setObjectName("secondaryAction");
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::showRegistrationPage);
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

    QPushButton *registerButton = new QPushButton("Register", panel);
    registerButton->setObjectName("primaryAction");
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::attemptRegistration);
    panelLayout->addWidget(registerButton);

    QPushButton *backButton = new QPushButton("Back to Login", panel);
    backButton->setObjectName("secondaryAction");
    connect(backButton, &QPushButton::clicked, this, &MainWindow::showLoginPage);
    panelLayout->addWidget(backButton);
    panelLayout->addStretch(1);

    layout->addWidget(panel, 4, Qt::AlignVCenter);
    return page;
}

QWidget* MainWindow::createWelcomePage() {
    QWidget *page = new QWidget();
    page->setStyleSheet(
        "QWidget { background-color:#0F0B09; }"
        "QFrame#introPanel {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 rgba(46,31,20,0.94), stop:1 rgba(20,14,11,0.98));"
        " border:1px solid rgba(212,160,23,0.22);"
        " border-radius:28px;"
        "}"
        "QLabel#introBody { color:rgba(245,230,184,0.80); font:15px 'Segoe UI'; }"
        "QFrame#introOrb {"
        " background:rgba(212,160,23,0.10);"
        " border:1px solid rgba(212,160,23,0.22);"
        " border-radius:34px;"
        "}"
    );

    QHBoxLayout *layout = new QHBoxLayout(page);
    layout->setContentsMargins(46, 40, 46, 40);
    layout->setSpacing(26);

    auto buildDecorationColumn = [page]() -> QWidget* {
        auto* column = new QWidget(page);
        auto* columnLayout = new QVBoxLayout(column);
        columnLayout->setContentsMargins(0, 0, 0, 0);
        columnLayout->setSpacing(24);
        columnLayout->addStretch(1);

        const QList<QSize> orbSizes = {QSize(68, 68), QSize(40, 40), QSize(86, 86)};
        for (const QSize& orbSize : orbSizes) {
            auto* orb = new QFrame(column);
            orb->setObjectName("introOrb");
            orb->setFixedSize(orbSize);

            auto* effect = new QGraphicsOpacityEffect(orb);
            effect->setOpacity(0.35);
            orb->setGraphicsEffect(effect);

            auto* animation = new QVariantAnimation(orb);
            animation->setStartValue(0.18);
            animation->setEndValue(0.62);
            animation->setDuration(1600 + orbSize.width() * 8);
            animation->setLoopCount(-1);
            animation->setEasingCurve(QEasingCurve::InOutSine);
            QObject::connect(animation, &QVariantAnimation::valueChanged, orb, [effect](const QVariant& value) {
                effect->setOpacity(value.toReal());
            });
            animation->start();

            columnLayout->addWidget(orb, 0, Qt::AlignHCenter);
        }

        columnLayout->addStretch(1);
        return column;
    };

    layout->addWidget(buildDecorationColumn(), 1);

    QFrame *panel = new QFrame(page);
    panel->setObjectName("introPanel");
    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(40, 40, 40, 40);
    panelLayout->setSpacing(16);
    panelLayout->addStretch(1);

    panelLayout->addWidget(createLogoLabel(panel, 170), 0, Qt::AlignCenter);

    QLabel *descLabel = new QLabel("Choose your fighter, climb through enemy stages, and enter hand-crafted battles one arena at a time.", panel);
    descLabel->setObjectName("introBody");
    descLabel->setWordWrap(true);
    descLabel->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(descLabel);
    panelLayout->addStretch(1);

    layout->addWidget(panel, 6, Qt::AlignCenter);
    layout->addWidget(buildDecorationColumn(), 1);
    return page;
}

QWidget* MainWindow::createSetupPage() {
    // 1v1 teammate:
    // Read duel setup from ProfileLobbyWidget here.
    // When 1v1 is selected, route into duel setup instead of showing "coming soon".
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
        // 1v1 teammate:
        // Replace the current 1v1 "coming soon" path.
        // Launch duel mode here using the selected opponent/background from the lobby.
        if (modeName.compare(QStringLiteral("Save the Kings"), Qt::CaseInsensitive) == 0) {
            startDemo();
            return;
        }

        QMessageBox::information(this,
                                 "Coming Soon",
                                 QString("%1 is coming soon.\n\nSelect Save the Kings to enter the current ready combat theme.")
                                     .arg(modeName));
    });

    connect(profileLobbyWidget_, &ProfileLobbyWidget::changeCharacterClicked, this, [this]() {
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
    });

    connect(profileLobbyWidget_, &ProfileLobbyWidget::usernameEditRequested, this, [this]() {
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
            currentLobbyUsername_ = entered.trimmed();
            ProfileLobbyWidget::UserProfile profile = profileLobbyWidget_->userProfile();
            profile.username = currentLobbyUsername_;
            profileLobbyWidget_->setUserProfile(profile);
        }
    });

    connect(profileLobbyWidget_, &ProfileLobbyWidget::settingsActionTriggered, this, [this](const QString& actionName) {
        if (actionName == "Settings") {
            showSettingsPage();
        } else if (actionName == "Crew") {
            stack_->setCurrentWidget(leaderboardPage_);
        }
    });

    return profileLobbyWidget_;
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
    // 1v1 teammate:
    // Reuse the same battle page for duel mode.
    // Do not build a separate combat page.
    gamePage_ = new GamePage();
    gamePage_->setGameManager(gameManager_);
    // gamePage_->setSoundManager(soundManager_); // Disabled for now
    connect(gamePage_, &GamePage::battleFinished, this, &MainWindow::handleBattleFinished);
    return gamePage_;
}

void MainWindow::showWelcomePage() {
    // Sound teammate:
    // Start welcome/intro music here.
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->start(3000);
    }
    stack_->setCurrentWidget(welcomePage_);
}

void MainWindow::showLoginPage() {
    // Sound teammate:
    // Switch to login/register music or stop intro music here.
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->stop();
    }
    stack_->setCurrentWidget(loginPage_);
}

void MainWindow::showRegistrationPage() {
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->stop();
    }
    stack_->setCurrentWidget(registrationPage_);
}

void MainWindow::showSetupPage() {
    // Sound teammate:
    // Start lobby music here.
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->stop();
    }
    stack_->setCurrentWidget(setupPage_);
}

void MainWindow::showSettingsPage() {
    if (welcomeTransitionTimer_) {
        welcomeTransitionTimer_->stop();
    }
    stack_->setCurrentWidget(settingsPage_);
}

void MainWindow::attemptLogin() {
    if (!loginUsernameEdit_ || !loginPasswordEdit_) {
        return;
    }

    const QString username = loginUsernameEdit_->text().trimmed();
    const QString password = loginPasswordEdit_->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Login", "Please enter username and password.");
        return;
    }

    setLoggedInUsername(username);
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
        QMessageBox::warning(this, "Registration", "Please fill in all fields.");
        return;
    }

    if (password != passwordRepeat) {
        QMessageBox::warning(this, "Registration", "Passwords do not match.");
        return;
    }

    loginUsernameEdit_->setText(username);
    loginPasswordEdit_->clear();
    showLoginPage();
    QMessageBox::information(this, "Registration", "Account created. Please login.");
}

void MainWindow::startDemo() {
    // 1v1 teammate:
    // This currently starts the campaign flow.
    // Either expand this to accept theme/setup data or add a parallel duel start path nearby.
    // Sound teammate:
    // Switch from lobby music to battle music here.
    QString playerName = currentLobbyUsername_.trimmed();
    if (profileLobbyWidget_) {
        playerName = profileLobbyWidget_->userProfile().username.trimmed();
    }
    if (playerName.isEmpty()) {
        playerName = "Player_01";
    }

    // Start the game with character type
    gameManager_->startGame(playerName.toStdString(), selectedPlayerType_);
    stack_->setCurrentWidget(battlePage_);
    
    // Start the battle
    if (gamePage_) {
        gamePage_->startBattle();
    }
}

void MainWindow::handleBattleFinished() {
    // Ranking teammate:
    // Update wins/losses, total score, rating, and rank around this battle-finished flow.
    QString playerName = currentLobbyUsername_;
    if (profileLobbyWidget_) {
        playerName = profileLobbyWidget_->userProfile().username;
    }

    const Player *player = gameManager_ ? gameManager_->getPlayer() : nullptr;
    const Enemy *enemy = gameManager_ ? gameManager_->getCurrentEnemy() : nullptr;
    const bool victory = player && player->isAlive() && gameManager_ && gameManager_->hasCompletedCampaign();
    const int damageDealt = enemy ? (enemy->getMaxHealth() - enemy->getHealth()) : 0;
    const int damageTaken = player ? (player->getMaxHealth() - player->getHealth()) : 0;
    const int finalScore = gameManager_ ? gameManager_->getCurrentScore() : 0;

    if (gameOverPage_) {
        gameOverPage_->setResult(victory);
        gameOverPage_->setBattleStats(damageDealt, damageTaken, finalScore);
    }

    showGameOverPage();

    if (!playerName.trimmed().isEmpty()) {
        databaseManager_->saveResult(playerName.trimmed().toStdString(), finalScore);
    }
}

void MainWindow::restartDemo() {
    showSetupPage();
}

void MainWindow::refreshProfile() {
    QString playerName = currentLobbyUsername_;
    if (profileLobbyWidget_) {
        playerName = profileLobbyWidget_->userProfile().username;
    }
    if (playerName.trimmed().isEmpty()) {
        return;
    }

    if (auto *profile = qobject_cast<ProfilePage*>(profilePage_)) {
        profile->setUsername(playerName);
        profile->setScore(gameManager_ ? gameManager_->getCurrentScore() : 0);

        const int level = gameManager_ ? gameManager_->getPlayerLevel() : 1;
        const QString rank = (level < 5) ? "Beginner" : (level < 10) ? "Intermediate" : "Veteran";
        profile->setRank(rank);
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

QString MainWindow::stateTitle() const {
    if (!gameManager_) return "Unknown";
    
    // Map game state to string
    // This would depend on GameManager implementation
    return "Idle";
}
