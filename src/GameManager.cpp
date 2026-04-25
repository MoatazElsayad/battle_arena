#include "GameManager.h"
#include "Player.h"
#include "Enemy.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>

namespace {
struct PlayerStats {
    int hp;
    int attack;
};

struct EnemyDefinition {
    EnemyType type;
    const char *name;
    int hp;
    int attack;
    float speed;
};

constexpr std::array<EnemyDefinition, 5> kEnemyCampaign = {{
    {EnemyType::FIRE_WORM, "Fire Worm", 72, 10, 120.0f},
    {EnemyType::FIRE_WIZARD, "Fire Wizard", 88, 12, 128.0f},
    {EnemyType::FLYING_DEMON, "Flying Demon", 102, 14, 142.0f},
    {EnemyType::NIGHTWEAVER, "Nightweaver", 118, 17, 156.0f},
    {EnemyType::EVIL_WIZARD, "Evil Wizard", 145, 20, 165.0f},
}};

PlayerStats statsForPlayer(PlayerType type) {
    switch (type) {
        case PlayerType::KNIGHT:
        case PlayerType::MEDIEVAL_WARRIOR:
            return {120, 12};
        case PlayerType::WIZARD:
            return {70, 20};
        case PlayerType::ARCEN:
            return {80, 18};
        case PlayerType::DEMON_SLAYER:
            return {110, 16};
        case PlayerType::HUNTRESS:
            return {85, 17};
        case PlayerType::MARTIAL:
        case PlayerType::MARTIAL_HERO:
            return {100, 15};
        case PlayerType::FANTASY_WARRIOR:
            return {105, 14};
        default:
            return {100, 15};
    }
}
}

GameManager::GameManager(QObject *parent)
    : QObject(parent),
      playerName_(""),
      currentScore_(0),
      playerLevel_(1),
      currentLevel_(1),
      state_(GameState::MENU),
      player_(nullptr),
      currentEnemy_(nullptr),
      selectedPlayerType_(PlayerType::KNIGHT),
      campaignCompleted_(false)
    runMode_(RunMode::CAMPAIGN),
    duelVictory_(false){
    if (rand() == 0) srand(time(nullptr)); // seed random once
}

GameManager::~GameManager() {
    delete player_;
    delete currentEnemy_;
}

void GameManager::startCampaign(const std::string &playerName, PlayerType playerType) {
    runMode_ = RunMode::CAMPAIGN;
    duelVictory_ = false;

    playerName_ = playerName;
    selectedPlayerType_ = playerType;
    currentScore_ = 0;
    playerLevel_ = 1;
    currentLevel_ = 1;
    campaignCompleted_ = false;
    state_ = GameState::PLAYING;

    delete player_;
    player_ = nullptr;

    const PlayerStats playerStats = statsForPlayer(playerType);
    player_ = new Player(
        playerStats.hp,
        playerStats.hp,
        150.0f,
        400.0f,
        200.0f,
        playerStats.attack,
        playerType
    );

    player_->setName(playerName);

    spawnEnemyForCurrentLevel();
}
void GameManager::startDuel(const std::string &playerName,
                            PlayerType playerType,
                            const DuelConfig& config) {
    runMode_ = RunMode::DUEL;
    duelConfig_ = config;
    duelVictory_ = false;

    playerName_ = playerName;
    selectedPlayerType_ = playerType;
    currentScore_ = 0;
    playerLevel_ = 1;
    currentLevel_ = 1;
    campaignCompleted_ = false;
    state_ = GameState::PLAYING;

    delete player_;
    player_ = nullptr;

    const PlayerStats playerStats = statsForPlayer(playerType);
    player_ = new Player(
        playerStats.hp,
        playerStats.hp,
        150.0f,
        400.0f,
        200.0f,
        playerStats.attack,
        playerType
    );

    player_->setName(playerName);

    delete currentEnemy_;
    currentEnemy_ = nullptr;

    if (config.opponentMode == DuelOpponentMode::RANDOM) {
        const int randomIndex = rand() % static_cast<int>(kEnemyCampaign.size());
        const EnemyDefinition &definition = kEnemyCampaign[randomIndex];

        currentEnemy_ = new Enemy(
            definition.type,
            definition.name,
            definition.hp,
            definition.hp,
            700.0f,
            400.0f,
            definition.speed,
            definition.attack
        );
    } else {
        currentEnemy_ = new Enemy(
            config.manualEnemyOpponent,
            "Duel Challenger",
            110,
            110,
            700.0f,
            400.0f,
            150.0f,
            16
        );
    }
}

bool GameManager::advanceToNextLevel() {
    // 1v1 teammate:
    // Duel mode should not advance through campaign levels.
    // For 1v1, battle should end after one match.
    if (runMode_ == RunMode::DUEL) {
        duelVictory_ = true;
        state_ = GameState::GAME_OVER;
        return false;
    }
    if (campaignCompleted_) {
        return false;
    }

    if (currentLevel_ >= getTotalLevels()) {
        campaignCompleted_ = true;
        state_ = GameState::GAME_OVER;
        return false;
    }

    ++currentLevel_;
    spawnEnemyForCurrentLevel();

    if (player_) {
        // Small sustain between stages so the player can survive a full run.
        player_->takeDamage(-30);
    }

    return currentEnemy_ != nullptr;
}

