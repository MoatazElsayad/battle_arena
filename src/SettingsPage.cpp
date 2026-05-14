#include "SettingsPage.h"

#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QStyle>
#include <QVBoxLayout>
#include <QtGlobal>

namespace {
QString pageStyle() {
    return QStringLiteral(
        "QWidget { background-color:#100B08; color:#F7E4B4; }"
        "QLabel#pageEyebrow { color:rgba(245,213,143,0.72); font:800 12px 'Segoe UI'; letter-spacing:2px; }"
        "QLabel#pageTitle { color:#FFD700; font:900 42px 'Showcard Gothic'; letter-spacing:1px; }"
        "QLabel#sectionTitle { color:#F7D774; font:900 15px 'Segoe UI'; letter-spacing:2px; }"
        "QLabel#cardTitle { color:#FFF0C6; font:900 18px 'Segoe UI'; }"
        "QLabel#cardBody { color:rgba(245,230,184,0.76); font:12px 'Segoe UI'; }"
        "QLabel#inputBadge { color:#FFD700; background:rgba(67,44,29,0.88); border:1px solid rgba(212,160,23,0.44); border-radius:18px; font:900 18px 'Segoe UI'; }"
        "QLabel#statusReady { color:#B9F7D0; font:800 12px 'Segoe UI'; letter-spacing:1px; }"
        "QLabel#statusIdle { color:rgba(245,230,184,0.56); font:800 12px 'Segoe UI'; letter-spacing:1px; }"
        "QLabel#percentLabel { color:#FFD700; font:900 15px 'Segoe UI'; min-width:48px; }"
        "QFrame#settingsPanel { background:rgba(27,18,12,0.90); border:1px solid rgba(212,160,23,0.40); border-radius:22px; }"
        "QFrame#audioCard, QFrame#inputCard { background:rgba(35,24,17,0.82); border:1px solid rgba(212,160,23,0.25); border-radius:16px; }"
        "QFrame#inputCard[selected=\"true\"] { background:rgba(38,55,35,0.86); border:2px solid rgba(110,231,183,0.88); }"
        "QFrame#inputCard[selected=\"false\"] { background:rgba(35,24,17,0.68); border:1px solid rgba(212,160,23,0.22); }"
        "QSlider::groove:horizontal { background:#3B2A1D; height:10px; border-radius:5px; }"
        "QSlider::sub-page:horizontal { background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #B91C1C, stop:1 #D4AF37); border-radius:5px; }"
        "QSlider::handle:horizontal { background:#FFF0C6; border:2px solid #D4AF37; width:22px; margin:-7px 0; border-radius:11px; }"
        "QSlider::handle:horizontal:hover { background:#FFD700; }"
        "QPushButton#backButton { background:rgba(67,44,29,0.82); color:#FFF0C6; border:1px solid rgba(212,160,23,0.58); border-radius:14px; padding:13px 24px; font:900 14px 'Segoe UI'; letter-spacing:1px; }"
        "QPushButton#backButton:hover { background:rgba(92,64,43,0.96); border-color:#FFD700; }");
}
}

SettingsPage::SettingsPage(QWidget* parent)
    : QWidget(parent),
      musicVolumeSlider_(nullptr),
      sfxVolumeSlider_(nullptr),
      musicVolumeLabel_(nullptr),
      sfxVolumeLabel_(nullptr),
      musicPercentage_(nullptr),
      sfxPercentage_(nullptr),
      keyboardStatusLabel_(nullptr),
      controllerStatusLabel_(nullptr),
      keyboardCard_(nullptr),
      controllerCard_(nullptr),
      backButton_(nullptr),
      musicVolume_(70),
      sfxVolume_(70),
      difficulty_(DifficultyLevel::NORMAL) {
    loadSettings();
    initializeUI();

    connect(musicVolumeSlider_, &QSlider::valueChanged, this, &SettingsPage::onMusicVolumeChanged);
    connect(sfxVolumeSlider_, &QSlider::valueChanged, this, &SettingsPage::onSfxVolumeChanged);
    connect(backButton_, &QPushButton::clicked, this, &SettingsPage::onBackClicked);
    connect(&inputRefreshTimer_, &QTimer::timeout, this, &SettingsPage::refreshInputStatus);

    inputRefreshTimer_.start(700);
    refreshInputStatus();
}

SettingsPage::~SettingsPage() = default;

