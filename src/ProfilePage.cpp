#include "ProfilePage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QFont>

ProfilePage::ProfilePage(QWidget *parent)
    : QWidget(parent),
      ui(nullptr),
      selectedCharacterType_("Arcen") {
    
    setStyleSheet("QWidget { background-color: #2A1810; } "
                  "QLabel { color: #D4AF37; } "
                  "QComboBox { background-color: #5C4033; color: #F5E6D3; border: 1px solid #D4AF37; padding: 5px; } "
                  "QPushButton { background-color: #8B0000; color: #F5E6D3; border: 2px solid #D4AF37; padding: 8px; font-weight: bold; } "
                  "QPushButton:hover { background-color: #A50000; }");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(20);

    // Title
    QLabel *titleLabel = new QLabel("Player Profile");
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Username display (read-only)
    QHBoxLayout *userLayout = new QHBoxLayout();
    QLabel *userLabel = new QLabel("Name:");
    QLabel *userName = new QLabel();
    userName->setObjectName("userName");
    userName->setStyleSheet("color: #FFD700; font-weight: bold;");
    userLayout->addWidget(userLabel);
    userLayout->addWidget(userName);
    userLayout->addStretch();
    mainLayout->addLayout(userLayout);

    // Score display
    QHBoxLayout *scoreLayout = new QHBoxLayout();
    QLabel *scoreLabel = new QLabel("Score:");
    QLabel *scoreValue = new QLabel("0");
    scoreValue->setObjectName("ScoreLable");
    scoreValue->setStyleSheet("color: #FFD700; font-weight: bold;");
    scoreLayout->addWidget(scoreLabel);
    scoreLayout->addWidget(scoreValue);
    scoreLayout->addStretch();
    mainLayout->addLayout(scoreLayout);

    // Rank display
    QHBoxLayout *rankLayout = new QHBoxLayout();
    QLabel *rankLabel = new QLabel("Rank:");
    QLabel *rankValue = new QLabel("Beginner");
    rankValue->setObjectName("rankLabel");
    rankValue->setStyleSheet("color: #FFD700; font-weight: bold;");
    rankLayout->addWidget(rankLabel);
    rankLayout->addWidget(rankValue);
    rankLayout->addStretch();
    mainLayout->addLayout(rankLayout);

    // Character Type selection
    QHBoxLayout *charLayout = new QHBoxLayout();
    QLabel *charLabel = new QLabel("Select Character:");
    QComboBox *characterCombo = new QComboBox();
    characterCombo->setObjectName("characterCombo");
    characterCombo->addItem("Arcen");
    characterCombo->addItem("Demon Slayer");
    characterCombo->addItem("Fantasy Warrior");
    characterCombo->addItem("Huntress");
    characterCombo->addItem("Knight");
    characterCombo->addItem("Martial");
    characterCombo->addItem("Martial Hero");
    characterCombo->addItem("Medieval Warrior");
    characterCombo->addItem("Wizard");
    characterCombo->setCurrentIndex(0);
    connect(characterCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged), 
            this, &ProfilePage::on_characterCombo_currentTextChanged);
    charLayout->addWidget(charLabel);
    charLayout->addWidget(characterCombo);
    charLayout->addStretch();
    mainLayout->addLayout(charLayout);

    // Difficulty selector
    QHBoxLayout *diffLayout = new QHBoxLayout();
    QLabel *diffLabel = new QLabel("Difficulty:");
    QComboBox *difficultyCombo = new QComboBox();
    difficultyCombo->setObjectName("diffucultySelector");
    difficultyCombo->addItem("Easy");
    difficultyCombo->addItem("Normal");
    difficultyCombo->addItem("Hard");
    connect(difficultyCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged), 
            this, &ProfilePage::on_diffucultySelector_currentTextChanged);
    diffLayout->addWidget(diffLabel);
    diffLayout->addWidget(difficultyCombo);
    diffLayout->addStretch();
    mainLayout->addLayout(diffLayout);

    mainLayout->addSpacing(20);

    // Play button
    QPushButton *playButton = new QPushButton("Play");
    playButton->setObjectName("pushButton");
    connect(playButton, &QPushButton::clicked, this, &ProfilePage::on_pushButton_clicked);
    mainLayout->addWidget(playButton);

    mainLayout->addStretch();
}

ProfilePage::~ProfilePage() {
    // No need to delete ui since we're building programmatically
}

void ProfilePage::setUsername(const QString &username) {
    QLabel *userLabel = findChild<QLabel *>("userName");
    if (userLabel) {
        userLabel->setText(username);
    }
}

void ProfilePage::setScore(int score) {
    QLabel *scoreLabel = findChild<QLabel *>("ScoreLable");
    if (scoreLabel) {
        scoreLabel->setText(QString::number(score));
    }
}

void ProfilePage::setRank(const QString &rank) {
    QLabel *rankLabel = findChild<QLabel *>("rankLabel");
    if (rankLabel) {
        rankLabel->setText(rank);
    }
}

void ProfilePage::setCharacterType(const QString &type) {
    selectedCharacterType_ = type;
    QComboBox *charCombo = findChild<QComboBox *>("characterCombo");
    if (charCombo) {
        charCombo->setCurrentText(type);
    }
}

QString ProfilePage::getCharacterType() const {
    return selectedCharacterType_;
}

void ProfilePage::on_pushButton_clicked() {
    emit playClicked();
}

void ProfilePage::on_diffucultySelector_currentTextChanged(const QString &text) {
    emit difficultyChanged(text);
}

void ProfilePage::on_characterCombo_currentTextChanged(const QString &text) {
    selectedCharacterType_ = text;
    emit characterTypeChanged(text);
}


