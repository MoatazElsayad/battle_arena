#ifndef LEADERBOARDPAGE_H
#define LEADERBOARDPAGE_H

#include <QWidget>
#include <vector>
#include <utility>
#include <QString>

class QListWidget;
class DatabaseManager;

class LeaderboardPage : public QWidget {
    Q_OBJECT

public:
    explicit LeaderboardPage(QWidget *parent = nullptr);
    ~LeaderboardPage();

    void setDatabaseManager(DatabaseManager *db);
    void loadScores();

signals:
    void backClicked();

private:
    void setupUI();

    QListWidget *scoreListWidget_;
    DatabaseManager *databaseManager_;
};

#endif // LEADERBOARDPAGE_H
