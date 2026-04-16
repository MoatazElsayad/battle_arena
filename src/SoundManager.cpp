#include "SoundManager.h"
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QStandardPaths>
#include <QFile>

SoundManager::SoundManager()
    : backgroundMusic_(nullptr),
      soundVolume_(80),
      musicVolume_(60),
      muted_(false) {}

SoundManager::~SoundManager() {
    // Clean up sound effects
    for (auto *effect : soundEffects_) {
        delete effect;
    }
    soundEffects_.clear();

    // Clean up background music
    if (backgroundMusic_) {
        backgroundMusic_->stop();
        delete backgroundMusic_;
    }
}

void SoundManager::initialize() {
    // Create media player for background music
    backgroundMusic_ = new QMediaPlayer();

    // Load sound effects - gracefully handle missing files
    loadSound("attack", "assets/sounds/attack.wav");
    loadSound("hit", "assets/sounds/hit.wav");
    loadSound("miss", "assets/sounds/miss.wav");
    loadSound("victory", "assets/sounds/victory.wav");
    loadSound("defeat", "assets/sounds/defeat.wav");
    loadSound("click", "assets/sounds/click.wav");
}

void SoundManager::loadSound(const QString &name, const QString &path) {
    // Check if file exists
    if (!QFile::exists(path)) {
        // Silently skip if file doesn't exist (development mode)
        return;
    }

    QSoundEffect *effect = new QSoundEffect();
    effect->setSource(QUrl::fromLocalFile(path));
    effect->setVolume(soundVolume_ / 100.0f);
    soundEffects_[name] = effect;
}

void SoundManager::playAttack() {
    if (muted_) return;
    auto it = soundEffects_.find("attack");
    if (it != soundEffects_.end() && *it) {
        (*it)->play();
    }
}

void SoundManager::playHit() {
    if (muted_) return;
    auto it = soundEffects_.find("hit");
    if (it != soundEffects_.end() && *it) {
        (*it)->play();
    }
}

void SoundManager::playMiss() {
    if (muted_) return;
    auto it = soundEffects_.find("miss");
    if (it != soundEffects_.end() && *it) {
        (*it)->play();
    }
}

void SoundManager::playVictory() {
    if (muted_) return;
    auto it = soundEffects_.find("victory");
    if (it != soundEffects_.end() && *it) {
        (*it)->play();
    }
}

void SoundManager::playDefeat() {
    if (muted_) return;
    auto it = soundEffects_.find("defeat");
    if (it != soundEffects_.end() && *it) {
        (*it)->play();
    }
}

void SoundManager::playMenuClick() {
    if (muted_) return;
    auto it = soundEffects_.find("click");
    if (it != soundEffects_.end() && *it) {
        (*it)->play();
    }
}

void SoundManager::playBackgroundMusic() {
    if (muted_ || !backgroundMusic_) return;
    
    backgroundMusic_->setVolume(musicVolume_);
    backgroundMusic_->play();
}

void SoundManager::stopBackgroundMusic() {
    if (backgroundMusic_) {
        backgroundMusic_->stop();
    }
}

void SoundManager::setMusicVolume(int volume) {
    musicVolume_ = qMax(0, qMin(100, volume));
    if (backgroundMusic_) {
        backgroundMusic_->setVolume(musicVolume_);
    }
}

void SoundManager::setSoundVolume(int volume) {
    soundVolume_ = qMax(0, qMin(100, volume));
    for (auto *effect : soundEffects_) {
        if (effect) {
            effect->setVolume(soundVolume_ / 100.0f);
        }
    }
}

void SoundManager::setMuted(bool muted) {
    muted_ = muted;
    if (muted_) {
        stopBackgroundMusic();
    } else {
        playBackgroundMusic();
    }
}

bool SoundManager::isMuted() const {
    return muted_;
}
