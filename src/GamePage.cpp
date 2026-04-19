#include "GamePage.h"
#include "BattleWidget.h"
#include "GameManager.h"
#include "Player.h"
#include "Enemy.h"
#include <QVBoxLayout>
#include <QLabel>

GamePage::GamePage(QWidget *parent)
    : QWidget(parent),
      gameManager_(nullptr),
      soundManager_(nullptr) {
    
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
        connect(battleWidget_, &BattleWidget::battleFinished, this, &GamePage::onBattleFinished);
    }
}

void GamePage::setSoundManager(SoundManager *sm) {
    soundManager_ = sm;
    
    if (battleWidget_) {
        battleWidget_->setSoundManager(sm);
    }
}

void GamePage::startBattle() {
    if (battleWidget_) {
        battleWidget_->startBattle();
    }
    updateStats();
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
    battleWidget_->stopBattle();
    
    const Player *player = gameManager_->getPlayer();
    const Enemy *enemy = gameManager_->getCurrentEnemy();
    
    if (!player || !enemy) return;
    
    if (player->isAlive()) {
        if (playerInfoLabel_) {
            playerInfoLabel_->setText("Victory! Press ESC to continue...");
        }
        gameManager_->addScore(50 + gameManager_->getPlayerLevel() * 10);
    } else {
        if (playerInfoLabel_) {
            playerInfoLabel_->setText("Defeat! Press ESC to continue...");
        }
    }
    
    updateStats();
    emit battleFinished();
}
