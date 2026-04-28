#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QString>
#include <QMap>
#include <QHash>
#include <QElapsedTimer>

class QSoundEffect;
class QMediaPlayer;
class QAudioOutput;

class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    void initialize();

    // MUSIC
    void playWelcomeMusic();
    void playLobbyMusic();
    void playBattleMusic();
    void stopMusic();

    // UI
    void playUIClick();
    void playUIConfirm();
    void playUIError();

    // COMBAT
    void playAttack();
    void playHit();
    void playHeal();
    void playDeath();
    void playEnemyDeath();
    void playProjectile();
    void playRun();

    // SYSTEM
    void setMusicVolume(int volume);
    void setSoundVolume(int volume);
    void setMuted(bool muted);
    bool isMuted() const;

private:
    QMap<QString, QSoundEffect*> soundEffects_;
    QMediaPlayer *backgroundMusic_;
    QAudioOutput *audioOutput_;
    QString currentMusicPath_;
    int soundVolume_;
    int musicVolume_;
    bool muted_;
    bool audioAvailable_;
    bool musicAvailable_;
    bool initialized_;
    QElapsedTimer playbackClock_;
    QHash<QString, qint64> lastSoundPlayMs_;
    QHash<QString, int> soundThrottleMs_;

    void ensureInitialized();
    void loadSound(const QString &name, const QString &path);
    void playSound(const QString &name);
    void playMusic(const QString &path);
    void playMusicCandidates(const QStringList& candidatePaths);
};

#endif // SOUNDMANAGER_H
