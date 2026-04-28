#include "SoundManager.h"

#include <QSoundEffect>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QtGlobal>

namespace {
QString resolveSoundAssetPath(const QString& relativePath) {
    if (relativePath.isEmpty()) {
        return QString();
    }

    const QFileInfo directInfo(relativePath);
    if (directInfo.isAbsolute() && directInfo.exists()) {
        return directInfo.absoluteFilePath();
    }

    const QStringList candidates = {
        QDir::current().filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../") + relativePath),
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("../../") + relativePath)
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QDir::cleanPath(candidate);
        }
    }

    return QString();
}
}

SoundManager::SoundManager()
    : backgroundMusic_(nullptr),
      audioOutput_(nullptr),
      soundVolume_(80),
      musicVolume_(60),
      muted_(false),
      audioAvailable_(true),
      musicAvailable_(true),
      initialized_(false) {}

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

void SoundManager::ensureInitialized() {
    if (!initialized_) {
        initialize();
    }
}

void SoundManager::initialize() {
    if (initialized_) {
        return;
    }
    initialized_ = true;
    playbackClock_.start();
    musicAvailable_ = true;
    audioAvailable_ = true;

    if (!backgroundMusic_) {
        backgroundMusic_ = new QMediaPlayer();
    }
    if (!audioOutput_) {
        audioOutput_ = new QAudioOutput();
    }

    backgroundMusic_->setAudioOutput(audioOutput_);
    audioOutput_->setVolume(musicVolume_ / 100.0f);
    QObject::connect(backgroundMusic_, &QMediaPlayer::errorOccurred, backgroundMusic_,
                     [this](QMediaPlayer::Error, const QString&) {
                         currentMusicPath_.clear();
                     });

    loadSound("ui_click", "assets/sound/ui/UIclick.wav");
    loadSound("ui_confirm", "assets/sound/ui/confirm.wav");
    loadSound("ui_error", "assets/sound/ui/error.wav");

    loadSound("attack", "assets/sound/combat/attack.wav");
    loadSound("hit", "assets/sound/combat/hit.wav");
    loadSound("heal", "assets/sound/combat/heal.wav");
    loadSound("death", "assets/sound/combat/death.wav");
    loadSound("enemy_death", "assets/sound/combat/enemy_death.wav");
    loadSound("projectile", "assets/sound/combat/projectile.wav");
    loadSound("run", "assets/sound/combat/run.wav");
}

void SoundManager::loadSound(const QString &name, const QString &path) {
    const QString resolvedPath = resolveSoundAssetPath(path);
    if (resolvedPath.isEmpty()) {
        return;
    }

    QSoundEffect *effect = new QSoundEffect();
    effect->setSource(QUrl::fromLocalFile(resolvedPath));
    effect->setLoopCount(1);
    effect->setVolume(soundVolume_ / 100.0f);
    soundEffects_[name] = effect;
}

void SoundManager::playSound(const QString &name) {
    ensureInitialized();
    if (muted_) return;

    auto it = soundEffects_.find(name);
    if (it != soundEffects_.end() && it.value()) {
        if (it.value()->isPlaying()) {
            it.value()->stop();
        }
        it.value()->play();
    }
}

void SoundManager::playMusic(const QString &path) {
    ensureInitialized();
    if (muted_ || !backgroundMusic_ || !audioOutput_) return;
    const QString resolvedPath = resolveSoundAssetPath(path);
    if (resolvedPath.isEmpty()) return;

    if (currentMusicPath_ == resolvedPath &&
        backgroundMusic_->playbackState() == QMediaPlayer::PlayingState) {
        return;
    }

    currentMusicPath_ = resolvedPath;

    backgroundMusic_->stop();
    backgroundMusic_->setSource(QUrl::fromLocalFile(resolvedPath));
    audioOutput_->setVolume(musicVolume_ / 100.0f);
    backgroundMusic_->play();
}

void SoundManager::playMusicCandidates(const QStringList& candidatePaths) {
    ensureInitialized();
    if (muted_ || !backgroundMusic_ || !audioOutput_) {
        return;
    }

    for (const QString& candidate : candidatePaths) {
        const QString resolvedPath = resolveSoundAssetPath(candidate);
        if (resolvedPath.isEmpty()) {
            continue;
        }
        playMusic(candidate);
        return;
    }
}

// MUSIC

void SoundManager::playWelcomeMusic() {
    playMusicCandidates({
        QStringLiteral("assets/sound/music/main_menu.ogg"),
        QStringLiteral("assets/sound/music/main_menu.wav"),
        QStringLiteral("assets/sound/music/main_menu.mp3")
    });
}

void SoundManager::playLobbyMusic() {
    playMusicCandidates({
        QStringLiteral("assets/sound/music/lobby.ogg"),
        QStringLiteral("assets/sound/music/lobby.wav"),
        QStringLiteral("assets/sound/music/lobby.mp3")
    });
}

void SoundManager::playBattleMusic() {
    playMusicCandidates({
        QStringLiteral("assets/sound/music/battle.ogg"),
        QStringLiteral("assets/sound/music/battle.wav"),
        QStringLiteral("assets/sound/music/battle.mp3")
    });
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
