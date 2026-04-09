// GameManager.h - Game logic and state management
// GameManager.h - Game logic and state management

#indef GAMEMANAGER_H
#indef GAMEMANAGER_H

#include <iostream>
#include <string>
using namespace std;

class GameManager{
private:

Player* player;
Enemy* enemy;
Gamestate state;
void handleCombat();


public:

GameManager(Player* p, Enemy* e);
void update();
void StartGame();
string endGame();
void checkWinCondition();
~GameManager();

};