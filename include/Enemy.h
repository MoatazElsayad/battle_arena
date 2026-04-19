#ifndef ENEMY_H
#define ENEMY_H

#include "Character.h"
#include "Enums.h"

class Player;

class Enemy : public Character {

    private:
        int attackCooldown;

    public:
        Enemy(CharacterType type, const std::string& name, int hp, int maxHp, float x, float y, float speed, int attackPower);
        ~Enemy();
        
        void move() override;
        void attack(Character* target) override;
        float getX() const { return x; }
        CharacterType getType() const override { return type_; }
};

#endif
