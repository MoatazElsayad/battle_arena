#include "GameOverPage.h"
#include "ui_GameOverPage.h"
#include <QApplication>

GameOverPage::GameOverPage(QWidget *parent)
    : QDialog(parent),
    ui(new Ui::GameOverPage)  
{
    ui->setupUi(this);

    connect(ui->backButton, &QPushButton::clicked,
            this, &GameOverPage::onBackToMenu);

    connect(ui->restartButton, &QPushButton::clicked,
            this, &GameOverPage::onRestartGame);

    connect(ui->exitButton, &QPushButton::clicked,
            this, &GameOverPage::onExitGame);
}

GameOverPage::~GameOverPage()
{
    delete ui;
}
void GameOverPage::setResult(bool win)
{
    if (win)
        ui->winlose->setText("You Win!");
    else
        ui->winlose->setText("You Lose!");
}

void GameOverPage::setScore(int score)
{
    ui->scorePoints->setText("Score: " + QString::number(score));
}

void GameOverPage::onBackToMenu()
{
    this->close();
    emit backToMenu();
}
void GameOverPage::onRestartGame()
{
    this->close();
    emit restartGame(); // closes GameOverPage
}
void GameOverPage::onExitGame()
{
    QApplication::quit();
}
