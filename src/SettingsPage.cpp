#include "SettingsPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QFont>
#include <QSettings>

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent)
    , musicVolume_(70)
    , sfxVolume_(70)
    , difficulty_(DifficultyLevel::NORMAL) {
    
    setStyleSheet("QWidget { background-color: #2A1810; }");
    
    // Load saved settings
    loadSettings();
    
    // Initialize UI
    initializeUI();
    
    // Connect signals
    connect(musicVolumeSlider_, QOverload<int>::of(&QSlider::valueChanged),
            this, &SettingsPage::onMusicVolumeChanged);
    connect(sfxVolumeSlider_, QOverload<int>::of(&QSlider::valueChanged),
            this, &SettingsPage::onSfxVolumeChanged);
    connect(difficultyCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsPage::onDifficultyChanged);
    connect(backButton_, &QPushButton::clicked, this, &SettingsPage::onBackClicked);
    connect(applyButton_, &QPushButton::clicked, this, &SettingsPage::applySettings);
}

SettingsPage::~SettingsPage() {
}

void SettingsPage::initializeUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    
    // Title
    QLabel *titleLabel = new QLabel("Settings", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setStyleSheet("color: #D4AF37;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Audio Settings Group
    QGroupBox *audioGroup = new QGroupBox("Audio Settings", this);
    audioGroup->setStyleSheet("QGroupBox { color: #D4AF37; border: 2px solid #D4AF37; "
                             "border-radius: 5px; margin-top: 10px; padding-top: 10px; } "
                             "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }");
    QVBoxLayout *audioLayout = new QVBoxLayout(audioGroup);
    audioLayout->setSpacing(15);
    
    // Music Volume
    QHBoxLayout *musicLayout = new QHBoxLayout();
    musicVolumeLabel_ = new QLabel("Music Volume:", this);
    musicVolumeLabel_->setStyleSheet("color: #D4AF37; font-weight: bold;");
    musicVolumeSlider_ = new QSlider(Qt::Horizontal, this);
    musicVolumeSlider_->setMinimum(0);
    musicVolumeSlider_->setMaximum(100);
    musicVolumeSlider_->setValue(musicVolume_);
    musicVolumeSlider_->setStyleSheet(
        "QSlider::groove:horizontal { background-color: #5C4033; height: 8px; border-radius: 4px; }"
        "QSlider::handle:horizontal { background-color: #D4AF37; width: 18px; margin: -5px 0; border-radius: 9px; }");
    musicPercentage_ = new QLabel(QString("%1%").arg(musicVolume_), this);
    musicPercentage_->setStyleSheet("color: #FFD700; font-weight: bold; min-width: 40px;");
    musicLayout->addWidget(musicVolumeLabel_, 0);
    musicLayout->addWidget(musicVolumeSlider_, 1);
    musicLayout->addWidget(musicPercentage_, 0);
    audioLayout->addLayout(musicLayout);
    
    // SFX Volume
    QHBoxLayout *sfxLayout = new QHBoxLayout();
    sfxVolumeLabel_ = new QLabel("SFX Volume:", this);
    sfxVolumeLabel_->setStyleSheet("color: #D4AF37; font-weight: bold;");
    sfxVolumeSlider_ = new QSlider(Qt::Horizontal, this);
    sfxVolumeSlider_->setMinimum(0);
    sfxVolumeSlider_->setMaximum(100);
    sfxVolumeSlider_->setValue(sfxVolume_);
    sfxVolumeSlider_->setStyleSheet(
        "QSlider::groove:horizontal { background-color: #5C4033; height: 8px; border-radius: 4px; }"
        "QSlider::handle:horizontal { background-color: #D4AF37; width: 18px; margin: -5px 0; border-radius: 9px; }");
    sfxPercentage_ = new QLabel(QString("%1%").arg(sfxVolume_), this);
    sfxPercentage_->setStyleSheet("color: #FFD700; font-weight: bold; min-width: 40px;");
    sfxLayout->addWidget(sfxVolumeLabel_, 0);
    sfxLayout->addWidget(sfxVolumeSlider_, 1);
    sfxLayout->addWidget(sfxPercentage_, 0);
    audioLayout->addLayout(sfxLayout);
    
    mainLayout->addWidget(audioGroup);
    
    // Gameplay Settings Group
    QGroupBox *gameplayGroup = new QGroupBox("Gameplay Settings", this);
    gameplayGroup->setStyleSheet("QGroupBox { color: #D4AF37; border: 2px solid #D4AF37; "
                                "border-radius: 5px; margin-top: 10px; padding-top: 10px; } "
                                "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }");
    QVBoxLayout *gameplayLayout = new QVBoxLayout(gameplayGroup);
    gameplayLayout->setSpacing(15);
    
    // Difficulty
    QHBoxLayout *difficultyLayout = new QHBoxLayout();
    QLabel *difficultyLabel = new QLabel("Difficulty:", this);
    difficultyLabel->setStyleSheet("color: #D4AF37; font-weight: bold;");
    difficultyCombo_ = new QComboBox(this);
    difficultyCombo_->addItem("Easy");
    difficultyCombo_->addItem("Normal");
    difficultyCombo_->addItem("Hard");
    difficultyCombo_->setCurrentIndex(static_cast<int>(difficulty_));
    difficultyCombo_->setStyleSheet(
        "QComboBox { background-color: #5C4033; color: #D4AF37; border: 1px solid #D4AF37; padding: 5px; border-radius: 3px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background-color: #2A1810; color: #D4AF37; selection-background-color: #8B0000; }");
    difficultyLayout->addWidget(difficultyLabel, 0);
    difficultyLayout->addWidget(difficultyCombo_, 1);
    gameplayLayout->addLayout(difficultyLayout);
    
    mainLayout->addWidget(gameplayGroup);
    
    // Spacer
    mainLayout->addStretch();
    
    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);
    
    applyButton_ = new QPushButton("Apply Settings", this);
    applyButton_->setStyleSheet(
        "QPushButton { background-color: #8B0000; color: #D4AF37; border: 2px solid #D4AF37; "
        "padding: 10px 20px; font-weight: bold; border-radius: 5px; }"
        "QPushButton:hover { background-color: #A71930; }"
        "QPushButton:pressed { background-color: #6B0000; }");
    applyButton_->setMinimumHeight(45);
    
    backButton_ = new QPushButton("Back to Menu", this);
    backButton_->setStyleSheet(
        "QPushButton { background-color: #5C4033; color: #D4AF37; border: 2px solid #D4AF37; "
        "padding: 10px 20px; font-weight: bold; border-radius: 5px; }"
        "QPushButton:hover { background-color: #7C6044; }"
        "QPushButton:pressed { background-color: #3C2823; }");
    backButton_->setMinimumHeight(45);
    
    buttonLayout->addWidget(applyButton_);
    buttonLayout->addWidget(backButton_);
    mainLayout->addLayout(buttonLayout);
    
    setLayout(mainLayout);
}

