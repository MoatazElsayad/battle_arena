#include "Enemy.h"
#include <cmath>

Enemy::Enemy(CharacterType type, const std::string& name, int hp, int maxHp, float x, float y, float speed, int attackPower)
    : Character(hp, maxHp, x, y, speed, attackPower) {
    type_ = type;
    name_ = name;
    attackCooldown = 0;
}

Enemy::~Enemy() {}

void Enemy::move() {
    // Movement controlled by BattleWidget's updateEnemyAI
}

void Enemy::attack(Character* target) {
    if (target && target->isAlive()) {
        int damage = calculateDamage();
        target->takeDamage(damage);
    }
}

