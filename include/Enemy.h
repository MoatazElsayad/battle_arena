
//character needs to have x, speed and attackpower as protected 
#define ENEMY_H
#ifndef ENEMY_H

#include "Character.h" //not implemented yet 
#include "Player.h" //not implemented yet 
#include <cmath>

enum EnemyDifficulty{ EASY , MEDIUM , HARD};
class Enemy : public Character{

    private:
        EnemyDifficulty level;
        int attackCooldown;

    public:
        Enemy(EnemyDifficulty l):Character(100,100,100,4,8){ //(hp, x-axis, y-axis, speed,attackpower)
            this->level=l;
            setDifficultyStats();
            attackCooldown=0; 
        }
        void setDifficultyStats(){
            swtich(level){

                case EASY: 
                    speed=3;
                    attackPower =6;
                    break;
                case MEDIUM: 
                    speed=4;
                    attackPower =8;
                    break;
                case HARD: 
                    speed=6;
                    attackPower =12;
                    break;
            }
        }
        void updateAI(Player* player) {
            float dx = player->getX()-x;

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
        void move() overide {}

        void attack(Character* target) override{
            target->takeDamage(attackPower);
        }


};

#endif
