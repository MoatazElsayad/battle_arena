// DatabaseManager.cpp - Database manager implementation

#include "DatabaseManager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>

namespace {
constexpr int kMinimumPasswordLength = 6;
const QRegularExpression kEmailPattern(
    QStringLiteral(R"(^[A-Z0-9._%+\-]+@[A-Z0-9.\-]+\.[A-Z]{2,}$)"),
    QRegularExpression::CaseInsensitiveOption);
}

// Ranking teammate:
// Do most persistence work for progression in this file.
// Keep it simple:
// - save progression
// - load progression
// - update wins/losses/score/rating/rank

DatabaseManager::DatabaseManager() {
    initializeDatabase();
}

bool DatabaseManager::saveProgression(const PlayerProgression& data) {
    QSettings settings("MyGameCompany", "Gladiators");

    settings.beginGroup("PlayerProfile");
    settings.setValue("totalScore", data.totalScore);
    settings.setValue("wins", data.wins);
    settings.setValue("losses", data.losses);
    settings.setValue("totalMatches", data.totalMatches);
    settings.setValue("currentRating", data.currentRating);
    settings.setValue("currentRank", QString::fromStdString(data.currentRank));
    settings.endGroup();

    currentStats_ = data;
    return true;
}

PlayerProgression DatabaseManager::loadProgression() {
    QSettings settings("MyGameCompany", "Gladiators");
    PlayerProgression data;

    settings.beginGroup("PlayerProfile");
    data.totalScore = settings.value("totalScore", 0).toInt();
    data.wins = settings.value("wins", 0).toInt();
    data.losses = settings.value("losses", 0).toInt();
    data.totalMatches = settings.value("totalMatches", 0).toInt();
    data.currentRating = settings.value("currentRating", 0.0).toDouble();
    data.currentRank = settings.value("currentRank", "Wanderer").toString().toStdString();
    settings.endGroup();

    currentStats_ = data;
    return data;
}

bool DatabaseManager::saveProgressionForUser(const QString& username,
                                             const PlayerProgression& data,
                                             QString* errorMessage) {
    UserRecord* user = findUserByUsername(username);
    if (!user) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Account not found for progression update.");
        }
        return false;
    }

    user->totalScore = data.totalScore;
    user->matches = data.totalMatches;
    user->wins = data.wins;
    user->losses = data.losses;
    user->currentRating = data.currentRating;
    user->currentRank = QString::fromStdString(data.currentRank);
    user->badge = badgeForTotalScore(data.totalScore);
    user->highScore = std::max(user->highScore, data.totalScore);

    if (!saveToDisk()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not save updated progression.");
        }
        return false;
    }

    currentStats_ = data;
    return saveProgression(data);
}

PlayerProgression DatabaseManager::loadProgressionForUser(const QString& username) const {
    const UserRecord* user = findUserByUsername(username);
    if (!user) {
        return currentStats_;
    }

    PlayerProgression data;
    data.totalScore = user->totalScore;
    data.wins = user->wins;
    data.losses = user->losses;
    data.totalMatches = user->matches;
    data.currentRating = user->currentRating;
    data.currentRank = user->currentRank.isEmpty()
        ? std::string("Wanderer")
        : user->currentRank.toStdString();
    return data;
}

bool DatabaseManager::initializeDatabase() {
    scores_.clear();
    users_.clear();
    const bool loaded = loadFromDisk();
    currentStats_ = loadProgression();
    return loaded;
}

bool DatabaseManager::registerUser(const QString& email,
                                   const QString& username,
                                   const QString& password,
                                   QString* errorMessage) {
    const QString cleanEmail = email.trimmed();
    const QString cleanUsername = username.trimmed();

    if (cleanEmail.isEmpty() || cleanUsername.isEmpty() || password.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Please fill in email, username, and password.");
        }
        return false;
    }

    if (!kEmailPattern.match(cleanEmail).hasMatch()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Please enter a valid email address.");
        }
        return false;
    }

    if (cleanUsername.size() < 3) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Username must be at least 3 characters long.");
        }
        return false;
    }

    if (password.size() < kMinimumPasswordLength) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Password must be at least 6 characters long.");
        }
        return false;
    }

    const QString usernameKey = normalizedKey(cleanUsername);
    const QString emailKey = normalizedKey(cleanEmail);
    for (const UserRecord& existing : users_) {
        if (normalizedKey(existing.username) == usernameKey) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("That username is already taken.");
            }
            return false;
        }
        if (normalizedKey(existing.email) == emailKey) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("That email is already registered.");
            }
            return false;
        }
    }

    UserRecord user;
    user.username = cleanUsername;
    user.email = cleanEmail;
    user.passwordSalt = generateSalt();
    user.passwordHash = hashPassword(user.passwordSalt, password);
    user.currentRank = QStringLiteral("Wanderer");
    user.currentRating = 0.0;
    user.badge = badgeForTotalScore(0);

    users_.push_back(user);
    if (!saveToDisk()) {
        users_.pop_back();
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not save the new account.");
        }
        return false;
    }

    return true;
}

