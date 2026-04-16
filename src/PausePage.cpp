#include "PausePage.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>

PausePage::PausePage(QWidget *parent)
    : QWidget(parent) {
    
    setStyleSheet("QWidget { background-color: rgba(42, 24, 16, 200); }");
    setupUI();
}

PausePage::~PausePage() {}

void PausePage::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(20);

    // Title
    QLabel *titleLabel = new QLabel("PAUSED");
    QFont titleFont;
    titleFont.setPointSize(36);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #D4AF37;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    mainLayout->addSpacing(30);

    // Resume button
    QPushButton *resumeButton = new QPushButton("Resume Battle");
    resumeButton->setMinimumWidth(200);
    resumeButton->setMinimumHeight(50);
    resumeButton->setStyleSheet(
        "QPushButton { background-color: #8B0000; color: #F5E6D3; "
        "border: 2px solid #D4AF37; padding: 12px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #A50000; }"
    );
    connect(resumeButton, &QPushButton::clicked, this, &PausePage::resumeClicked);
    mainLayout->addWidget(resumeButton, 0, Qt::AlignCenter);

    // Settings button
    QPushButton *settingsButton = new QPushButton("Settings");
    settingsButton->setMinimumWidth(200);
    settingsButton->setMinimumHeight(50);
    settingsButton->setStyleSheet(
        "QPushButton { background-color: #5C4033; color: #D4AF37; "
        "border: 2px solid #D4AF37; padding: 12px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #6B4D3D; }"
    );
    connect(settingsButton, &QPushButton::clicked, this, &PausePage::settingsClicked);
    mainLayout->addWidget(settingsButton, 0, Qt::AlignCenter);

    // Back to menu button
    QPushButton *menuButton = new QPushButton("Back to Menu");
    menuButton->setMinimumWidth(200);
    menuButton->setMinimumHeight(50);
    menuButton->setStyleSheet(
        "QPushButton { background-color: #3D2817; color: #FFD700; "
        "border: 2px solid #D4AF37; padding: 12px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #4D3817; }"
    );
    connect(menuButton, &QPushButton::clicked, this, &PausePage::menuClicked);
    mainLayout->addWidget(menuButton, 0, Qt::AlignCenter);

    setLayout(mainLayout);
}
