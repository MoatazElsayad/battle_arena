// DatabaseManager.cpp - Database manager implementation

#include "DatabaseManager.h"
#include <algorithm>
#include <QSettings>

// Ranking teammate:
// Do most persistence work for progression in this file.
// Keep it simple:
// - save progression
// - load progression
// - update wins/losses/score/rating/rank

DatabaseManager::DatabaseManager() {
    initializeDatabase();
}

// Implement the progression persistence logic
bool DatabaseManager::saveProgression(const PlayerProgression& data) {
    QSettings settings("MyGameCompany", "Gladiators");

    settings.beginGroup("PlayerProfile");
    settings.setValue("totalScore", data.totalScore); [cite:79]
        settings.setValue("wins", data.wins); [cite:80]
        settings.setValue("losses", data.losses); [cite:81]
        settings.setValue("totalMatches", data.totalMatches); [cite:82]
        settings.setValue("currentRating", data.currentRating); [cite:84]
        settings.setValue("currentRank", QString::fromStdString(data.currentRank)); [cite:83]
        settings.endGroup();

    currentStats = data; // Keep the internal memory in sync
    return true;
}

// Support loading progression data
PlayerProgression DatabaseManager::loadProgression() {
    QSettings settings("MyGameCompany", "Gladiators");
    PlayerProgression data;

    settings.beginGroup("PlayerProfile");
    data.totalScore = settings.value("totalScore", 0).toInt(); [cite:79]
        data.wins = settings.value("wins", 0).toInt(); [cite:80]
        data.losses = settings.value("losses", 0).toInt(); [cite:81]
        data.totalMatches = settings.value("totalMatches", 0).toInt(); [cite:82]
        data.currentRating = settings.value("currentRating", 0.0).toDouble(); [cite:84]

        // Default to Wanderer if no rank exists
        QString savedRank = settings.value("currentRank", "Wanderer").toString();
    data.currentRank = savedRank.toStdString(); [cite:83]
        settings.endGroup();

    currentStats = data; // Store it locally for easy access
    return data;
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
