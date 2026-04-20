// DatabaseManager.cpp - Database manager implementation

#include "DatabaseManager.h"
#include <algorithm>

// Ranking teammate:
// Do most persistence work for progression in this file.
// Keep it simple:
// - save progression
// - load progression
// - update wins/losses/score/rating/rank

DatabaseManager::DatabaseManager() {
    initializeDatabase();
}

bool DatabaseManager::initializeDatabase() {
    // Initialize with empty scores vector
    scores.clear();
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

    scores.push_back(entry);
    
    // Sort scores in descending order
    std::sort(scores.begin(), scores.end(), 
        [](const ScoreEntry& a, const ScoreEntry& b) {
            return a.score > b.score;
        });

    return true;
}

// Login page dev: implement this - calls saveScore with default level
bool DatabaseManager::saveResult(const std::string& playerName, int score) {
    return saveScore(playerName, score, 1);
}

std::vector<ScoreEntry> DatabaseManager::getTopScores(int limit) {
    std::vector<ScoreEntry> result;
    int count = 0;

    for (const auto& entry : scores) {
        if (count >= limit) break;
        result.push_back(entry);
        count++;
    }

    return result;
}
