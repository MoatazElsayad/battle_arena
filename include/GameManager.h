#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include <string>
#include <memory>

#include "Enums.h"

class Player;
class Enemy;

class GameManager : public QObject {
    Q_OBJECT

public:
    // 1v1 teammate:
    // Add duel-theme setup/state here.
    // GameManager should know whether the run is:
    // - Save the Kings campaign
    // - 1v1 single-match duel
    explicit GameManager(QObject *parent = nullptr);
    ~GameManager();

    // Ranking teammate:
    // GameManager should calculate battle rewards and expose enough result info
    // so MainWindow/DatabaseManager can update progression cleanly.
    void startGame(const std::string &playerName, PlayerType playerType);
    bool advanceToNextLevel();
    void finishBattle();
    void addScore(int amount);

    int getCurrentScore() const;
    int getPlayerLevel() const;
    int getCurrentLevel() const;
    int getTotalLevels() const;
    std::string getBattleTitle() const;
    GameState getState() const;
    PlayerType getSelectedPlayerType() const;
    bool hasCompletedCampaign() const;
    
    Player* getPlayer() const;
    Enemy* getCurrentEnemy() const;

signals:
    void battleFinished();

private:
    // 1v1 teammate:
    // Duel support likely needs:
    // - selected opponent category (player/enemy)
    // - selected opponent type
    // - random/manual selection flag
    // - selected arena background
    // Keep this clean and reuse current combat setup.
    void spawnEnemyForCurrentLevel();

    std::string playerName_;
    int currentScore_;
    int playerLevel_;
    int currentLevel_;
    GameState state_;
    Player* player_;
    Enemy* currentEnemy_;
    PlayerType selectedPlayerType_;
    bool campaignCompleted_;
};

#endif // GAMEMANAGER_H
