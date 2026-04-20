#ifndef CHARACTER_H
#define CHARACTER_H

#include <string>
#include "Enums.h"

class Character {
protected:
    int hp, maxHp, attackPower;
    float x, y, speed;
    std::string name_;

public:
    Character(int hp, int maxHp, float x, float y, float speed, int attackPower);
    
    virtual ~Character();

    virtual void move() = 0;
    virtual void attack(Character* target) = 0;

    void takeDamage(int dmg);
    bool isAlive() const;
    
    int getHealth() const { return hp; }
    int getMaxHealth() const { return maxHp; }
    virtual int calculateDamage() const;
    std::string getName() const { return name_; }
    void setName(const std::string &n) { name_ = n; }
};

#endif