void SettingsPage::loadSettings() {
    QSettings settings("Gladiators", "Gladiators");
    musicVolume_ = settings.value("audio/musicVolume", 70).toInt();
    sfxVolume_ = settings.value("audio/sfxVolume", 70).toInt();
    int diffIndex = settings.value("gameplay/difficulty", 1).toInt();
    difficulty_ = static_cast<DifficultyLevel>(diffIndex);
}

void SettingsPage::saveSettings() {
    QSettings settings("Gladiators", "Gladiators");
    settings.setValue("audio/musicVolume", musicVolume_);
    settings.setValue("audio/sfxVolume", sfxVolume_);
    settings.setValue("gameplay/difficulty", static_cast<int>(difficulty_));
    settings.sync();
}

void SettingsPage::applySettings() {
    saveSettings();
    emit settingsChanged(musicVolume_, sfxVolume_, difficulty_);
}

void SettingsPage::onMusicVolumeChanged(int value) {
    musicVolume_ = value;
    musicPercentage_->setText(QString("%1%").arg(value));
}

void SettingsPage::onSfxVolumeChanged(int value) {
    sfxVolume_ = value;
    sfxPercentage_->setText(QString("%1%").arg(value));
}

void SettingsPage::onDifficultyChanged(int index) {
    difficulty_ = static_cast<DifficultyLevel>(index);
}

void SettingsPage::onBackClicked() {
    emit backClicked();
}

int SettingsPage::getMusicVolume() const {
    return musicVolume_;
}

int SettingsPage::getSfxVolume() const {
    return sfxVolume_;
}

DifficultyLevel SettingsPage::getDifficulty() const {
    return difficulty_;
}

void SettingsPage::setMusicVolume(int volume) {
    musicVolume_ = volume;
    if (musicVolumeSlider_) {
        musicVolumeSlider_->setValue(volume);
    }
}

void SettingsPage::setSfxVolume(int volume) {
    sfxVolume_ = volume;
    if (sfxVolumeSlider_) {
        sfxVolumeSlider_->setValue(volume);
    }
}

void SettingsPage::setDifficulty(DifficultyLevel level) {
    difficulty_ = level;
    if (difficultyCombo_) {
        difficultyCombo_->setCurrentIndex(static_cast<int>(level));
    }
}
