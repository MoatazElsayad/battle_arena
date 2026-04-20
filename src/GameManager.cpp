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
      campaignCompleted_(false) {
    if (rand() == 0) srand(time(nullptr)); // seed random once
}

GameManager::~GameManager() {
    delete player_;
    delete currentEnemy_;
}

void GameManager::startGame(const std::string &playerName, PlayerType playerType) {
    // 1v1 teammate:
    // Split setup here by theme:
    // - Save the Kings = current campaign flow
    // - 1v1 = one match only with chosen/random opponent and chosen background
    // Ranking teammate:
    // Keep track of enough battle context here for reward calculation after the match.
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
    player_ = new Player(playerStats.hp, playerStats.hp, 150.0f, 400.0f, 200.0f, playerStats.attack, playerType);
    player_->setName(playerName);

    spawnEnemyForCurrentLevel();
}

bool GameManager::advanceToNextLevel() {
    // 1v1 teammate:
    // Duel mode should not advance through campaign levels.
    // For 1v1, battle should end after one match.
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
