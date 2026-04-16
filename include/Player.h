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
	CharacterType characterType_;
	CharacterFeatureSet features_;

public:
	Player(int hp, int maxHp, float x, float y, float speed, int attackPower, CharacterType type = CharacterType::KNIGHT);
	~Player();

	void move() override;
	void attack(Character* target) override;
	float getX() const { return x; }
	CharacterType getCharacterType() const { return characterType_; }
	CharacterFeatureSet getFeatures() const { return features_; }
	void setCharacterType(CharacterType type);

	void addScore(int s);
	int getScore() const;
};

#endif