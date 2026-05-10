#ifndef COMBATHIGHLIGHTTYPES_H
#define COMBATHIGHLIGHTTYPES_H

#include <QByteArray>
#include <QDateTime>
#include <QString>

enum class HighlightAttackType {
    Attack1,
    Attack2,
    Attack3
};

struct CombatHighlightCandidate {
    HighlightAttackType attackType = HighlightAttackType::Attack1;
    int damage = 0;
    bool wasProjectile = false;
    bool wasFinisher = false;
    int playerHpBefore = 0;
    int playerHpAfter = 0;
    int playerMaxHp = 0;
    int enemyHpBefore = 0;
    int enemyHpAfter = 0;
};

struct CombatHighlightSnapshot {
    HighlightAttackType attackType = HighlightAttackType::Attack1;
    int damage = 0;
    bool wasProjectile = false;
    bool wasFinisher = false;
    int highlightScore = 0;
    int playerHpBefore = 0;
    int playerHpAfter = 0;
    int playerMaxHp = 0;
    int enemyHpBefore = 0;
    int enemyHpAfter = 0;
    QDateTime capturedAtUtc;
    QByteArray imageBytes;
    QString imageMimeType;
};

QString highlightAttackTypeApiKey(HighlightAttackType attackType);
QString highlightAttackTypeDisplayName(HighlightAttackType attackType, bool wasProjectile);

#endif // COMBATHIGHLIGHTTYPES_H
