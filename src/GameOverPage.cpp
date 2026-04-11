#include "GameOverPage.h"

class GameOverPagePrivate
{
public:
};

GameOverPage::GameOverPage(QWidget *parent)
    : QWidget(parent)
    , d(new GameOverPagePrivate)
{
}

GameOverPage::~GameOverPage()
{
    delete d;
}