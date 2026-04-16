#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QWidget>
#include <QSlider>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include "Enums.h"

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
    void onDifficultyChanged(int index);

private:
    void initializeUI();
    void loadSettings();
    void saveSettings();
    void applySettings();

    QSlider *musicVolumeSlider_;
    QSlider *sfxVolumeSlider_;
    QComboBox *difficultyCombo_;
    QLabel *musicVolumeLabel_;
    QLabel *sfxVolumeLabel_;
    QLabel *musicPercentage_;
    QLabel *sfxPercentage_;
    QPushButton *backButton_;
    QPushButton *applyButton_;
    
    int musicVolume_;
    int sfxVolume_;
    DifficultyLevel difficulty_;
};

#endif // SETTINGSPAGE_H
