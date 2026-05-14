#ifndef COMBAT_AI_TYPES_H
#define COMBAT_AI_TYPES_H

#include "Enums.h"

#include <QString>

enum class Strategy {
    Aggressive,
    KeepDistance,
    Defensive,
    BaitAttack
};

enum class PreferredAttack {
    Melee,
    Projectile,
    Any
};

struct CombatSnapshot {
    PlayerType playerType = PlayerType::KNIGHT;
    EnemyType enemyType = EnemyType::FIRE_WORM;
    int playerHp = 0;
    int playerMaxHp = 0;
    int enemyHp = 0;
    int enemyMaxHp = 0;
    double distance = 0.0;
    int currentLevel = 1;
    DifficultyLevel difficulty = DifficultyLevel::NORMAL;
    bool projectileAvailable = false;
    bool healAvailable = false;
    QString recentPlayerAction;
    QString recentEnemyAction;
};

struct AiRecommendation {
    Strategy strategy = Strategy::Aggressive;
    PreferredAttack preferredAttack = PreferredAttack::Any;
    int durationMs = 1200;
    bool fromFallback = false;
    QString reason;
};

#endif // COMBAT_AI_TYPES_H
