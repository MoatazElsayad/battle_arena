// DatabaseManager.cpp - Database manager implementation

#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

DatabaseManager::DatabaseManager(const QString& path) : dbPath(path) {}

bool DatabaseManager::connect() {
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    if (!db.open()) {
        return false;
    }
    // Create tables if not exist
    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, password TEXT, avatarId INTEGER)");
    query.exec("CREATE TABLE IF NOT EXISTS scores (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, score INTEGER)");
    return true;
}

bool DatabaseManager::checkCredentials(const QString& username, const QString& password) {
    QSqlQuery query;
    query.prepare("SELECT * FROM users WHERE username = ? AND password = ?");
    query.addBindValue(username);
    query.addBindValue(password);
    if (query.exec() && query.next()) {
        return true;
    }
    return false;
}

void DatabaseManager::saveProfile(const QString& username, int avatarId) {
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO users (username, avatarId) VALUES (?, ?)");
    query.addBindValue(username);
    query.addBindValue(avatarId);
    query.exec();
}

void DatabaseManager::saveResult(const std::string& name, int score) {
    QSqlQuery query;
    query.prepare("INSERT INTO scores (name, score) VALUES (?, ?)");
    query.addBindValue(QString::fromStdString(name));
    query.addBindValue(score);
    query.exec();
}

QList<Score> DatabaseManager::getLeaderboard() {
    QList<Score> leaderboard;
    QSqlQuery query("SELECT name, score FROM scores ORDER BY score DESC LIMIT 10");
    while (query.next()) {
        Score s;
        s.name = query.value(0).toString().toStdString();
        s.score = query.value(1).toInt();
        leaderboard.append(s);
    }
    return leaderboard;
}
