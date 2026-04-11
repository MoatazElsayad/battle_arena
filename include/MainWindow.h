#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <vector>

class BattleWidget;
class DatabaseManager;
class GameManager;
class QLabel;
class QLineEdit;
class QListWidget;
class QStackedWidget;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void showWelcomePage();
    void showSetupPage();
    void startDemo();
    void handleBattleFinished();
    void restartDemo();

private:
    void buildUi();
    QWidget* createWelcomePage();
    QWidget* createSetupPage();
    QWidget* createProfilePage();
    QWidget* createBattlePage();
    QWidget* createGameOverPage();

    void refreshProfile();
    void refreshBattleView();  // GamePage dev: call this to update battle display
    void showGameOverPage();
    void updateHighScores();
    QString stateTitle() const;

    QStackedWidget *stack_;
    QWidget *welcomePage_;
    QWidget *setupPage_;
    QWidget *profilePage_;
    QWidget *battlePage_;
    QWidget *gameOverPage_;

    QLineEdit *nameEdit_;
    QLabel *profileNameValue_;
    QLabel *profileLevelValue_;
    QLabel *profileExpValue_;
    QLabel *profileStateValue_;
    QListWidget *scoreList_;
    BattleWidget *battleWidget_;
    QLabel *battleTitleLabel_;
    QLabel *gameOverTitleLabel_;
    QLabel *gameOverSummaryLabel_;

    GameManager *gameManager_;
    DatabaseManager *databaseManager_;
};

#endif // MAINWINDOW_H
