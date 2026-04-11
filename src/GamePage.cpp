#include "GamePage.h"

class GamePagePrivate
{
public:
};

GamePage::GamePage(QWidget *parent)
    : QWidget(parent)
    , d(new GamePagePrivate)
{
}

GamePage::~GamePage()
{
    delete d;
}