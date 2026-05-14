#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

#include "ControllerInputManager.h"
#include "Enums.h"

class QFrame;

class SettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsPage(QWidget* parent = nullptr);
    ~SettingsPage();

    int getMusicVolume() const;
    int getSfxVolume() const;
    DifficultyLevel getDifficulty() const;
    
    void setMusicVolume(int volume);
    void setSfxVolume(int volume);
    void setDifficulty(DifficultyLevel level);

signals:
    void backClicked();
    void settingsChanged(int musicVolume, int sfxVolume, DifficultyLevel difficulty);

private slots:
    void onBackClicked();
    void onMusicVolumeChanged(int value);
    void onSfxVolumeChanged(int value);
    void refreshInputStatus();

private:
    void initializeUI();
    void loadSettings();
    void saveSettings();
    void applySettings();
    QFrame* createAudioCard(const QString& title,
                            const QString& body,
                            QSlider** outSlider,
                            QLabel** outPercentLabel);
    QFrame* createInputCard(const QString& badge,
                            const QString& title,
                            const QString& body,
                            QLabel** outStatusLabel);
    void setInputCardSelected(QFrame* card, bool selected);

    QSlider *musicVolumeSlider_;
    QSlider *sfxVolumeSlider_;
    QLabel *musicVolumeLabel_;
    QLabel *sfxVolumeLabel_;
    QLabel *musicPercentage_;
    QLabel *sfxPercentage_;
    QLabel *keyboardStatusLabel_;
    QLabel *controllerStatusLabel_;
    QFrame *keyboardCard_;
    QFrame *controllerCard_;
    QPushButton *backButton_;
    QTimer inputRefreshTimer_;
    ControllerInputManager controllerInput_;
    
    int musicVolume_;
    int sfxVolume_;
    DifficultyLevel difficulty_;
};

#endif // SETTINGSPAGE_H
