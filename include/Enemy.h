#ifndef ENEMY_H
#define ENEMY_H

#include "Character.h"

class Player;

class Enemy : public Character {

    private:
        int attackCooldown;

    public:
        Enemy(int hp, int maxHp, float x, float y, float speed, int attackPower);
        ~Enemy();
        
        void move() override;
        void attack(Character* target) override;
        float getX() const { return x; }
};

#endif
