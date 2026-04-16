#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <QObject>
#include <string>
#include <memory>

#include "Enums.h"

class Player;
class Enemy;

class GameManager : public QObject {
    Q_OBJECT

public:
    explicit GameManager(QObject *parent = nullptr);
    ~GameManager();

    void startGame(const std::string &playerName, CharacterType charType);
    void finishBattle();
    void addScore(int amount);

    int getCurrentScore() const;
    int getPlayerLevel() const;
    std::string getBattleTitle() const;
    GameState getState() const;
    CharacterType getSelectedCharacterType() const;
    
    Player* getPlayer() const;
    Enemy* getCurrentEnemy() const;

signals:
    void battleFinished();

private:
    std::string playerName_;
    int currentScore_;
    int playerLevel_;
    GameState state_;
    Player* player_;
    Enemy* currentEnemy_;
    CharacterType selectedCharacterType_;
};

#endif // GAMEMANAGER_H
