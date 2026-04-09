#include <iostream>
#include "GameManager.h"
#include <string>
using namespace std;

{
GameManager::GameManager(Player* p,Enemy* e) : player(p), enemy(e), state(GameState::Playing)
{
   
}
  
void GameManager::attack()

{
  float dx = player->getX() - enemy->getX();


  if(abs(dx) < 10)
  {
    player->attack(enemy);//i visulaize that attack reduces hp and does al
   
  }
}


void GameManager::update()
{
 if (state != PLAYING)
 {
  return;
 }
if()//move button is clicked
{
 player -> move();//but what would determine if he moves right or left(i think there should be move left or more right)
}

 enemy->updateAI(player);
 if()//attack button is clicked
 {
    attack();
 }
 if((player->isAlive() == false || !(enemy->isAlive()) == false)
 {
  state = GAME_OVER;
  endgame();
 }

}

void GameManager::StartGame()
{
  this->state = PLAYING;
  player->setx(-50);
  enemy->setx(50);
  if(player->type == TANK)
  {
    player->setStats(TANK);
  }
  else if(player->type == FAST)
  {
    player->setStats(FAST);
  }
  else if(player->type == BALANCED)
  {
    player->setStats(BALANCED);
  }
}

string GameManager::endGame();
{
   checkWinCondition();
}


void GameManager::checkWinCondition()
{
    if(player->isAlive() && !(enemy->isAlive()))
  {
     cout << "You Won!"

  }
  else if(!(player->isAlive()) && !(enemy->isAlive()))
  {
     cout << "You Lost!"
  }
  else if((player->isAlive()) && (enemy->isAlive()))
  {
      cout << "Draw"//due to time finished while no one defeated the other
  }
}



GameManager::~GameManager(){

  cout << "GameManager destructor is called";

}

}// GameManager.cpp - Game manager implementation// GameManager.cpp - Game manager implementation
