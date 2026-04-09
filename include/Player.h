// Player.h - Player class definition
#ifndef PLAYER_H
#define PLAYER_H

#include "Character.h"
#include "InputHandler.h"

enum PlayerType {
	TANK,
	FAST,
	BALANCED
};

class Player : public Character {
private:
	int score;
	PlayerType type;
	InputHandler* input;

	void setStats();

public:
	Player(PlayerType t, InputHandler* input);

	void move() override;
	void attack(Character* target) override;

	void addScore(int s);
	int getScore() const;
};

#endif