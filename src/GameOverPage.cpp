#include "GameOverPage.h"
#include <QApplication>
#include <QVBoxLayout>
#include "ui_GameOverPage.h"

GameOverPage::GameOverPage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::GameOverPage),
      damageDealt_(0),
      damageTaken_(0),
      score_(0),
      victory_(false) {

    ui->setupUi(this);

    // Group all absolutely positioned widgets into a fixed-size container
    QWidget *container = new QWidget(this);
    container->setFixedSize(673, 350);

    // Re-parent all the auto-generated widgets into the container
    ui->overLabel->setParent(container);
    ui->winlose->setParent(container);
    ui->scorePoints->setParent(container);
    ui->backButton->setParent(container);
    ui->restartButton->setParent(container);
    ui->exitButton->setParent(container);

    // Create a main layout to align the container in the center of the screen
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(container);

    ui->scorePoints->setWordWrap(true);
    ui->scorePoints->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    connect(ui->backButton, &QPushButton::clicked, this, &GameOverPage::backToMenu);
    connect(ui->restartButton, &QPushButton::clicked, this, &GameOverPage::playAgain);
    connect(ui->exitButton, &QPushButton::clicked, qApp, &QApplication::quit);
}

GameOverPage::~GameOverPage() {
    delete ui;
}

void GameOverPage::setResult(bool victory) {
    victory_ = victory;
    
    if (victory) {
        ui->winlose->setText("VICTORY");
        ui->winlose->setStyleSheet("color: #FFD700;");
    } else {
        ui->winlose->setText("DEFEAT");
        ui->winlose->setStyleSheet("color: #C95A4A;");
    }
}

void GameOverPage::setBattleStats(int damageDealt, int damageTaken, int score) {
    damageDealt_ = damageDealt;
    damageTaken_ = damageTaken;
    score_ = score;

    const QString statsText = QString(
        "<span style=\"font-family:'Showcard Gothic'; font-size:18pt; color:#D4A017;\">"
        "SCORE: %1<br>"
        "DAMAGE DEALT: %2<br>"
        "DAMAGE TAKEN: %3"
        "</span>"
    ).arg(score).arg(damageDealt).arg(damageTaken);

    ui->scorePoints->setText(statsText);
}
