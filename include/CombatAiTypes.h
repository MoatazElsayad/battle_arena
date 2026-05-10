// Shared combat AI data only.
// Add:
// - CombatSnapshot: current battle state sent to the advisor
// - AiRecommendation: short tactical result returned to combat
// Keep both structs small and simple.
#ifndef COMBAT_AI_TYPES_H
#define COMBAT_AI_TYPES_H

#include "Enums.h"
#include <QString>

enum class Strategy 
{
    Aggressive,
    KeepDistance,
    Defensive,
    BaitAttack
};

enum class PreferredAttack 
{
    Melee,
    Projectile,
    Any
};



struct CombatSnapshot
 {
    CharacterType playerType;
    CharacterType enemyType;
    int playerHp;
    int enemyHp;
    double distance;
    int currentLevel;
    bool projectileAvailable;
    QString recentPlayerAction;
    QString recentEnemyAction;
};

struct AiRecommendation 
{
    Strategy strategy;
    PreferredAttack preferredAttack;
    int durationMs;
    bool fromFallback;
    QString reason;
};

