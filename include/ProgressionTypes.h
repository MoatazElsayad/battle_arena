#ifndef PROGRESSIONTYPES_H
#define PROGRESSIONTYPES_H

#include <QString>
#include <string>

struct PlayerProgression {
    int totalScore = 0;
    int wins = 0;
    int losses = 0;
    int totalMatches = 0;
    double currentRating = 0.0;
    std::string currentRank = "Wanderer";
};

struct UserProfileRecord {
    QString username;
    QString email;
    int totalScore = 0;
    int highScore = 0;
    int matches = 0;
    int wins = 0;
    int losses = 0;
    double currentRating = 0.0;
    QString currentRank;
    QString badge;
    QString avatarPath;
};

#endif // PROGRESSIONTYPES_H