void SettingsPage::initializeUI() {
    setStyleSheet(pageStyle());

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(48, 38, 48, 38);
    mainLayout->setSpacing(20);

    auto* eyebrow = new QLabel(QStringLiteral("ARENA SETTINGS"), this);
    eyebrow->setObjectName(QStringLiteral("pageEyebrow"));
    eyebrow->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(eyebrow);

    auto* title = new QLabel(QStringLiteral("Settings"), this);
    title->setObjectName(QStringLiteral("pageTitle"));
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);

    auto* panel = new QFrame(this);
    panel->setObjectName(QStringLiteral("settingsPanel"));
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(30, 28, 30, 28);
    panelLayout->setSpacing(22);

    auto* audioTitle = new QLabel(QStringLiteral("AUDIO CONTROL"), panel);
    audioTitle->setObjectName(QStringLiteral("sectionTitle"));
    panelLayout->addWidget(audioTitle);

    auto* audioRow = new QWidget(panel);
    auto* audioLayout = new QHBoxLayout(audioRow);
    audioLayout->setContentsMargins(0, 0, 0, 0);
    audioLayout->setSpacing(18);
    audioLayout->addWidget(createAudioCard(QStringLiteral("Music"),
                                           QStringLiteral("Controls lobby, welcome, and battle background tracks."),
                                           &musicVolumeSlider_,
                                           &musicPercentage_));
    audioLayout->addWidget(createAudioCard(QStringLiteral("Sound Effects"),
                                           QStringLiteral("Controls attacks, hits, heals, clicks, and combat feedback."),
                                           &sfxVolumeSlider_,
                                           &sfxPercentage_));
    panelLayout->addWidget(audioRow);

    musicVolumeLabel_ = new QLabel(QStringLiteral("Music Volume"), panel);
    sfxVolumeLabel_ = new QLabel(QStringLiteral("Sound Effects Volume"), panel);
    musicVolumeSlider_->setValue(musicVolume_);
    sfxVolumeSlider_->setValue(sfxVolume_);
    musicPercentage_->setText(QStringLiteral("%1%").arg(musicVolume_));
    sfxPercentage_->setText(QStringLiteral("%1%").arg(sfxVolume_));

    auto* inputTitle = new QLabel(QStringLiteral("CURRENT CONTROL SYSTEM"), panel);
    inputTitle->setObjectName(QStringLiteral("sectionTitle"));
    panelLayout->addWidget(inputTitle);

    auto* inputRow = new QWidget(panel);
    auto* inputLayout = new QHBoxLayout(inputRow);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(18);
    keyboardCard_ = createInputCard(QStringLiteral("WASD"),
                                    QStringLiteral("Keyboard"),
                                    QStringLiteral("A/D to move, J/K/L to attack, H to heal, ESC to pause."),
                                    &keyboardStatusLabel_);
    controllerCard_ = createInputCard(QStringLiteral("DS4"),
                                      QStringLiteral("Wireless Controller"),
                                      QStringLiteral("Left stick to move, face buttons to attack, R1 to heal."),
                                      &controllerStatusLabel_);
    inputLayout->addWidget(keyboardCard_);
    inputLayout->addWidget(controllerCard_);
    panelLayout->addWidget(inputRow);

    auto* inputNote = new QLabel(QStringLiteral("The game auto-selects controller when one is connected. Keyboard remains available as backup."), panel);
    inputNote->setObjectName(QStringLiteral("cardBody"));
    inputNote->setWordWrap(true);
    panelLayout->addWidget(inputNote);

    mainLayout->addWidget(panel, 1);

    backButton_ = new QPushButton(QStringLiteral("BACK TO MENU"), this);
    backButton_->setObjectName(QStringLiteral("backButton"));
    backButton_->setMinimumHeight(48);
    mainLayout->addWidget(backButton_, 0, Qt::AlignHCenter);

    setLayout(mainLayout);
}

QFrame* SettingsPage::createAudioCard(const QString& title,
                                      const QString& body,
                                      QSlider** outSlider,
                                      QLabel** outPercentLabel) {
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("audioCard"));
    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(12);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("cardTitle"));
    layout->addWidget(titleLabel);

    auto* bodyLabel = new QLabel(body, card);
    bodyLabel->setObjectName(QStringLiteral("cardBody"));
    bodyLabel->setWordWrap(true);
    layout->addWidget(bodyLabel);

    auto* row = new QWidget(card);
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(14);

    auto* slider = new QSlider(Qt::Horizontal, row);
    slider->setRange(0, 100);
    slider->setMinimumWidth(240);
    auto* percent = new QLabel(row);
    percent->setObjectName(QStringLiteral("percentLabel"));
    percent->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    rowLayout->addWidget(slider, 1);
    rowLayout->addWidget(percent, 0);
    layout->addWidget(row);

    *outSlider = slider;
    *outPercentLabel = percent;
    return card;
}

