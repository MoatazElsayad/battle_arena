#ifndef FIGHTERAIBRAIN_H
#define FIGHTERAIBRAIN_H

#include "FighterAiProfile.h"

struct DifficultyAiTuning {
    double reactionDelay = 0.55;
    double mistakeChance = 0.14;
    double aggressionMultiplier = 1.0;
    double punishMultiplier = 1.0;
    double comboMultiplier = 1.0;
    double projectileMultiplier = 1.0;
    double cooldownMultiplier = 1.0;
    double spacingMultiplier = 1.0;
};

struct FighterAiContext {
    bool opponentIsPlayerRival = false;
    PlayerType rivalPlayerType = PlayerType::KNIGHT;
    EnemyType enemyType = EnemyType::FIRE_WORM;
    DifficultyLevel difficulty = DifficultyLevel::NORMAL;

    double distance = 0.0;
    double attackRange = 120.0;
    int playerHp = 0;
    int playerMaxHp = 0;
    int enemyHp = 0;
    int enemyMaxHp = 0;
    bool projectileReady = false;
    bool enemyHealReady = false;
    bool playerRecentlyAttacked = false;
    bool playerRecentlyMissed = false;
    bool playerRecentlyHealed = false;
    bool enemyRecentlyDamaged = false;
};

struct FighterAiDecision {
    FighterAiState state = FighterAiState::Approach;
    AnimationState attackState = AnimationState::ATTACK1;
    bool wantsAttack = false;
    bool wantsProjectile = false;
    bool wantsHeal = false;
    double moveSpeedMultiplier = 1.0;
    double attackCooldownMultiplier = 1.0;
    double decisionDuration = 0.55;
};

class FighterAiBrain {
public:
    static FighterAiDecision chooseDecision(const FighterAiContext& context);
    static DifficultyAiTuning tuningFor(DifficultyLevel difficulty);
    static FighterAiProfile profileForContext(const FighterAiContext& context);
};

#endif // FIGHTERAIBRAIN_H
