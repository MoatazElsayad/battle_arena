#include "LeaderboardPage.h"
#include "DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QFont>

LeaderboardPage::LeaderboardPage(QWidget *parent)
    : QWidget(parent),
      databaseManager_(nullptr) {
    
    setStyleSheet("QWidget { background-color: #2A1810; }");
    setupUI();
}

LeaderboardPage::~LeaderboardPage() {}

void LeaderboardPage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(20);

    // Title
    QLabel *titleLabel = new QLabel("Leaderboard");
    QFont titleFont;
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #D4AF37;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Score list
    scoreListWidget_ = new QListWidget();
    scoreListWidget_->setStyleSheet(
        "QListWidget { background-color: #3D2817; border: 2px solid #D4AF37; }"
        "QListWidget::item { color: #D4AF37; padding: 8px; }"
        "QListWidget::item:selected { background-color: #5C4033; }"
    );
    mainLayout->addWidget(scoreListWidget_);

    // Back button
    QPushButton *backButton = new QPushButton("Back");
    backButton->setStyleSheet(
        "QPushButton { background-color: #8B0000; color: #F5E6D3; "
        "border: 2px solid #D4AF37; padding: 10px; font-weight: bold; }"
        "QPushButton:hover { background-color: #A50000; }"
    );
    backButton->setMaximumWidth(150);
    connect(backButton, &QPushButton::clicked, this, &LeaderboardPage::backClicked);
    mainLayout->addWidget(backButton, 0, Qt::AlignCenter);

    setLayout(mainLayout);
}

void LeaderboardPage::setDatabaseManager(DatabaseManager *db) {
    databaseManager_ = db;
}

void LeaderboardPage::loadScores() {
    scoreListWidget_->clear();
    
    if (!databaseManager_) {
        QListWidgetItem *item = new QListWidgetItem("No database available");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        scoreListWidget_->addItem(item);
        return;
    }

    // Placeholder: Load scores from database
    // For now, display a message
    QListWidgetItem *item = new QListWidgetItem("Scores will appear here");
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    scoreListWidget_->addItem(item);
}
