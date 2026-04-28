#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <string>
#include <vector>

#include "ProgressionTypes.h"

struct ScoreEntry {
    std::string playerName;
    int score;
    int level;
};

// Ranking teammate:
// Expand DatabaseManager to store progression data, not only top scores.
// Main data to support:
// - total score
// - wins
// - losses
// - matches
// - rating out of 5
// - current rank
class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager() = default;

    bool saveProgression(const PlayerProgression& data);
    PlayerProgression loadProgression();
    bool saveProgressionForUser(const QString& username,
                                const PlayerProgression& data,
                                QString* errorMessage = nullptr);
    PlayerProgression loadProgressionForUser(const QString& username) const;

    // Ranking teammate:
    // This is the main persistence layer for score/rating/rank.
    // Add simple save/load/update helpers here for player progression.
    // Score management
    bool registerUser(const QString& email,
                      const QString& username,
                      const QString& password,
                      QString* errorMessage = nullptr);
    bool authenticateUser(const QString& identity,
                          const QString& password,
                          QString* resolvedUsername = nullptr,
                          QString* errorMessage = nullptr) const;
    bool renameUser(const QString& currentUsername,
                    const QString& newUsername,
                    QString* errorMessage = nullptr);
    bool getUserProfile(const QString& username, UserProfileRecord* outProfile) const;
    bool saveScore(const std::string& playerName, int score, int level);
    // Login page dev: implement saveResult() if needed - calls saveScore internally
    bool saveResult(const std::string& playerName, int score);
    std::vector<ScoreEntry> getTopScores(int limit = 10) const;
    bool initializeDatabase();

private:
    struct UserRecord {
        QString username;
        QString email;
        QString passwordSalt;
        QString passwordHash;
        int totalScore = 0;
        int highScore = 0;
        int matches = 0;
        int wins = 0;
        int losses = 0;
        double currentRating = 0.0;
        QString currentRank = QStringLiteral("Wanderer");
        QString badge = QStringLiteral("Rookie");
        QString avatarPath;
    };

    bool loadFromDisk();
    bool saveToDisk() const;
    QString storageFilePath() const;
    void sortScores();

    static QString normalizedKey(const QString& value);
    static QString generateSalt();
    static QString hashPassword(const QString& salt, const QString& password);
    static QString badgeForTotalScore(int totalScore);

    UserRecord* findUserByUsername(const QString& username);
    const UserRecord* findUserByUsername(const QString& username) const;
    const UserRecord* findUserByIdentity(const QString& identity) const;

    std::vector<ScoreEntry> scores_;
    std::vector<UserRecord> users_;
    PlayerProgression currentStats_;
};

#endif // DATABASEMANAGER_H
