// Player.cpp - Player implementation
#include "Player.h"

Player::Player(int hp, int maxHp, float x, float y, float speed, int attackPower, CharacterType type)
    : Character(hp, maxHp, x, y, speed, attackPower),
      score(0),
    input(nullptr),
    characterType_(type),
    features_(InputHandler::getCharacterFeatures(type)) {}

Player::~Player() {}

void Player::move() {
    // Movement controlled by BattleWidget via InputHandler
}

void Player::attack(Character* target) {
    if (target && target->isAlive()) {
        int damage = calculateDamage();
        target->takeDamage(damage);
    }
}

void Player::addScore(int s) {
    score += s;
}

int Player::getScore() const {
    return score;
}

void Player::setCharacterType(CharacterType type) {
    characterType_ = type;
    features_ = InputHandler::getCharacterFeatures(type);
}