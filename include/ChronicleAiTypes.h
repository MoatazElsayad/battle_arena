#ifndef CHRONICLEAITYPES_H
#define CHRONICLEAITYPES_H

#include <QString>
#include <QStringList>

struct ChronicleBattleReport {
    QString playerName;
    QString playerType;
    QString defeatedEnemyName;
    QString defeatedEnemyType;
    QString nextEnemyName;
    QString nextEnemyType;

    int completedLevel = 0;
    int totalLevels = 0;
    int currentScore = 0;
    int playerHp = 0;
    int playerMaxHp = 0;
    int enemyMaxHp = 0;
    int damageDealt = 0;
    int damageTaken = 0;
    int healsUsed = 0;
    int playerAttacks = 0;
    int playerHits = 0;
    int playerMisses = 0;
    int projectilesFired = 0;
    int projectilesHit = 0;
    int enemyHits = 0;
    double battleDurationSeconds = 0.0;
    bool victory = false;
    bool campaignComplete = false;
};

struct ChronicleSummary {
    QString lastLevelSummary;
    QString nextLevelPreview;
    QString recommendation;
    QStringList focusTags;
    bool fromFallback = true;
    bool fromCache = false;
    QString error;
};

#endif // CHRONICLEAITYPES_H
