#include "MainMenuPage.h"

class MainMenuPagePrivate
{
public:
};

MainMenuPage::MainMenuPage(QWidget *parent)
    : QWidget(parent)
    , d(new MainMenuPagePrivate)
{
}

MainMenuPage::~MainMenuPage()
{
    delete d;
}