bool DatabaseManager::authenticateUser(const QString& identity,
                                       const QString& password,
                                       QString* resolvedUsername,
                                       QString* errorMessage) const {
    if (identity.trimmed().isEmpty() || password.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Please enter username/email and password.");
        }
        return false;
    }

    const UserRecord* user = findUserByIdentity(identity);
    if (!user) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("No account matches that username or email.");
        }
        return false;
    }

    const QString enteredHash = hashPassword(user->passwordSalt, password);
    if (enteredHash != user->passwordHash) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Incorrect password.");
        }
        return false;
    }

    if (resolvedUsername) {
        *resolvedUsername = user->username;
    }
    return true;
}

bool DatabaseManager::renameUser(const QString& currentUsername,
                                 const QString& newUsername,
                                 QString* errorMessage) {
    UserRecord* user = findUserByUsername(currentUsername);
    if (!user) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Current account was not found.");
        }
        return false;
    }

    const QString cleanUsername = newUsername.trimmed();
    if (cleanUsername.isEmpty() || cleanUsername.size() < 3) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Username must be at least 3 characters long.");
        }
        return false;
    }

    const QString newKey = normalizedKey(cleanUsername);
    for (const UserRecord& existing : users_) {
        if (&existing == user) {
            continue;
        }
        if (normalizedKey(existing.username) == newKey) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("That username is already taken.");
            }
            return false;
        }
    }

    const QString oldUsername = user->username;
    user->username = cleanUsername;
    for (ScoreEntry& score : scores_) {
        if (QString::fromStdString(score.playerName).compare(oldUsername, Qt::CaseInsensitive) == 0) {
            score.playerName = cleanUsername.toStdString();
        }
    }

    if (!saveToDisk()) {
        user->username = oldUsername;
        for (ScoreEntry& score : scores_) {
            if (QString::fromStdString(score.playerName).compare(cleanUsername, Qt::CaseInsensitive) == 0) {
                score.playerName = oldUsername.toStdString();
            }
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("Could not save the updated username.");
        }
        return false;
    }

    return true;
}

bool DatabaseManager::getUserProfile(const QString& username, UserProfileRecord* outProfile) const {
    const UserRecord* user = findUserByUsername(username);
    if (!user) {
        return false;
    }

    if (outProfile) {
        outProfile->username = user->username;
        outProfile->email = user->email;
        outProfile->totalScore = user->totalScore;
        outProfile->highScore = user->highScore;
        outProfile->matches = user->matches;
        outProfile->wins = user->wins;
        outProfile->losses = user->losses;
        outProfile->currentRating = user->currentRating;
        outProfile->currentRank = user->currentRank.isEmpty()
            ? QStringLiteral("Wanderer")
            : user->currentRank;
        outProfile->badge = user->badge.isEmpty()
            ? badgeForTotalScore(user->totalScore)
            : user->badge;
        outProfile->avatarPath = user->avatarPath;
    }
    return true;
}

bool DatabaseManager::saveScore(const std::string& playerName, int score, int level) {
    // Ranking teammate:
    // Keep leaderboard score saving here, but also add/update long-term player progression nearby.
    if (playerName.empty() || score < 0 || level < 1) {
        return false;
    }

    ScoreEntry entry;
    entry.playerName = playerName;
    entry.score = score;
    entry.level = level;

    scores_.push_back(entry);
    sortScores();

    if (UserRecord* user = findUserByUsername(QString::fromStdString(playerName))) {
        user->highScore = std::max(user->highScore, score);
    }

    return saveToDisk();
}

bool DatabaseManager::saveResult(const std::string& playerName, int score) {
    return saveScore(playerName, score, 1);
}

std::vector<ScoreEntry> DatabaseManager::getTopScores(int limit) const {
    std::vector<ScoreEntry> result;
    int count = 0;

    for (const auto& entry : scores_) {
        if (count >= limit) {
            break;
        }
        result.push_back(entry);
        count++;
    }

    return result;
}

bool DatabaseManager::loadFromDisk() {
    QFile file(storageFilePath());
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!document.isObject()) {
        return false;
    }

    scores_.clear();
    const QJsonArray scoresArray = document.object().value(QStringLiteral("scores")).toArray();
    for (const QJsonValue& value : scoresArray) {
        const QJsonObject object = value.toObject();
        const QString playerName = object.value(QStringLiteral("playerName")).toString().trimmed();
        const int score = object.value(QStringLiteral("score")).toInt(-1);
        const int level = object.value(QStringLiteral("level")).toInt(0);
        if (playerName.isEmpty() || score < 0 || level < 1) {
            continue;
        }

        ScoreEntry entry;
        entry.playerName = playerName.toStdString();
        entry.score = score;
        entry.level = level;
        scores_.push_back(entry);
    }
    sortScores();

    users_.clear();
    const QJsonArray usersArray = document.object().value(QStringLiteral("users")).toArray();
    for (const QJsonValue& value : usersArray) {
        const QJsonObject object = value.toObject();
        const QString username = object.value(QStringLiteral("username")).toString().trimmed();
        const QString email = object.value(QStringLiteral("email")).toString().trimmed();
        const QString salt = object.value(QStringLiteral("passwordSalt")).toString();
        const QString hash = object.value(QStringLiteral("passwordHash")).toString();
        if (username.isEmpty() || email.isEmpty() || salt.isEmpty() || hash.isEmpty()) {
            continue;
        }

        UserRecord user;
        user.username = username;
        user.email = email;
        user.passwordSalt = salt;
        user.passwordHash = hash;
        user.totalScore = object.value(QStringLiteral("totalScore")).toInt(0);
        user.highScore = object.value(QStringLiteral("highScore")).toInt(0);
        user.matches = object.value(QStringLiteral("matches")).toInt(0);
        user.wins = object.value(QStringLiteral("wins")).toInt(0);
        user.losses = object.value(QStringLiteral("losses")).toInt(0);
        user.currentRating = object.value(QStringLiteral("currentRating")).toDouble(0.0);
        user.currentRank = object.value(QStringLiteral("currentRank")).toString(QStringLiteral("Wanderer"));
        user.badge = object.value(QStringLiteral("badge")).toString(badgeForTotalScore(user.totalScore));
        user.avatarPath = object.value(QStringLiteral("avatarPath")).toString();
        users_.push_back(user);
    }

    return true;
}

