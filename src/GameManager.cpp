#include "GameManager.h"
#include "Player.h"
#include "Enemy.h"
#include <cstdlib>
#include <ctime>

GameManager::GameManager(QObject *parent)
    : QObject(parent),
      playerName_(""),
      currentScore_(0),
      playerLevel_(1),
      state_(GameState::MENU),
      player_(nullptr),
      currentEnemy_(nullptr),
      selectedCharacterType_(CharacterType::KNIGHT) {
    if (rand() == 0) srand(time(nullptr)); // seed random once
}

GameManager::~GameManager() {
    delete player_;
    delete currentEnemy_;
}

void GameManager::startGame(const std::string &playerName, CharacterType charType) {
    playerName_ = playerName;
    selectedCharacterType_ = charType;
    currentScore_ = 0;
    playerLevel_ = 1;
    state_ = GameState::PLAYING;
    
    // Create or reset player
    if (!player_) {
        // Stats vary by character type
        int hp = 100;
        int atk = 15;
        switch (charType) {
            case CharacterType::KNIGHT:
            case CharacterType::MEDIEVAL_WARRIOR:
                hp = 120; atk = 12; break;
            case CharacterType::WIZARD:
                hp = 70; atk = 20; break;
            case CharacterType::ARCEN:
                hp = 80; atk = 18; break;
            case CharacterType::DEMON_SLAYER:
                hp = 110; atk = 16; break;
            case CharacterType::HUNTRESS:
                hp = 85; atk = 17; break;
            case CharacterType::MARTIAL:
            case CharacterType::MARTIAL_HERO:
                hp = 100; atk = 15; break;
            case CharacterType::FANTASY_WARRIOR:
                hp = 105; atk = 14; break;
            default:
                hp = 100; atk = 15;
        }
        player_ = new Player(hp, hp, 150, 400, 200, atk, charType);
        player_->setName(playerName);
    } else {
        player_->setCharacterType(charType);
        player_->takeDamage(-player_->getMaxHealth()); // Full heal
    }
    
    // Create enemy
    delete currentEnemy_;
    currentEnemy_ = new Enemy(CharacterType::FLYING_DEMON, "Flying Demon", 80, 80, 700, 400, 150, 12);
}

void GameManager::finishBattle() {
    if (state_ == GameState::PLAYING) {
        state_ = GameState::GAME_OVER;
        emit battleFinished();
    }
}

void GameManager::addScore(int amount) {
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

std::string GameManager::getBattleTitle() const {
    if (playerName_.empty()) {
        return "Battle";
    }

    return playerName_ + "'s Battle";
}

GameState GameManager::getState() const {
    return state_;
}

CharacterType GameManager::getSelectedCharacterType() const {
    return selectedCharacterType_;
}

Player* GameManager::getPlayer() const {
    return player_;
}

Enemy* GameManager::getCurrentEnemy() const {
    return currentEnemy_;
}
