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
#include <QListWidget>
#include <QStackedWidget>
#include <QWidget>
#include <QComboBox>
#include <QInputDialog>
#include <QMessageBox>
#include <QFont>
#include <QObject>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

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

QString characterImagePath(CharacterType type) {
    switch (type) {
        case CharacterType::ARCEN:
            return resolveAssetPath("assets/players/Arcen/image.png");
        case CharacterType::DEMON_SLAYER:
            return resolveAssetPath("assets/players/Demon_Slayer/image.png");
        case CharacterType::FANTASY_WARRIOR:
            return resolveAssetPath("assets/players/Fantasy_Warrior/image.png");
        case CharacterType::HUNTRESS:
            return resolveAssetPath("assets/players/Huntress/image.png");
        case CharacterType::KNIGHT:
            return resolveAssetPath("assets/players/Knight/image.png");
        case CharacterType::MARTIAL:
            return resolveAssetPath("assets/players/Martial/image.png");
        case CharacterType::MARTIAL_HERO:
            return resolveAssetPath("assets/players/Martial_Hero/image.png");
        case CharacterType::MEDIEVAL_WARRIOR:
            return resolveAssetPath("assets/players/Medieval_Warrior/image.png");
        case CharacterType::WIZARD:
            return resolveAssetPath("assets/players/Wizard/image.png");
        default:
            return QString();
    }
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
      gameManager_(nullptr),
      databaseManager_(nullptr),
      soundManager_(nullptr),
    currentLobbyUsername_("Player_01"),
    currentCharacterIndex_(0),
      selectedCharacterType_(CharacterType::KNIGHT) {
    setWindowTitle("Battle Arena");
    setWindowState(Qt::WindowMaximized);
    setStyleSheet("QMainWindow { background-color: #3D2817; color: #F5E6D3; }");
    
    // Initialize managers
    databaseManager_ = new DatabaseManager();
    gameManager_ = new GameManager();
    // soundManager_ = new SoundManager(); // Disabled for now
    // soundManager_->initialize();
    
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
    connect(gameOverPage_, &GameOverPage::backToMenu, this, &MainWindow::showWelcomePage);
    connect(pausePage_, &PausePage::resumeClicked, this, &MainWindow::showWelcomePage);
    connect(pausePage_, &PausePage::menuClicked, this, &MainWindow::showWelcomePage);
    connect(pausePage_, &PausePage::settingsClicked, this, &MainWindow::showSettingsPage);
    connect(leaderboardPage_, &LeaderboardPage::backClicked, this, &MainWindow::showWelcomePage);
    connect(settingsPage_, &SettingsPage::backClicked, this, &MainWindow::showWelcomePage);

    // Show integrated login page initially
    showLoginPage();
}

QWidget* MainWindow::createLoginPage() {
    QWidget *page = new QWidget();
    page->setStyleSheet(
        "QWidget { background-color: #1A140F; color: #F5E6D3; }"
        "QLabel { color: #D4A017; }"
        "QLineEdit { background-color: #2C1F14; color: #F5E6D3; border: 1px solid #7C5A24; border-radius: 8px; padding: 10px; }"
        "QLineEdit:focus { border: 1px solid #D4A017; }"
        "QPushButton { background-color: #8B0000; color: #F5E6D3; border: 2px solid #D4A017; border-radius: 10px; padding: 10px 18px; font-weight: bold; }"
        "QPushButton:hover { background-color: #A50000; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addStretch();

    QWidget *panel = new QWidget(page);
    panel->setMaximumWidth(520);
    panel->setStyleSheet("QWidget { background-color: #2C1F14; border: 1px solid #5A3D1E; border-radius: 14px; }");
    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(28, 28, 28, 28);
    panelLayout->setSpacing(16);

    QLabel *title = new QLabel("Battle Arena Login", panel);
    QFont titleFont = title->font();
    titleFont.setPointSize(22);
    titleFont.setBold(true);
    title->setFont(titleFont);
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
    connect(loginButton, &QPushButton::clicked, this, &MainWindow::attemptLogin);
    panelLayout->addWidget(loginButton);

    QPushButton *registerButton = new QPushButton("Create Account", panel);
    registerButton->setStyleSheet(
        "QPushButton { background-color: #5C4033; color: #F5E6D3; border: 1px solid #D4A017; border-radius: 10px; padding: 10px 18px; font-weight: bold; }"
        "QPushButton:hover { background-color: #725240; }"
    );
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::showRegistrationPage);
    panelLayout->addWidget(registerButton);

    layout->addWidget(panel, 0, Qt::AlignHCenter);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::createRegistrationPage() {
    QWidget *page = new QWidget();
    page->setStyleSheet(
        "QWidget { background-color: #1A140F; color: #F5E6D3; }"
        "QLabel { color: #D4A017; }"
        "QLineEdit { background-color: #2C1F14; color: #F5E6D3; border: 1px solid #7C5A24; border-radius: 8px; padding: 10px; }"
        "QLineEdit:focus { border: 1px solid #D4A017; }"
        "QPushButton { background-color: #8B0000; color: #F5E6D3; border: 2px solid #D4A017; border-radius: 10px; padding: 10px 18px; font-weight: bold; }"
        "QPushButton:hover { background-color: #A50000; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addStretch();

    QWidget *panel = new QWidget(page);
    panel->setMaximumWidth(560);
    panel->setStyleSheet("QWidget { background-color: #2C1F14; border: 1px solid #5A3D1E; border-radius: 14px; }");
    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(28, 28, 28, 28);
    panelLayout->setSpacing(14);

    QLabel *title = new QLabel("Create Battle Account", panel);
    QFont titleFont = title->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    panelLayout->addWidget(title);

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
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::attemptRegistration);
    panelLayout->addWidget(registerButton);

    QPushButton *backButton = new QPushButton("Back to Login", panel);
    backButton->setStyleSheet(
        "QPushButton { background-color: #5C4033; color: #F5E6D3; border: 1px solid #D4A017; border-radius: 10px; padding: 10px 18px; font-weight: bold; }"
        "QPushButton:hover { background-color: #725240; }"
    );
    connect(backButton, &QPushButton::clicked, this, &MainWindow::showLoginPage);
    panelLayout->addWidget(backButton);

    layout->addWidget(panel, 0, Qt::AlignHCenter);
    layout->addStretch();

    return page;
}

QWidget* MainWindow::createWelcomePage() {
    QWidget *page = new QWidget();
    page->setStyleSheet("QWidget { background-color: #2A1810; } QLabel { color: #D4AF37; } QPushButton { background-color: #8B0000; color: #F5E6D3; border: 2px solid #D4AF37; padding: 8px; font-weight: bold; } QPushButton:hover { background-color: #A50000; }");
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel("Welcome to Battle Arena");
    QFont font = titleLabel->font();
    font.setPointSize(24);
    font.setBold(true);
    titleLabel->setFont(font);
    layout->addWidget(titleLabel);

    QLabel *descLabel = new QLabel("Test your skills in epic battles!");
    layout->addWidget(descLabel);

    layout->addSpacing(30);

    QPushButton *playButton = new QPushButton("Play Game");
    connect(playButton, &QPushButton::clicked, this, &MainWindow::showSetupPage);
    layout->addWidget(playButton);

    layout->addStretch();
    return page;
}

QWidget* MainWindow::createSetupPage() {
    profileLobbyWidget_ = new ProfileLobbyWidget(this);

    sortedCharacterTypes_ = InputHandler::getCharactersSortedByFeatures();
    if (sortedCharacterTypes_.empty()) {
        sortedCharacterTypes_.push_back(CharacterType::KNIGHT);
    }

    currentCharacterIndex_ = 0;
    selectedCharacterType_ = sortedCharacterTypes_[currentCharacterIndex_];

    ProfileLobbyWidget::UserProfile profile;
    profile.username = currentLobbyUsername_;
    profile.score = 0;
    profile.badge = "Rookie";
    profile.avatarPath = QString();
    profileLobbyWidget_->setUserProfile(profile);

    ProfileLobbyWidget::Character character;
    character.name = QString::fromStdString(InputHandler::characterTypeToDisplayName(selectedCharacterType_));
    character.imagePath = characterImagePath(selectedCharacterType_);
    character.specialMoves = "Feature-based action set";
    profileLobbyWidget_->setSelectedCharacter(character);

    connect(profileLobbyWidget_, &ProfileLobbyWidget::enterArenaClicked, this, [this](const QString&) {
        startDemo();
    });

    connect(profileLobbyWidget_, &ProfileLobbyWidget::changeCharacterClicked, this, [this]() {
        if (sortedCharacterTypes_.empty()) {
            return;
        }
        currentCharacterIndex_ = (currentCharacterIndex_ + 1) % static_cast<int>(sortedCharacterTypes_.size());
        selectedCharacterType_ = sortedCharacterTypes_[currentCharacterIndex_];

        ProfileLobbyWidget::Character updated;
        updated.name = QString::fromStdString(InputHandler::characterTypeToDisplayName(selectedCharacterType_));
        updated.imagePath = characterImagePath(selectedCharacterType_);
        updated.specialMoves = "Feature-based action set";
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
    gamePage_ = new GamePage();
    gamePage_->setGameManager(gameManager_);
    // gamePage_->setSoundManager(soundManager_); // Disabled for now
    connect(gamePage_, &GamePage::battleFinished, this, &MainWindow::handleBattleFinished);
    return gamePage_;
}

void MainWindow::showWelcomePage() {
    stack_->setCurrentWidget(welcomePage_);
}

void MainWindow::showLoginPage() {
    stack_->setCurrentWidget(loginPage_);
}

void MainWindow::showRegistrationPage() {
    stack_->setCurrentWidget(registrationPage_);
}

void MainWindow::showSetupPage() {
    stack_->setCurrentWidget(setupPage_);
}

void MainWindow::showSettingsPage() {
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
    QString playerName = currentLobbyUsername_.trimmed();
    if (profileLobbyWidget_) {
        playerName = profileLobbyWidget_->userProfile().username.trimmed();
    }
    if (playerName.isEmpty()) {
        playerName = "Player_01";
    }

    // Start the game with character type
    gameManager_->startGame(playerName.toStdString(), selectedCharacterType_);
    stack_->setCurrentWidget(battlePage_);
    
    // Start the battle
    if (gamePage_) {
        gamePage_->startBattle();
    }
}

void MainWindow::handleBattleFinished() {
    QString playerName = currentLobbyUsername_;
    if (profileLobbyWidget_) {
        playerName = profileLobbyWidget_->userProfile().username;
    }

    const Player *player = gameManager_ ? gameManager_->getPlayer() : nullptr;
    const Enemy *enemy = gameManager_ ? gameManager_->getCurrentEnemy() : nullptr;
    const bool victory = player && player->isAlive();
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
