#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <vector>
#include "Enums.h"

class BattleWidget;
class DatabaseManager;
class GameManager;
class GamePage;
class GameOverPage;
class PausePage;
class LeaderboardPage;
class SettingsPage;
class SoundManager;
class ProfileLobbyWidget;
class QLabel;
class QLineEdit;
class QListWidget;
class QComboBox;
class ProfilePage;
class QStackedWidget;
class QTimer;
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
    void attemptLogin();
    void attemptRegistration();
    void handleBattleFinished();
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
    QWidget* createProfilePage();
    QWidget* createBattlePage();

    void refreshProfile();
    void refreshBattleView();  // GamePage dev: call this to update battle display
    void showGameOverPage();
    void updateHighScores();
    QString stateTitle() const;

    QStackedWidget *stack_;
    QWidget *loginPage_;
    QWidget *registrationPage_;
    QWidget *welcomePage_;
    QWidget *setupPage_;
    QWidget *profilePage_;
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
    BattleWidget *battleWidget_;
    GamePage *gamePage_;
    QLabel *battleTitleLabel_;
    QLabel *gameOverTitleLabel_;
    QLabel *gameOverSummaryLabel_;

    GameManager *gameManager_;
    DatabaseManager *databaseManager_;
    SoundManager *soundManager_;
    std::vector<PlayerType> sortedPlayerTypes_;
    QString currentLobbyUsername_;
    int currentCharacterIndex_;
    PlayerType selectedPlayerType_;
};

#endif // MAINWINDOW_H
