#include "GamePage.h"
#include "BattleWidget.h"
#include "ChronicleAiAdvisor.h"
#include "GameManager.h"
#include "LanSessionManager.h"
#include "Player.h"
#include "Enemy.h"
#include <QVBoxLayout>
#include <QLabel>

GamePage::GamePage(QWidget *parent)
    : QWidget(parent),
      gameManager_(nullptr),
      soundManager_(nullptr),
      chronicleAiAdvisor_(new ChronicleAiAdvisor(this)),
      pendingChronicleLevel_(0),
      pendingChronicleCampaignComplete_(false) {
    
    setStyleSheet("QWidget { background-color: #22140D; }");
    setupUI();
}

GamePage::~GamePage() {}

void GamePage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    playerInfoLabel_ = nullptr;
    instructionsLabel_ = nullptr;

    battleWidget_ = new BattleWidget();
    mainLayout->addWidget(battleWidget_, 1);

    setLayout(mainLayout);
}

void GamePage::setGameManager(GameManager *gm) {
    gameManager_ = gm;

    if (battleWidget_) {
        battleWidget_->setGameManager(gm);
    }

    updateStats();

    if (battleWidget_) {
        connect(battleWidget_, &BattleWidget::battleFinished, this, &GamePage::onBattleFinished, Qt::UniqueConnection);
        connect(battleWidget_, &BattleWidget::levelTransitionFinished, this, &GamePage::onLevelTransitionFinished,
                Qt::UniqueConnection);
        connect(battleWidget_, &BattleWidget::pauseRequested, this, &GamePage::pauseRequested, Qt::UniqueConnection);
    }
}

void GamePage::setLanSessionManager(LanSessionManager *manager) {
    if (battleWidget_) {
        battleWidget_->setLanSessionManager(manager);
    }
}

void GamePage::setSoundManager(SoundManager *sm) {
    // Sound teammate:
    // SoundManager passes through this page into BattleWidget.
    soundManager_ = sm;
    
    if (battleWidget_) {
        battleWidget_->setSoundManager(sm);
    }
}

void GamePage::startBattle() {
    // LAN teammate:
    // Same battle handling is reused for Arena Link duel mode.
    if (battleWidget_) {
        battleWidget_->startBattle();
    }
    updateStats();
}

void GamePage::pauseBattle() {
    if (battleWidget_) {
        battleWidget_->pauseBattle();
    }
}

void GamePage::resumeBattle() {
    if (battleWidget_) {
        battleWidget_->resumeBattle();
    }
}

bool GamePage::hasActiveBattle() const {
    return battleWidget_ && battleWidget_->isBattleRunning();
}

ChronicleBattleReport GamePage::lastChronicleReport() const {
    return pendingChronicleReport_;
}

std::optional<CombatHighlightSnapshot> GamePage::lastBattleHighlight() const {
    return pendingBattleHighlight_;
}

void GamePage::onLevelTransitionFinished() {
    if (pendingChronicleLevel_ > 0) {
        emit chronicleRequested(pendingChronicleLevel_, pendingChronicleCampaignComplete_);
        pendingChronicleLevel_ = 0;
        pendingChronicleCampaignComplete_ = false;
        return;
    }

    startBattle();
}

void GamePage::updateStats() {
    if (!gameManager_ || !playerInfoLabel_) return;

    const Player *player = gameManager_->getPlayer();
    if (!player) return;

    QString info = QString("Player: %1 | Level: %2 | Score: %3")
        .arg(QString::fromStdString(player->getName()))
        .arg(gameManager_->getPlayerLevel())
        .arg(gameManager_->getCurrentScore());

    playerInfoLabel_->setText(info);
}

