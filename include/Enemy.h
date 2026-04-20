#ifndef ENEMY_H
#define ENEMY_H

#include "Character.h"
#include "Enums.h"

class Player;

class Enemy : public Character {

    private:
        int attackCooldown;
        EnemyType enemyType_;

    public:
        Enemy(EnemyType type, const std::string& name, int hp, int maxHp, float x, float y, float speed, int attackPower);
        ~Enemy();
        
        void move() override;
        void attack(Character* target) override;
        float getX() const { return x; }
        EnemyType getEnemyType() const { return enemyType_; }
};

#endif
