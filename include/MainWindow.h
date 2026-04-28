#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <vector>
#include "Enums.h"
#include "ProgressionTypes.h"

struct ChronicleBattleReport;

class BattleWidget;
class DatabaseManager;
class GameManager;
class GamePage;
class GameOverPage;
class PausePage;
class LeaderboardPage;
class LevelTransitionChroniclePage;
class SettingsPage;
class SoundManager;
class ExhibitionSetupPage;
class ProfileLobbyWidget;
class SaveKingIntroPage;
class LanArenaPage;
class LanSessionManager;
class WebsiteSyncClient;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QComboBox;
class ProfilePage;
class QStackedWidget;
class QTimer;
class QVariantAnimation;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setLoggedInUsername(const QString &username);

private slots:
    void showWelcomePage();
    void showLoginPage();
    void showRegistrationPage();
    void showSetupPage();
    void showSettingsPage();
    void startDemo();
    void startDuelMode();
    void attemptLogin();
    void attemptRegistration();
    void handleBattleFinished();
    void handleChronicleRequested(int completedLevel, bool campaignComplete);
    void continueAfterChronicle();
    void handleSaveKingSceneFinished();
    void restartDemo();

private:
    // 1v1 teammate:
    // MainWindow should gather duel setup from the lobby
    // and launch the correct theme using the existing combat page.
    // Ranking teammate:
    // MainWindow should refresh profile/lobby UI after progression changes.
    void buildUi();
    QWidget* createWelcomePage();
    QWidget* createLoginPage();
    QWidget* createRegistrationPage();
    QWidget* createSetupPage();
    QWidget* createLanArenaPage();
    QWidget* createExhibitionSetupPage();
    QWidget* createProfilePage();
    QWidget* createBattlePage();
    void showLanArenaPage();
    void showExhibitionSetupPage();
    void syncExhibitionSelectionToLobby();

    void refreshProfile();
    void refreshBattleView();  // GamePage dev: call this to update battle display
    void showGameOverPage();
    void updateHighScores();
    QString stateTitle() const;
    void uploadBattleResult(const ChronicleBattleReport& report, bool isDuelMatch) const;
    PlayerProgression applyBattleProgression(const QString& username,
                                            const ChronicleBattleReport& report,
                                            bool isDuelMatch,
                                            int* outReward = nullptr);

    enum class SaveKingSceneAction {
        None,
        StartCampaignBattle,
        ShowVictoryResult
    };

    QStackedWidget *stack_;
    QWidget *loginPage_;
    QWidget *registrationPage_;
    QWidget *welcomePage_;
    QWidget *setupPage_;
    ExhibitionSetupPage *exhibitionSetupPage_;
    QWidget *profilePage_;
    LanArenaPage *lanArenaPage_;
    SaveKingIntroPage *saveKingIntroPage_;
    LevelTransitionChroniclePage *chroniclePage_;
    QWidget *battlePage_;
    GameOverPage *gameOverPage_;
    PausePage *pausePage_;
    LeaderboardPage *leaderboardPage_;
    SettingsPage *settingsPage_;
    ProfileLobbyWidget *profileLobbyWidget_;

    QLineEdit *nameEdit_;
    QComboBox *characterTypeCombo_;
    QLabel *profileNameValue_;
    QLabel *profileLevelValue_;
    QLabel *profileExpValue_;
    QLabel *profileStateValue_;
    QListWidget *scoreList_;
    QLineEdit *loginUsernameEdit_;
    QLineEdit *loginPasswordEdit_;
    QLineEdit *regEmailEdit_;
    QLineEdit *regUsernameEdit_;
    QLineEdit *regPasswordEdit_;
    QLineEdit *regPasswordRepeatEdit_;
    QTimer *welcomeTransitionTimer_;
    QLabel *welcomeLogoLabel_;
    QFrame *welcomeLoadingFill_;
    QVariantAnimation *welcomeLogoFadeAnimation_;
    QVariantAnimation *welcomeLoadingAnimation_;
    BattleWidget *battleWidget_;
    GamePage *gamePage_;
    QLabel *battleTitleLabel_;
    QLabel *gameOverTitleLabel_;
    QLabel *gameOverSummaryLabel_;

    GameManager *gameManager_;
    DatabaseManager *databaseManager_;
    LanSessionManager *lanSessionManager_;
    WebsiteSyncClient *websiteSyncClient_;
    SoundManager *soundManager_;
    std::vector<PlayerType> sortedPlayerTypes_;
    QString currentLobbyUsername_;
    int currentCharacterIndex_;
    PlayerType selectedPlayerType_;
    SaveKingSceneAction saveKingSceneAction_;
    bool chronicleCampaignComplete_;
};

#endif // MAINWINDOW_H
