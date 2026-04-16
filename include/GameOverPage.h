#ifndef GAMEOVERPAGE_H
#define GAMEOVERPAGE_H

#include <QDialog>

namespace Ui {
class GameOverPage;
}

class GameOverPage : public QDialog
{
    Q_OBJECT

public:
    explicit GameOverPage(QWidget *parent = nullptr);
    ~GameOverPage();

    void setResult(bool win);
    void setScore(int score);

private slots:
    void onBackToMenu();
    void onRestartGame();
    void onExitGame();

signals:
    void backToMenu();
    void restartGame();

private:
    Ui::GameOverPage *ui;
};

#endif // GAMEOVERPAGE_H
