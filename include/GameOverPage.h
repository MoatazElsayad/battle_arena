#ifndef GAMEOVERPAGE_H
#define GAMEOVERPAGE_H

#include <QWidget>

class GameOverPage : public QWidget
{
    Q_OBJECT

public:
    explicit GameOverPage(QWidget *parent = nullptr);
    ~GameOverPage();

signals:
    void restartClicked();
    void menuClicked();

public slots:

private:
    class GameOverPagePrivate *d;
};

#endif // GAMEOVERPAGE_H