void GameManager::finishBattle() {
    // Ranking teammate:
    // Good place to finalize match result info before progression is updated outside.
    if (runMode_ == RunMode::DUEL && player_ && player_->isAlive()) {
        duelVictory_ = true;
    }
    if (state_ == GameState::PLAYING) {
        state_ = GameState::GAME_OVER;
        emit battleFinished();
    }
}

void GameManager::addScore(int amount) {
    // Ranking teammate:
    // This can stay as the local battle score helper,
    // but long-term profile score/rank/rating should be updated through the progression system.
    if (amount <= 0) {
        return;
    }

    currentScore_ += amount;
    playerLevel_ = 1 + (currentScore_ / 100);
}

int GameManager::getCurrentScore() const {
    return currentScore_;
}

int GameManager::getPlayerLevel() const {
    return playerLevel_;
}

int GameManager::getCurrentLevel() const {
    return currentLevel_;
}

int GameManager::getTotalLevels() const {
    return static_cast<int>(kEnemyCampaign.size());
}

std::string GameManager::getBattleTitle() const {
    if (playerName_.empty()) {
        return "Battle";
    }

    return playerName_ + "'s Battle";
}

GameState GameManager::getState() const {
    return state_;
}

PlayerType GameManager::getSelectedPlayerType() const {
    return selectedPlayerType_;
}

bool GameManager::hasCompletedCampaign() const {
    return campaignCompleted_;
}

Player* GameManager::getPlayer() const {
    return player_;
}

Enemy* GameManager::getCurrentEnemy() const {
    return currentEnemy_;
}

RunMode GameManager::getRunMode() const {
    return runMode_;
}

DuelConfig GameManager::getDuelConfig() const {
    return duelConfig_;
}

bool GameManager::isDuelMode() const {
    return runMode_ == RunMode::DUEL;
}

bool GameManager::didWinDuel() const {
    return duelVictory_;
}

void GameManager::spawnEnemyForCurrentLevel() {
    // 1v1 teammate:
    // Reuse this logic for campaign only.
    // Duel mode will need separate opponent spawning based on the player's duel selection.
    delete currentEnemy_;
    currentEnemy_ = nullptr;

    const int index = std::clamp(currentLevel_ - 1, 0, getTotalLevels() - 1);
    const EnemyDefinition &definition = kEnemyCampaign[static_cast<size_t>(index)];
    currentEnemy_ = new Enemy(definition.type,
                              definition.name,
                              definition.hp,
                              definition.hp,
                              700.0f,
                              400.0f,
                              definition.speed,
                              definition.attack);
}
/*
 Calculates the score reward based on match type and performance.
 Duel: +50 for win, +10 for loss.
 Campaign: +50 per stage, +150 bonus for full clear, +10 for early defeat.
 */
int GameManager::calculateRewardForMatch(RunMode mode, bool victory, int stagesCleared, bool fullClear) {
    if (mode == RunMode::DUEL) {
        return victory ? 50 : 10;
    }
    else { // Campaign (Save the Kings)
        if (fullClear) {
            return (stagesCleared * 50) + 150;
        }
        return (stagesCleared > 0) ? (stagesCleared * 50) : 10;
    }
}

//Returns the fantasy rank title based on the player's accumulated score.
std::string GameManager::calculateRankFromScore(int totalScore) {
    if (totalScore >= 9000) return "Immortal";
    if (totalScore >= 6500) return "Legend";
    if (totalScore >= 4500) return "High Champion";
    if (totalScore >= 3200) return "Champion";
    if (totalScore >= 2200) return "Warlord";
    if (totalScore >= 1400) return "Elite Knight";
    if (totalScore >= 800)  return "Knight";
    if (totalScore >= 400)  return "Gladiator";
    if (totalScore >= 150)  return "Squire";
    return "Wanderer";
}

// Calculates a 5-star rating based on the player's win percentage.
double GameManager::calculateRatingFromStats(int totalWins, int totalMatches) {
    if (totalMatches <= 0) return 0.0;

    double winPercent = (static_cast<double>(totalWins) / totalMatches) * 100.0;

    if (winPercent >= 95.0) return 5.0;
    if (winPercent >= 85.0) return 4.5;
    if (winPercent >= 75.0) return 4.0;
    if (winPercent >= 65.0) return 3.5;
    if (winPercent >= 55.0) return 3.0;
    if (winPercent >= 45.0) return 2.5;
    if (winPercent >= 35.0) return 2.0;
    if (winPercent >= 25.0) return 1.5;
    if (winPercent >= 10.0) return 1.0;
    return 0.5;
}