#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QString>
#include <QMap>

class QSoundEffect;
class QMediaPlayer;

class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    // Initialize sound manager
    void initialize();

    // Sound effect methods
    void playAttack();        // Sword slash sound
    void playHit();           // Impact/damage sound
    void playMiss();          // Miss/dodge sound
    void playVictory();       // Victory fanfare
    void playDefeat();        // Defeat music
    void playMenuClick();     // UI click sound

    // Background music
    void playBackgroundMusic();
    void stopBackgroundMusic();
    void setMusicVolume(int volume);  // 0-100
    void setSoundVolume(int volume);  // 0-100

    // Utility
    void setMuted(bool muted);
    bool isMuted() const;

private:
    QMap<QString, QSoundEffect*> soundEffects_;
    QMediaPlayer *backgroundMusic_;
    int soundVolume_;
    int musicVolume_;
    bool muted_;

    void loadSound(const QString &name, const QString &path);
};

#endif // SOUNDMANAGER_H
