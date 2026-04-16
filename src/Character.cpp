#include "Character.h"

Character::Character(int hp, int maxHp, float x, float y, float speed, int attackPower)
    : hp(hp), maxHp(maxHp), x(x), y(y), speed(speed), attackPower(attackPower), name("Unknown") {}

Character::~Character() {}

void Character::takeDamage(int dmg) {
    hp -= dmg;
    if (hp < 0) {
        hp = 0;
    } else if (hp > maxHp) {
        hp = maxHp;
    }
}

bool Character::isAlive() const {
    return hp > 0;
}

int Character::calculateDamage() const {
    // Base damage with ±25% variance
    int variance = (rand() % 50) - 25; // -25 to +25
    return attackPower + (attackPower * variance) / 100;
}

