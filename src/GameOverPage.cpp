#include "GameOverPage.h"
#include <QApplication>
#include <QColor>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include "ui_GameOverPage.h"

GameOverPage::GameOverPage(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::GameOverPage),
      damageDealt_(0),
      damageTaken_(0),
      score_(0),
      victory_(false),
      summaryTheme_(SummaryTheme::SaveTheKing) {

    ui->setupUi(this);

    setObjectName("gameOverRoot");
    setStyleSheet(
        "QWidget#gameOverRoot {"
        " background:qradialgradient(cx:0.5, cy:0.42, radius:0.92,"
        " stop:0 #402818, stop:0.42 #22140D, stop:1 #070403);"
        " color:#F5E6D3;"
        "}"
    );

    auto *container = new QFrame(this);
    container->setObjectName("resultCard");
    container->setMinimumSize(760, 500);
    container->setMaximumWidth(920);
    container->setStyleSheet(
        "QFrame#resultCard {"
        " background:qlineargradient(x1:0,y1:0,x2:0,y2:1,"
        " stop:0 rgba(43,30,20,0.96), stop:0.5 rgba(20,14,11,0.94), stop:1 rgba(12,8,7,0.97));"
        " border:2px solid rgba(212,160,23,0.52);"
        " border-radius:30px;"
        "}"
        "QLabel { background:transparent; }"
    );

    auto *cardGlow = new QGraphicsDropShadowEffect(container);
    cardGlow->setColor(QColor(212, 65, 36, 120));
    cardGlow->setOffset(0, 0);
    cardGlow->setBlurRadius(36);
    container->setGraphicsEffect(cardGlow);

    // Re-parent all the auto-generated widgets into the container
    ui->overLabel->setParent(container);
    ui->winlose->setParent(container);
    ui->scorePoints->setParent(container);
    ui->backButton->setParent(container);
    ui->restartButton->setParent(container);
    ui->exitButton->setParent(container);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(44, 40, 44, 40);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(container);

    auto *cardLayout = new QVBoxLayout(container);
    cardLayout->setContentsMargins(42, 34, 42, 34);
    cardLayout->setSpacing(18);

    ui->overLabel->setText("SAVE THE KING");
    ui->overLabel->setAlignment(Qt::AlignCenter);
    ui->overLabel->setStyleSheet(
        "color:#FFD45A;"
        "font:900 44px 'Showcard Gothic';"
        "letter-spacing:3px;"
    );
    cardLayout->addWidget(ui->overLabel);

    ui->winlose->setAlignment(Qt::AlignCenter);
    ui->winlose->setStyleSheet(
        "color:#4CFF6C;"
        "font:900 38px 'Showcard Gothic';"
        "letter-spacing:2px;"
    );
    cardLayout->addWidget(ui->winlose);

    ui->scorePoints->setWordWrap(true);
    ui->scorePoints->setAlignment(Qt::AlignCenter);
    ui->scorePoints->setMinimumHeight(210);
    ui->scorePoints->setStyleSheet(
        "QLabel#scorePoints {"
        " background:rgba(31,22,16,0.92);"
        " border:1px solid rgba(255,206,91,0.42);"
        " border-radius:18px;"
        " padding:20px 24px;"
        " color:#F5E6D3;"
        "}"
    );
    cardLayout->addWidget(ui->scorePoints, 1);

    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(16);
    buttonRow->addWidget(ui->backButton);
    buttonRow->addWidget(ui->restartButton);
    buttonRow->addWidget(ui->exitButton);
    cardLayout->addLayout(buttonRow);

    const QString buttonStyle =
        "QPushButton {"
        " background:rgba(58,42,27,0.92);"
        " border:1px solid rgba(212,160,23,0.42);"
        " border-radius:15px;"
        " color:#F0DEB2;"
        " font:800 15px 'Segoe UI';"
        " padding:13px 22px;"
        " min-height:24px;"
        "}"
        "QPushButton:hover { background:rgba(82,58,35,0.96); border-color:#D4A017; }"
        "QPushButton:pressed { background:rgba(50,34,22,0.98); }";

    ui->backButton->setText("Back to Lobby");
    ui->restartButton->setText("Play Again");
    ui->exitButton->setText("Exit");
    ui->backButton->setCursor(Qt::PointingHandCursor);
    ui->restartButton->setCursor(Qt::PointingHandCursor);
    ui->exitButton->setCursor(Qt::PointingHandCursor);
    ui->backButton->setStyleSheet(buttonStyle);
    ui->restartButton->setStyleSheet(
        "QPushButton {"
        " background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #6E0D0D, stop:0.48 #A81616, stop:1 #D84321);"
        " border:2px solid #D86A55;"
        " border-radius:15px;"
        " color:#FFE6E6;"
        " font:900 15px 'Segoe UI';"
        " padding:13px 22px;"
        " min-height:24px;"
        "}"
        "QPushButton:hover { border-color:#FFB299; }"
    );
    ui->exitButton->setStyleSheet(buttonStyle);

    connect(ui->backButton, &QPushButton::clicked, this, &GameOverPage::backToMenu);
    connect(ui->restartButton, &QPushButton::clicked, this, &GameOverPage::playAgain);
    connect(ui->exitButton, &QPushButton::clicked, qApp, &QApplication::quit);
}

GameOverPage::~GameOverPage() {
    delete ui;
}

