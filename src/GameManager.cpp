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

constexpr std::array<EnemyDefinition, 3> kFinalWerewolfGuardians = {{
    {EnemyType::BLACK_WEREWOLF, "Black Werewolf", 168, 20, 175.0f},
    {EnemyType::RED_WEREWOLF, "Red Werewolf", 188, 23, 182.0f},
    {EnemyType::WHITE_WEREWOLF, "White Werewolf", 214, 27, 190.0f},
}};

constexpr std::array<PlayerType, 9> kPlayerRoster = {{
    PlayerType::ARCEN,
    PlayerType::DEMON_SLAYER,
    PlayerType::FANTASY_WARRIOR,
    PlayerType::HUNTRESS,
    PlayerType::KNIGHT,
    PlayerType::MARTIAL,
    PlayerType::MARTIAL_HERO,
    PlayerType::MEDIEVAL_WARRIOR,
    PlayerType::WIZARD
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

EnemyType duelArchetypeForPlayer(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN:
            return EnemyType::FIRE_WORM;
        case PlayerType::WIZARD:
            return EnemyType::FIRE_WIZARD;
        case PlayerType::HUNTRESS:
            return EnemyType::FLYING_DEMON;
        case PlayerType::MARTIAL:
        case PlayerType::MARTIAL_HERO:
            return EnemyType::NIGHTWEAVER;
        case PlayerType::DEMON_SLAYER:
            return EnemyType::RED_WEREWOLF;
        case PlayerType::MEDIEVAL_WARRIOR:
            return EnemyType::BLACK_WEREWOLF;
        case PlayerType::FANTASY_WARRIOR:
            return EnemyType::EVIL_WIZARD;
        case PlayerType::KNIGHT:
        default:
            return EnemyType::WHITE_WEREWOLF;
    }
}

float duelSpeedForPlayer(PlayerType type) {
    switch (type) {
        case PlayerType::HUNTRESS:
        case PlayerType::MARTIAL:
        case PlayerType::MARTIAL_HERO:
            return 182.0f;
        case PlayerType::ARCEN:
        case PlayerType::WIZARD:
            return 148.0f;
        case PlayerType::KNIGHT:
        case PlayerType::MEDIEVAL_WARRIOR:
            return 154.0f;
        default:
            return 168.0f;
    }
}

std::string displayNameForPlayer(PlayerType type) {
    switch (type) {
        case PlayerType::ARCEN: return "Arcen";
        case PlayerType::DEMON_SLAYER: return "Demon Slayer";
        case PlayerType::FANTASY_WARRIOR: return "Fantasy Warrior";
        case PlayerType::HUNTRESS: return "Huntress";
        case PlayerType::KNIGHT: return "Knight";
        case PlayerType::MARTIAL: return "Martial";
        case PlayerType::MARTIAL_HERO: return "Martial Hero";
        case PlayerType::MEDIEVAL_WARRIOR: return "Medieval Warrior";
        case PlayerType::WIZARD: return "Wizard";
    }
    return "Gladiator";
}

EnemyDefinition definitionForEnemyType(EnemyType type) {
    const auto inCampaign = std::find_if(kEnemyCampaign.begin(), kEnemyCampaign.end(), [type](const EnemyDefinition& definition) {
        return definition.type == type;
    });
    if (inCampaign != kEnemyCampaign.end()) {
        return *inCampaign;
    }

    const auto inFinal = std::find_if(kFinalWerewolfGuardians.begin(), kFinalWerewolfGuardians.end(), [type](const EnemyDefinition& definition) {
        return definition.type == type;
    });
    if (inFinal != kFinalWerewolfGuardians.end()) {
        return *inFinal;
    }

    return kEnemyCampaign.front();
}

PlayerType randomPlayerOpponent(PlayerType selected) {
    std::vector<PlayerType> candidates;
    for (PlayerType type : kPlayerRoster) {
        if (type != selected) {
            candidates.push_back(type);
        }
    }

    if (candidates.empty()) {
        return PlayerType::KNIGHT;
    }

    const int index = rand() % static_cast<int>(candidates.size());
    return candidates[static_cast<size_t>(index)];
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
      duelOpponentPlayerType_(PlayerType::KNIGHT),
      duelOpponentName_(""),
      duelArenaName_(""),
      campaignCompleted_(false) {
    lanDuelMode_ = false;
    finalGuardianIndex_ = 0;
    runMode_ = RunMode::CAMPAIGN;
    duelConfig_ = DuelConfig();
    duelVictory_ = false;
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
    duelOpponentPlayerType_ = PlayerType::KNIGHT;
    duelOpponentName_.clear();
    duelArenaName_.clear();
    currentScore_ = 0;
    playerLevel_ = 1;
    currentLevel_ = 1;
    campaignCompleted_ = false;
    lanDuelMode_ = false;
    finalGuardianIndex_ = 0;
    runMode_ = RunMode::CAMPAIGN;
    duelConfig_ = DuelConfig();
    duelVictory_ = false;
    state_ = GameState::PLAYING;

    delete player_;
    player_ = nullptr;

    const PlayerStats playerStats = statsForPlayer(playerType);
    player_ = new Player(playerStats.hp, playerStats.hp, 150.0f, 400.0f, 200.0f, playerStats.attack, playerType);
    player_->setName(playerName);

    spawnEnemyForCurrentLevel();
}

void GameManager::startLanDuel(const std::string &playerName,
                               PlayerType playerType,
                               const std::string &opponentName,
                               PlayerType opponentType,
                               const std::string &arenaName) {
    playerName_ = playerName;
    selectedPlayerType_ = playerType;
    duelOpponentPlayerType_ = opponentType;
    duelOpponentName_ = opponentName.empty() ? "Linked Rival" : opponentName;
    duelArenaName_ = arenaName.empty() ? "Arena Link" : arenaName;
    currentScore_ = 0;
    playerLevel_ = 1;
    currentLevel_ = 1;
    campaignCompleted_ = false;
    lanDuelMode_ = true;
    finalGuardianIndex_ = 0;
    runMode_ = RunMode::DUEL;
    duelConfig_ = DuelConfig();
    duelVictory_ = false;
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
    lanDuelMode_ = false;

    playerName_ = playerName;
    selectedPlayerType_ = playerType;
    duelOpponentPlayerType_ = PlayerType::KNIGHT;
    duelOpponentName_.clear();
    duelArenaName_ = config.selectedArena.trimmed().isEmpty()
        ? "Colosseum"
        : config.selectedArena.toStdString();
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
    spawnEnemyForCurrentLevel();
}

bool GameManager::advanceToNextLevel() {
    // LAN teammate:
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
    if (runMode_ == RunMode::DUEL) {
        return 1;
    }
    return static_cast<int>(kEnemyCampaign.size()) + 1;
}

std::string GameManager::getBattleTitle() const {
    if (runMode_ == RunMode::DUEL) {
        const std::string playerLabel = playerName_.empty() ? "Gladiator" : playerName_;
        const std::string opponentLabel = duelOpponentName_.empty() ? "Exhibition Rival" : duelOpponentName_;
        return playerLabel + " vs " + opponentLabel;
    }

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

PlayerType GameManager::getLanOpponentPlayerType() const {
    return duelOpponentPlayerType_;
}

std::string GameManager::getLanOpponentName() const {
    return duelOpponentName_;
}

std::string GameManager::getLanArenaName() const {
    return duelArenaName_;
}

bool GameManager::hasCompletedCampaign() const {
    return campaignCompleted_;
}

bool GameManager::isLanDuel() const {
    return lanDuelMode_;
}

bool GameManager::isFinalKingStage() const {
    if (runMode_ != RunMode::CAMPAIGN || lanDuelMode_) {
        return false;
    }
    return currentLevel_ == getTotalLevels();
}

bool GameManager::advanceFinalGuardianWave() {
    if (runMode_ != RunMode::CAMPAIGN || lanDuelMode_) {
        return false;
    }

    if (!isFinalKingStage()) {
        return false;
    }

    if (finalGuardianIndex_ >= static_cast<int>(kFinalWerewolfGuardians.size()) - 1) {
        return false;
    }

    ++finalGuardianIndex_;
    spawnEnemyForCurrentLevel();
    return currentEnemy_ != nullptr;
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
    // LAN teammate:
    // Reuse this logic for campaign only.
    // Duel mode will need separate opponent spawning based on the player's duel selection.
    delete currentEnemy_;
    currentEnemy_ = nullptr;

    if (lanDuelMode_) {
        const PlayerStats duelStats = statsForPlayer(duelOpponentPlayerType_);
        currentEnemy_ = new Enemy(duelArchetypeForPlayer(duelOpponentPlayerType_),
                                  duelOpponentName_.empty() ? "Linked Rival" : duelOpponentName_,
                                  duelStats.hp,
                                  duelStats.hp,
                                  700.0f,
                                  400.0f,
                                  duelSpeedForPlayer(duelOpponentPlayerType_),
                                  duelStats.attack);
        return;
    }

    if (runMode_ == RunMode::DUEL) {
        if (duelConfig_.category == DuelOpponentCategory::PLAYER_TYPE) {
            duelOpponentPlayerType_ = duelConfig_.opponentMode == DuelOpponentMode::RANDOM
                ? randomPlayerOpponent(selectedPlayerType_)
                : duelConfig_.manualPlayerOpponent;
            duelOpponentName_ = displayNameForPlayer(duelOpponentPlayerType_);

            const PlayerStats duelStats = statsForPlayer(duelOpponentPlayerType_);
            currentEnemy_ = new Enemy(duelArchetypeForPlayer(duelOpponentPlayerType_),
                                      duelOpponentName_.c_str(),
                                      duelStats.hp,
                                      duelStats.hp,
                                      700.0f,
                                      400.0f,
                                      duelSpeedForPlayer(duelOpponentPlayerType_),
                                      duelStats.attack);
            return;
        }

        const EnemyDefinition definition = duelConfig_.opponentMode == DuelOpponentMode::RANDOM
            ? kEnemyCampaign[static_cast<size_t>(rand() % static_cast<int>(kEnemyCampaign.size()))]
            : definitionForEnemyType(duelConfig_.manualEnemyOpponent);

        duelOpponentName_ = definition.name;
        currentEnemy_ = new Enemy(definition.type,
                                  definition.name,
                                  definition.hp,
                                  definition.hp,
                                  700.0f,
                                  400.0f,
                                  definition.speed,
                                  definition.attack);
        return;
    }

    EnemyDefinition definition = kEnemyCampaign.back();
    if (isFinalKingStage()) {
        const int waveIndex = std::clamp(finalGuardianIndex_, 0, static_cast<int>(kFinalWerewolfGuardians.size()) - 1);
        definition = kFinalWerewolfGuardians[static_cast<size_t>(waveIndex)];
    } else {
        const int index = std::clamp(currentLevel_ - 1, 0, static_cast<int>(kEnemyCampaign.size()) - 1);
        definition = kEnemyCampaign[static_cast<size_t>(index)];
    }

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
