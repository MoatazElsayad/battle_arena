#include "SoundManager.h"

#include <QSoundEffect>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFile>
#include <QUrl>
#include <QtGlobal>

SoundManager::SoundManager()
    : backgroundMusic_(nullptr),
    audioOutput_(nullptr),
    soundVolume_(80),
    musicVolume_(60),
    muted_(false) {}

SoundManager::~SoundManager() {
    for (auto it = soundEffects_.cbegin(); it != soundEffects_.cend(); ++it) {
        QSoundEffect *effect = it.value();
        delete effect;
    }

    soundEffects_.clear();

    if (backgroundMusic_) {
        backgroundMusic_->stop();
        delete backgroundMusic_;
        backgroundMusic_ = nullptr;
    }

    if (audioOutput_) {
        delete audioOutput_;
        audioOutput_ = nullptr;
    }
}
void SoundManager::initialize() {
    backgroundMusic_ = new QMediaPlayer();
    audioOutput_ = new QAudioOutput();

    backgroundMusic_->setAudioOutput(audioOutput_);
    audioOutput_->setVolume(musicVolume_ / 100.0f);

    loadSound("ui_click", "assets/sounds/ui/UIclick.wav");
    loadSound("ui_confirm", "assets/sounds/ui/confirm.wav");
    loadSound("ui_error", "assets/sounds/ui/error.wav");

    loadSound("attack", "assets/sounds/combat/attack.wav");
    loadSound("hit", "assets/sounds/combat/hit.wav");
    loadSound("heal", "assets/sounds/combat/heal.wav");
    loadSound("death", "assets/sounds/combat/death.wav");
    loadSound("enemy_death", "assets/sounds/combat/enemy_death.wav");
    loadSound("projectile", "assets/sounds/combat/projectile.wav");
    loadSound("run", "assets/sounds/combat/run.wav");
}

void SoundManager::loadSound(const QString &name, const QString &path) {
    if (!QFile::exists(path)) {
        return;
    }

    QSoundEffect *effect = new QSoundEffect();
    effect->setSource(QUrl::fromLocalFile(path));
    effect->setVolume(soundVolume_ / 100.0f);
    soundEffects_[name] = effect;
}

void SoundManager::playSound(const QString &name) {
    if (muted_) return;

    auto it = soundEffects_.find(name);
    if (it != soundEffects_.end() && it.value()) {
        it.value()->play();
    }
}

void SoundManager::playMusic(const QString &path) {
    if (muted_ || !backgroundMusic_ || !audioOutput_) return;
    if (!QFile::exists(path)) return;

    if (currentMusicPath_ == path &&
        backgroundMusic_->playbackState() == QMediaPlayer::PlayingState) {
        return;
    }

    currentMusicPath_ = path;

    backgroundMusic_->stop();
    backgroundMusic_->setSource(QUrl::fromLocalFile(path));
    audioOutput_->setVolume(musicVolume_ / 100.0f);
    backgroundMusic_->play();
}

// MUSIC

void SoundManager::playWelcomeMusic() {
    playMusic("assets/sounds/music/main_menu.mp3");
}

void SoundManager::playLobbyMusic() {
    playMusic("assets/sounds/music/lobby.mp3");
}

void SoundManager::playBattleMusic() {
    playMusic("assets/sounds/music/battle.mp3");
}

void SoundManager::stopMusic() {
    if (backgroundMusic_) {
        backgroundMusic_->stop();
    }
}

// UI

void SoundManager::playUIClick() {
    playSound("ui_click");
}

void SoundManager::playUIConfirm() {
    playSound("ui_confirm");
}

void SoundManager::playUIError() {
    playSound("ui_error");
}

// COMBAT

void SoundManager::playAttack() {
    playSound("attack");
}

void SoundManager::playHit() {
    playSound("hit");
}

void SoundManager::playHeal() {
    playSound("heal");
}

void SoundManager::playDeath() {
    playSound("death");
}

void SoundManager::playEnemyDeath() {
    playSound("enemy_death");
}

void SoundManager::playProjectile() {
    playSound("projectile");
}

void SoundManager::playRun() {
    playSound("run");
}

// SYSTEM

void SoundManager::setMusicVolume(int volume) {
    musicVolume_ = qMax(0, qMin(100, volume));

    if (audioOutput_) {
        audioOutput_->setVolume(musicVolume_ / 100.0f);
    }
}

void SoundManager::setSoundVolume(int volume) {
    soundVolume_ = qMax(0, qMin(100, volume));

    for (auto it = soundEffects_.cbegin(); it != soundEffects_.cend(); ++it) {
        QSoundEffect *effect = it.value();

        if (effect) {
            effect->setVolume(soundVolume_ / 100.0f);
        }
    }
}

void SoundManager::setMuted(bool muted) {
    muted_ = muted;

    if (muted_) {
        stopMusic();
    }
}

bool SoundManager::isMuted() const {
    return muted_;
}
