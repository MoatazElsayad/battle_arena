#ifndef GAMEOVERPAGE_H
#define GAMEOVERPAGE_H

#include <QWidget>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
class GameOverPage;
}
QT_END_NAMESPACE

class GameOverPage : public QWidget {
    Q_OBJECT

public:
    explicit GameOverPage(QWidget *parent = nullptr);
    ~GameOverPage();

    void setResult(bool victory);
    void setBattleStats(int damageDealt, int damageTaken, int score);

signals:
    void backToMenu();
    void playAgain();

private:
    Ui::GameOverPage *ui;

    // Battle stats
    int damageDealt_;
    int damageTaken_;
    int score_;
    bool victory_;
};

#endif // GAMEOVERPAGE_H
