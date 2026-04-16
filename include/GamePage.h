#ifndef GAMEPAGE_H
#define GAMEPAGE_H

#include <QWidget>
#include <QString>

class BattleWidget;
class GameManager;
class SoundManager;
class QLabel;

class GamePage : public QWidget {
    Q_OBJECT

public:
    explicit GamePage(QWidget *parent = nullptr);
    ~GamePage();

    void setGameManager(GameManager *gm);
    void setSoundManager(SoundManager *sm);
    void startBattle();

signals:
    void battleFinished();

private slots:
    void onBattleFinished();
    void updateStats();

private:
    void setupUI();

    GameManager *gameManager_;
    SoundManager *soundManager_;
    BattleWidget *battleWidget_;
    QLabel *playerInfoLabel_;
    QLabel *instructionsLabel_;
};

#endif // GAMEPAGE_H
