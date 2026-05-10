#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include <string>
#include <memory>
#include <QString>

#include "Enums.h"

class Player;
class Enemy;

enum class RunMode {
    CAMPAIGN,
    DUEL
};

enum class DuelOpponentMode {
    RANDOM,
    MANUAL
};

enum class DuelOpponentCategory {
    PLAYER_TYPE,
    ENEMY_TYPE
};

struct DuelConfig {
    DuelOpponentMode opponentMode = DuelOpponentMode::RANDOM;
    DuelOpponentCategory category = DuelOpponentCategory::ENEMY_TYPE;

    PlayerType manualPlayerOpponent = PlayerType::KNIGHT;
    EnemyType manualEnemyOpponent = EnemyType::FIRE_WORM;

    QString selectedArena = QStringLiteral("Default");
};

class GameManager : public QObject {
    Q_OBJECT

public:
    // LAN teammate:
    // Add duel-theme setup/state here.
    // GameManager should know whether the run is:
    // - Save the Kings campaign
    // - Arena Link single-match duel
    explicit GameManager(QObject *parent = nullptr);
    ~GameManager();

    // Ranking teammate:
    // GameManager should calculate battle rewards and expose enough result info
    // so MainWindow/DatabaseManager can update progression cleanly.
    void startGame(const std::string &playerName, PlayerType playerType);
    void startLanDuel(const std::string &playerName,
                      PlayerType playerType,
                      const std::string &opponentName,
                      PlayerType opponentType,
                      const std::string &arenaName);
    void startDuel(const std::string &playerName,
                   PlayerType playerType,
                   const DuelConfig& config);
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
    PlayerType getLanOpponentPlayerType() const;
    std::string getLanOpponentName() const;
    std::string getLanArenaName() const;
    bool hasCompletedCampaign() const;
    bool isLanDuel() const;
    bool isFinalKingStage() const;
    bool advanceFinalGuardianWave();
    
    Player* getPlayer() const;
    Enemy* getCurrentEnemy() const;
    RunMode getRunMode() const;
    DuelConfig getDuelConfig() const;
    bool isDuelMode() const;
    bool didWinDuel() const;
    void setDifficulty(DifficultyLevel difficulty);
    DifficultyLevel getDifficulty() const;

    // Ranking and Scoring Helpers
    static int calculateRewardForMatch(RunMode mode, bool victory, int stagesCleared, bool fullClear);
    static std::string calculateRankFromScore(int totalScore);
    static double calculateRatingFromStats(int totalWins, int totalMatches);

signals:
    void battleFinished();

private:
    // LAN teammate:
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
    PlayerType duelOpponentPlayerType_;
    std::string duelOpponentName_;
    std::string duelArenaName_;
    bool campaignCompleted_;
    bool lanDuelMode_;
    int finalGuardianIndex_;
    RunMode runMode_;
    DuelConfig duelConfig_;
    bool duelVictory_;
    DifficultyLevel difficulty_;
};

#endif // GAMEMANAGER_H
