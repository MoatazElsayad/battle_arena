#ifndef GAMEPAGE_H
#define GAMEPAGE_H

#include <QWidget>

class GamePage : public QWidget
{
    Q_OBJECT

public:
    explicit GamePage(QWidget *parent = nullptr);
    ~GamePage();

signals:
    void gameOver();
    void backToMenu();

public slots:

private:
    class GamePagePrivate *d;
};

#endif // GAMEPAGE_H