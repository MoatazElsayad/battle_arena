#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <string>
#include <vector>

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

    // Ranking teammate:
    // This is the main persistence layer for score/rating/rank.
    // Add simple save/load/update helpers here for player progression.
    // Score management
    // Login page dev: add authenticateUser(username, password) method
    bool saveScore(const std::string& playerName, int score, int level);
    // Login page dev: implement saveResult() if needed - calls saveScore internally
    bool saveResult(const std::string& playerName, int score);
    std::vector<ScoreEntry> getTopScores(int limit = 10);
    bool initializeDatabase();

private:
    // For demo purposes, we'll use in-memory storage
    std::vector<ScoreEntry> scores;
};

#endif // DATABASEMANAGER_H
