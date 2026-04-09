// DatabaseManager.h - Database operations for scores

#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>
#include <QList>
#include <string>

struct Score {
    std::string name;
    int score;
};

class DatabaseManager {
private:
    QSqlDatabase db;
    QString dbPath;

public:
    DatabaseManager(const QString& path);
    bool connect();
    bool checkCredentials(const QString& username, const QString& password);
    void saveProfile(const QString& username, int avatarId);
    void saveResult(const std::string& name, int score);
    QList<Score> getLeaderboard();
};

#endif // DATABASEMANAGER_H
