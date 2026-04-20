// Player.h - Player class definition
#ifndef PLAYER_H
#define PLAYER_H

#include "Character.h"
#include "InputHandler.h"
#include "Enums.h"

class Player : public Character {
private:
	int score;
	InputHandler* input;
	PlayerType playerType_;
	CharacterFeatureSet features_;

public:
	Player(int hp, int maxHp, float x, float y, float speed, int attackPower, PlayerType type = PlayerType::KNIGHT);
	~Player();

	void move() override;
	void attack(Character* target) override;
	float getX() const { return x; }
	PlayerType getPlayerType() const { return playerType_; }
	CharacterFeatureSet getFeatures() const { return features_; }
	void setPlayerType(PlayerType type);

	void addScore(int s);
	int getScore() const;
};

#endif