bool DatabaseManager::saveToDisk() const {
    const QString filePath = storageFilePath();
    const QFileInfo info(filePath);
    QDir directory(info.absolutePath());
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        return false;
    }

    QJsonArray scoresArray;
    for (const ScoreEntry& score : scores_) {
        QJsonObject object;
        object.insert(QStringLiteral("playerName"), QString::fromStdString(score.playerName));
        object.insert(QStringLiteral("score"), score.score);
        object.insert(QStringLiteral("level"), score.level);
        scoresArray.append(object);
    }

    QJsonArray usersArray;
    for (const UserRecord& user : users_) {
        QJsonObject object;
        object.insert(QStringLiteral("username"), user.username);
        object.insert(QStringLiteral("email"), user.email);
        object.insert(QStringLiteral("passwordSalt"), user.passwordSalt);
        object.insert(QStringLiteral("passwordHash"), user.passwordHash);
        object.insert(QStringLiteral("totalScore"), user.totalScore);
        object.insert(QStringLiteral("highScore"), user.highScore);
        object.insert(QStringLiteral("matches"), user.matches);
        object.insert(QStringLiteral("wins"), user.wins);
        object.insert(QStringLiteral("losses"), user.losses);
        object.insert(QStringLiteral("currentRating"), user.currentRating);
        object.insert(QStringLiteral("currentRank"), user.currentRank);
        object.insert(QStringLiteral("badge"), user.badge);
        object.insert(QStringLiteral("avatarPath"), user.avatarPath);
        usersArray.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("scores"), scoresArray);
    root.insert(QStringLiteral("users"), usersArray);

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return file.commit();
}

QString DatabaseManager::storageFilePath() const {
    QString basePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (basePath.trimmed().isEmpty()) {
        basePath = QDir::currentPath();
    }
    return QDir(basePath).filePath(QStringLiteral("accounts.json"));
}

void DatabaseManager::sortScores() {
    std::sort(scores_.begin(), scores_.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) {
                  return a.score > b.score;
              });
}

QString DatabaseManager::normalizedKey(const QString& value) {
    return value.trimmed().toCaseFolded();
}

QString DatabaseManager::generateSalt() {
    QByteArray salt;
    salt.resize(16);
    for (int i = 0; i < salt.size(); ++i) {
        salt[i] = static_cast<char>(QRandomGenerator::global()->generate() & 0xFF);
    }
    return QString::fromLatin1(salt.toHex());
}

QString DatabaseManager::hashPassword(const QString& salt, const QString& password) {
    const QByteArray data = salt.toUtf8() + password.toUtf8();
    const QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    return QString::fromLatin1(hash.toHex());
}

QString DatabaseManager::badgeForTotalScore(int totalScore) {
    if (totalScore >= 12000) {
        return QStringLiteral("Legend");
    }
    if (totalScore >= 5000) {
        return QStringLiteral("Pro");
    }
    if (totalScore >= 1500) {
        return QStringLiteral("Elite");
    }
    return QStringLiteral("Rookie");
}

DatabaseManager::UserRecord* DatabaseManager::findUserByUsername(const QString& username) {
    const QString key = normalizedKey(username);
    for (UserRecord& user : users_) {
        if (normalizedKey(user.username) == key) {
            return &user;
        }
    }
    return nullptr;
}

const DatabaseManager::UserRecord* DatabaseManager::findUserByUsername(const QString& username) const {
    const QString key = normalizedKey(username);
    for (const UserRecord& user : users_) {
        if (normalizedKey(user.username) == key) {
            return &user;
        }
    }
    return nullptr;
}

const DatabaseManager::UserRecord* DatabaseManager::findUserByIdentity(const QString& identity) const {
    const QString key = normalizedKey(identity);
    for (const UserRecord& user : users_) {
        if (normalizedKey(user.username) == key || normalizedKey(user.email) == key) {
            return &user;
        }
    }
    return nullptr;
}
