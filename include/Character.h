#ifndef CHARACTER_H
#define CHARACTER_H

#include <string>

class Character {
protected:
    int hp, maxHp, attackPower;
    float x, y, speed;

public:
    Character(int hp, int maxHp, float x, float y, float speed, int attackPower);
    
    virtual ~Character();

    virtual void move() = 0;
    virtual void attack(Character& target) = 0;


    void takeDamage(int dmg);
    bool isAlive() const;
};

#endif// Character.h - Base character class