void GamePage::onBattleFinished() {
    // LAN teammate:
    // Save the Kings can still advance levels here.
    // Duel mode should stop after this one result and return to the normal finish flow.
    // Sound teammate:
    // Good place for victory/defeat/level-clear sounds before page flow continues.
    battleWidget_->stopBattle();
    
    const Player *player = gameManager_->getPlayer();
    const Enemy *enemy = gameManager_->getCurrentEnemy();
    
    if (!player || !enemy) return;

    battleWidget_->finalizeHighlightClip();
    pendingChronicleReport_ = battleWidget_->levelBattleReport();
    pendingBattleHighlight_ = battleWidget_->levelBattleHighlight();
    pendingChronicleReport_.completedLevel = gameManager_->getCurrentLevel();
    pendingChronicleReport_.totalLevels = gameManager_->getTotalLevels();
    pendingChronicleReport_.currentScore = gameManager_->getCurrentScore();
    pendingChronicleReport_.victory = gameManager_->isLanDuel()
        ? pendingChronicleReport_.victory
        : (player->isAlive() && !enemy->isAlive());
    pendingChronicleReport_.campaignComplete = false;

    if (gameManager_->isLanDuel()) {
        pendingChronicleReport_.campaignComplete = false;
        pendingChronicleReport_.completedLevel = 1;
        pendingChronicleReport_.totalLevels = 1;
        gameManager_->addScore(pendingChronicleReport_.victory ? 140 : 45);
        pendingChronicleReport_.currentScore = gameManager_->getCurrentScore();

        if (playerInfoLabel_) {
            playerInfoLabel_->setText(pendingChronicleReport_.victory
                ? "Arena Link duel won."
                : "Arena Link duel lost.");
        }

        updateStats();
        emit battleFinished();
        return;
    }

    if (gameManager_->isDuelMode()) {
        pendingChronicleReport_.victory = player->isAlive() && !enemy->isAlive();
        pendingChronicleReport_.campaignComplete = false;
        pendingChronicleReport_.completedLevel = 1;
        pendingChronicleReport_.totalLevels = 1;
        gameManager_->addScore(pendingChronicleReport_.victory ? 140 : 45);
        pendingChronicleReport_.currentScore = gameManager_->getCurrentScore();

        if (playerInfoLabel_) {
            playerInfoLabel_->setText(pendingChronicleReport_.victory
                ? "Exhibition duel won."
                : "Exhibition duel lost.");
        }

        updateStats();
        emit battleFinished();
        return;
    }

    if (gameManager_->isZombieMode()) {
        const bool advancedLevelPrepared = player->isAlive()
            && !gameManager_->hasCompletedCampaign()
            && gameManager_->getCurrentLevel() == 2;
        pendingChronicleReport_.victory = player->isAlive() && (advancedLevelPrepared || gameManager_->hasCompletedCampaign());
        pendingChronicleReport_.campaignComplete = player->isAlive() && gameManager_->hasCompletedCampaign();
        pendingChronicleReport_.completedLevel = advancedLevelPrepared ? 1 : gameManager_->getCurrentLevel();
        pendingChronicleReport_.totalLevels = gameManager_->getTotalLevels();
        pendingChronicleReport_.currentScore = gameManager_->getCurrentScore();
        pendingChronicleReport_.nextEnemyName = pendingChronicleReport_.victory
            ? QStringLiteral("Clean city")
            : (advancedLevelPrepared ? QStringLiteral("Advanced zombies") : QStringLiteral("Infected streets"));
        pendingChronicleReport_.nextEnemyType = QStringLiteral("Zombie Outbreak");

        if (playerInfoLabel_) {
            playerInfoLabel_->setText(pendingChronicleReport_.victory
                ? "Zombie outbreak cleared."
                : (advancedLevelPrepared
                       ? "First zombie wave cleared."
                       : "The zombie outbreak overran the city."));
        }

        if (advancedLevelPrepared) {
            if (chronicleAiAdvisor_) {
                chronicleAiAdvisor_->prefetchSummary(pendingChronicleReport_);
            }
            emit chronicleRequested(1, false);
            return;
        }

        updateStats();
        emit battleFinished();
        return;
    }

    if (player->isAlive()) {
        const int completedLevel = gameManager_->getCurrentLevel();
        gameManager_->addScore(50 + gameManager_->getCurrentLevel() * 15);
        const bool hasNextLevel = gameManager_->advanceToNextLevel();
        pendingChronicleReport_.completedLevel = completedLevel;
        pendingChronicleReport_.totalLevels = gameManager_->getTotalLevels();
        pendingChronicleReport_.currentScore = gameManager_->getCurrentScore();
        pendingChronicleReport_.victory = true;
        pendingChronicleReport_.campaignComplete = !hasNextLevel && gameManager_->hasCompletedCampaign();
        const Enemy *nextEnemy = gameManager_->getCurrentEnemy();
        if (nextEnemy && hasNextLevel) {
            pendingChronicleReport_.nextEnemyName = QString::fromStdString(nextEnemy->getName());
            pendingChronicleReport_.nextEnemyType = pendingChronicleReport_.nextEnemyName;
        } else {
            pendingChronicleReport_.nextEnemyName = "The saved king";
            pendingChronicleReport_.nextEnemyType = "Rescue ending";
        }
        pendingChronicleLevel_ = completedLevel;
        pendingChronicleCampaignComplete_ = !hasNextLevel && gameManager_->hasCompletedCampaign();
        if (chronicleAiAdvisor_) {
            chronicleAiAdvisor_->prefetchSummary(pendingChronicleReport_);
        }

        battleWidget_->startLevelTransition();
        return;
    } else {
        if (playerInfoLabel_) {
            playerInfoLabel_->setText("Defeat! Press ESC to continue...");
        }
    }
    
    updateStats();
    emit battleFinished();
}