QFrame* SettingsPage::createInputCard(const QString& badge,
                                      const QString& title,
                                      const QString& body,
                                      QLabel** outStatusLabel) {
    auto* card = new QFrame(this);
    card->setObjectName(QStringLiteral("inputCard"));
    card->setProperty("selected", false);
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(22, 20, 22, 20);
    layout->setSpacing(18);

    auto* badgeLabel = new QLabel(badge, card);
    badgeLabel->setObjectName(QStringLiteral("inputBadge"));
    badgeLabel->setFixedSize(68, 68);
    badgeLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(badgeLabel, 0, Qt::AlignTop);

    auto* copyWrap = new QWidget(card);
    auto* copyLayout = new QVBoxLayout(copyWrap);
    copyLayout->setContentsMargins(0, 0, 0, 0);
    copyLayout->setSpacing(7);

    auto* titleLabel = new QLabel(title, copyWrap);
    titleLabel->setObjectName(QStringLiteral("cardTitle"));
    copyLayout->addWidget(titleLabel);

    auto* bodyLabel = new QLabel(body, copyWrap);
    bodyLabel->setObjectName(QStringLiteral("cardBody"));
    bodyLabel->setWordWrap(true);
    copyLayout->addWidget(bodyLabel);

    auto* statusLabel = new QLabel(copyWrap);
    statusLabel->setObjectName(QStringLiteral("statusIdle"));
    copyLayout->addWidget(statusLabel);

    layout->addWidget(copyWrap, 1);
    *outStatusLabel = statusLabel;
    return card;
}

void SettingsPage::setInputCardSelected(QFrame* card, bool selected) {
    if (!card) {
        return;
    }
    card->setProperty("selected", selected);
    card->style()->unpolish(card);
    card->style()->polish(card);
    card->update();
}

void SettingsPage::refreshInputStatus() {
    controllerInput_.poll();
    const bool controllerConnected = controllerInput_.isAvailable();
    const QString controllerName = controllerInput_.controllerName().trimmed();

    setInputCardSelected(controllerCard_, controllerConnected);
    setInputCardSelected(keyboardCard_, !controllerConnected);

    if (keyboardStatusLabel_) {
        keyboardStatusLabel_->setObjectName(controllerConnected ? QStringLiteral("statusIdle") : QStringLiteral("statusReady"));
        keyboardStatusLabel_->setText(controllerConnected ? QStringLiteral("BACKUP READY") : QStringLiteral("SELECTED"));
        keyboardStatusLabel_->style()->unpolish(keyboardStatusLabel_);
        keyboardStatusLabel_->style()->polish(keyboardStatusLabel_);
    }

    if (controllerStatusLabel_) {
        controllerStatusLabel_->setObjectName(controllerConnected ? QStringLiteral("statusReady") : QStringLiteral("statusIdle"));
        controllerStatusLabel_->setText(controllerConnected
            ? QStringLiteral("SELECTED - %1").arg(controllerName.isEmpty() ? QStringLiteral("Controller") : controllerName)
            : QStringLiteral("NOT CONNECTED"));
        controllerStatusLabel_->style()->unpolish(controllerStatusLabel_);
        controllerStatusLabel_->style()->polish(controllerStatusLabel_);
    }
}

void SettingsPage::loadSettings() {
    QSettings settings(QStringLiteral("Gladiators"), QStringLiteral("Gladiators"));
    musicVolume_ = settings.value(QStringLiteral("audio/musicVolume"), 70).toInt();
    sfxVolume_ = settings.value(QStringLiteral("audio/sfxVolume"), 70).toInt();
    difficulty_ = DifficultyLevel::NORMAL;
}

void SettingsPage::saveSettings() {
    QSettings settings(QStringLiteral("Gladiators"), QStringLiteral("Gladiators"));
    settings.setValue(QStringLiteral("audio/musicVolume"), musicVolume_);
    settings.setValue(QStringLiteral("audio/sfxVolume"), sfxVolume_);
    settings.sync();
}

void SettingsPage::applySettings() {
    saveSettings();
    emit settingsChanged(musicVolume_, sfxVolume_, difficulty_);
}

void SettingsPage::onMusicVolumeChanged(int value) {
    musicVolume_ = value;
    if (musicPercentage_) {
        musicPercentage_->setText(QStringLiteral("%1%").arg(value));
    }
    applySettings();
}

void SettingsPage::onSfxVolumeChanged(int value) {
    sfxVolume_ = value;
    if (sfxPercentage_) {
        sfxPercentage_->setText(QStringLiteral("%1%").arg(value));
    }
    applySettings();
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
    musicVolume_ = qBound(0, volume, 100);
    if (musicVolumeSlider_) {
        musicVolumeSlider_->setValue(musicVolume_);
    }
    if (musicPercentage_) {
        musicPercentage_->setText(QStringLiteral("%1%").arg(musicVolume_));
    }
}

void SettingsPage::setSfxVolume(int volume) {
    sfxVolume_ = qBound(0, volume, 100);
    if (sfxVolumeSlider_) {
        sfxVolumeSlider_->setValue(sfxVolume_);
    }
    if (sfxPercentage_) {
        sfxPercentage_->setText(QStringLiteral("%1%").arg(sfxVolume_));
    }
}

void SettingsPage::setDifficulty(DifficultyLevel level) {
    difficulty_ = level;
}
