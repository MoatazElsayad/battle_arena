#include "Enemy.h"
#include <cmath>

Enemy::Enemy(EnemyDifficulty l): Character(100, 100, 100, 4, 8){
    level=l;
    attackCooldown =0;
    setDifficultyStats();
}

void Enemy:: setDifficultyStats(){
    if (level == EASY) {
        speed = 3;
        attackPower = 6;
    }
    else if (level == NORMAL) {
        speed = 4;
        attackPower = 8;
    }
    else if (level == HARD) {
        speed = 6;
        attackPower = 12;
    }
}
void Enemy::updateAI(Player* player) {
    float dx = player->getX() - x;

    if (attackCooldown > 0)
        attackCooldown--;

    if (std::abs(dx) > speed) {
        if (dx > 0)
            x += speed;
        else
            x -= speed;
    }
    if (std::abs(dx) < 20 && attackCooldown == 0) {
        attack(player);
        attackCooldown = 30;
    }
}

void Enemy::move() {
}

void Enemy::attack(Character* target) {
    target->takeDamage(attackPower);
}