void GameOverPage::setResult(bool victory) {
    victory_ = victory;

    QString overLabel = QStringLiteral("GAME OVER");
    QString winLoseLabel = QStringLiteral("DEFEAT");
    QString winLoseColor = QStringLiteral("#C95A4A");

    switch (summaryTheme_) {
        case SummaryTheme::SaveTheKing:
            if (victory) {
                overLabel = QStringLiteral("SAVE THE KING");
                winLoseLabel = QStringLiteral("THE KING IS SAVED");
                winLoseColor = QStringLiteral("#4CFF6C");
            }
            break;
        case SummaryTheme::ExhibitionDuel:
            overLabel = QStringLiteral("1V1 EXHIBITION");
            if (victory) {
                winLoseLabel = QStringLiteral("DUEL WON");
                winLoseColor = QStringLiteral("#4CFF6C");
            } else {
                winLoseLabel = QStringLiteral("DUEL LOST");
            }
            break;
        case SummaryTheme::LanDuel:
            overLabel = QStringLiteral("ARENA LINK");
            if (victory) {
                winLoseLabel = QStringLiteral("LINK WON");
                winLoseColor = QStringLiteral("#4CFF6C");
            } else {
                winLoseLabel = QStringLiteral("LINK LOST");
            }
            break;
    }

    ui->overLabel->setText(overLabel);
    ui->winlose->setText(winLoseLabel);
    ui->winlose->setStyleSheet(QString(
        "color:%1;"
        "font:900 38px 'Showcard Gothic';"
        "letter-spacing:2px;"
    ).arg(winLoseColor));
}

void GameOverPage::setSummaryTheme(SummaryTheme theme) {
    summaryTheme_ = theme;
}

void GameOverPage::setBattleStats(int damageDealt, int damageTaken, int score) {
    damageDealt_ = damageDealt;
    damageTaken_ = damageTaken;
    score_ = score;

    QString headline;
    QString statusLabel;
    QString statusValue;
    QString statusColor = victory_ ? QStringLiteral("#4CFF6C") : QStringLiteral("#C95A4A");
    QString footer = QStringLiteral("Your result has been recorded. Keep climbing the ranks and protect the crown.");

    switch (summaryTheme_) {
        case SummaryTheme::SaveTheKing:
            headline = victory_
                ? QStringLiteral("The king is safe. The realm remembers your victory.")
                : QStringLiteral("The king still waits for a champion. Return stronger.");
            statusLabel = QStringLiteral("KING STATUS");
            statusValue = victory_ ? QStringLiteral("SAVED") : QStringLiteral("NOT SAVED");
            break;
        case SummaryTheme::ExhibitionDuel:
            headline = victory_
                ? QStringLiteral("You claimed the exhibition duel with style.")
                : QStringLiteral("The exhibition rival took this round. Queue the rematch.");
            statusLabel = QStringLiteral("DUEL RESULT");
            statusValue = victory_ ? QStringLiteral("VICTORY") : QStringLiteral("DEFEAT");
            footer = QStringLiteral("Your exhibition result has been recorded. Fine-tune the matchup and run it again.");
            break;
        case SummaryTheme::LanDuel:
            headline = victory_
                ? QStringLiteral("Arena Link victory secured. The room felt that one.")
                : QStringLiteral("Arena Link defeat recorded. Reset and challenge again.");
            statusLabel = QStringLiteral("LINK RESULT");
            statusValue = victory_ ? QStringLiteral("VICTORY") : QStringLiteral("DEFEAT");
            footer = QStringLiteral("Your linked duel result has been recorded. Run the rematch when both fighters are ready.");
            break;
    }

    const QString statsText = QString(
        "<div style=\"font-family:'Segoe UI'; color:#F5E6D3;\">"
        "<div style=\"font-size:17pt; font-weight:900; color:#FFF0C6; margin-bottom:10px;\">%1</div>"
        "<table width=\"100%%\" cellspacing=\"10\" cellpadding=\"8\">"
        "<tr>"
        "<td style=\"background:#25180F; border:1px solid #9A6F28; border-radius:10px;\">"
        "<span style=\"font-size:9pt; color:#D4A017; font-weight:800;\">%2</span><br>"
        "<span style=\"font-size:20pt; color:%3; font-weight:900;\">%4</span>"
        "</td>"
        "<td style=\"background:#25180F; border:1px solid #9A6F28; border-radius:10px;\">"
        "<span style=\"font-size:9pt; color:#D4A017; font-weight:800;\">POINTS EARNED</span><br>"
        "<span style=\"font-size:20pt; color:#FFD700; font-weight:900;\">%5</span>"
        "</td>"
        "</tr>"
        "<tr>"
        "<td style=\"background:#1C130D; border:1px solid #6E4D20; border-radius:10px;\">"
        "<span style=\"font-size:9pt; color:#D4A017; font-weight:800;\">DAMAGE DEALT</span><br>"
        "<span style=\"font-size:18pt; color:#F5E6D3; font-weight:900;\">%6</span>"
        "</td>"
        "<td style=\"background:#1C130D; border:1px solid #6E4D20; border-radius:10px;\">"
        "<span style=\"font-size:9pt; color:#D4A017; font-weight:800;\">DAMAGE TAKEN</span><br>"
        "<span style=\"font-size:18pt; color:#F5E6D3; font-weight:900;\">%7</span>"
        "</td>"
        "</tr>"
        "</table>"
        "<div style=\"font-size:10pt; color:#D8C49A; margin-top:8px;\">"
        "%8"
        "</div>"
        "</div>"
    ).arg(headline, statusLabel, statusColor, statusValue)
     .arg(score)
     .arg(damageDealt)
     .arg(damageTaken)
     .arg(footer);

    ui->scorePoints->setText(statsText);
}
