#include "Character.h"

Character::Character(int hp, int maxHp, float x, float y, float speed, int attackPower)
    : hp(hp), maxHp(maxHp), x(x), y(y), speed(speed), attackPower(attackPower) {}

Character::~Character() {}

void Character::takeDamage(int dmg) {
    hp -= dmg;
    if (hp < 0) {
        hp = 0;
    }
}

bool Character::isAlive() const {
    return hp > 0;
}
