#include "FighterAiBrain.h"

#include <algorithm>
#include <cstdlib>

namespace {
double randomUnit() {
    return static_cast<double>(std::rand() % 10000) / 10000.0;
}

bool chance(double probability) {
    return randomUnit() < std::clamp(probability, 0.0, 0.98);
}

double hpRatio(int hp, int maxHp) {
    return maxHp > 0 ? std::clamp(static_cast<double>(hp) / maxHp, 0.0, 1.0) : 0.0;
}

AnimationState chooseAttackState(const FighterAiProfile& profile,
                                 const DifficultyAiTuning& tuning,
                                 const FighterAiContext& context,
                                 bool punishing) {
    const int attackCount = std::clamp(profile.attackCount, 1, 3);
    if (attackCount <= 1) {
        return AnimationState::ATTACK1;
    }

    double attack3Chance = 0.08 + profile.comboBias * 0.22 * tuning.comboMultiplier;
    double attack2Chance = 0.22 + profile.comboBias * 0.30 * tuning.comboMultiplier;

    if (context.difficulty == DifficultyLevel::EASY) {
        attack3Chance *= 0.35;
        attack2Chance *= 0.65;
    } else if (context.difficulty == DifficultyLevel::HARD) {
        attack3Chance += 0.16;
        attack2Chance += 0.10;
    }

    if (punishing || context.playerRecentlyMissed || hpRatio(context.playerHp, context.playerMaxHp) < 0.32) {
        attack3Chance += 0.18 * tuning.comboMultiplier;
        attack2Chance += 0.10 * tuning.comboMultiplier;
    }

    const double roll = randomUnit();
    if (attackCount >= 3 && roll < attack3Chance) {
        return AnimationState::ATTACK3;
    }
    if (roll < attack3Chance + attack2Chance) {
        return AnimationState::ATTACK2;
    }
    return AnimationState::ATTACK1;
}

FighterAiDecision makeDecision(FighterAiState state,
                               const FighterAiProfile& profile,
                               const DifficultyAiTuning& tuning,
                               const FighterAiContext& context,
                               bool wantsAttack,
                               bool wantsProjectile,
                               bool wantsHeal,
                               double moveMultiplier) {
    FighterAiDecision decision;
    decision.state = state;
    decision.wantsAttack = wantsAttack;
    decision.wantsProjectile = wantsProjectile;
    decision.wantsHeal = wantsHeal;
    decision.moveSpeedMultiplier = std::max(0.0, moveMultiplier * profile.movementBias);
    decision.attackCooldownMultiplier = tuning.cooldownMultiplier;
    decision.decisionDuration = tuning.reactionDelay * (0.75 + randomUnit() * 0.55);
    decision.attackState = chooseAttackState(profile, tuning, context, state == FighterAiState::Punish);
    return decision;
}
}

DifficultyAiTuning FighterAiBrain::tuningFor(DifficultyLevel difficulty) {
    switch (difficulty) {
        case DifficultyLevel::EASY:
            return {0.90, 0.26, 0.82, 0.55, 0.55, 0.72, 1.18, 0.88};
        case DifficultyLevel::HARD:
            return {0.32, 0.05, 1.18, 1.45, 1.35, 1.25, 0.86, 1.12};
        case DifficultyLevel::NORMAL:
        default:
            return {0.55, 0.14, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    }
}

FighterAiProfile FighterAiBrain::profileForContext(const FighterAiContext& context) {
    return context.opponentIsPlayerRival
        ? fighterAiProfileFor(context.rivalPlayerType)
        : fighterAiProfileFor(context.enemyType);
}

FighterAiDecision FighterAiBrain::chooseDecision(const FighterAiContext& context) {
    const FighterAiProfile profile = profileForContext(context);
    const DifficultyAiTuning tuning = tuningFor(context.difficulty);
    const double distance = std::max(0.0, context.distance);
    const double idealMin = profile.idealMinRange * tuning.spacingMultiplier;
    const double idealMax = profile.idealMaxRange * tuning.spacingMultiplier;
    const double enemyHealth = hpRatio(context.enemyHp, context.enemyMaxHp);
    const double aggression = profile.aggression * tuning.aggressionMultiplier;
    const double punish = profile.punishBias * tuning.punishMultiplier;
    const double projectile = profile.projectileBias * tuning.projectileMultiplier;
    const double mistake = tuning.mistakeChance + profile.randomness * 0.35;

    if (chance(mistake)) {
        return makeDecision(FighterAiState::Recover, profile, tuning, context, false, false, false, 0.0);
    }

    if (context.enemyRecentlyDamaged && enemyHealth < 0.52) {
        if (context.enemyHealReady && enemyHealth < 0.38
            && chance(profile.defense + profile.retreatBias * 0.25 + 0.18)) {
            return makeDecision(FighterAiState::Retreat, profile, tuning, context, false, false, true, 1.12);
        }
        if (chance(profile.retreatBias + (1.0 - enemyHealth) * 0.45)) {
            return makeDecision(FighterAiState::Retreat, profile, tuning, context, false, false, false, 1.18);
        }
    }

    if ((context.playerRecentlyMissed || context.playerRecentlyHealed)
        && distance <= context.attackRange + 135.0
        && chance(punish + (context.playerRecentlyHealed ? 0.14 : 0.0))) {
        return makeDecision(FighterAiState::Punish, profile, tuning, context, true, false, false, 1.32);
    }

    const bool arcenRival =
        context.opponentIsPlayerRival &&
        context.rivalPlayerType == PlayerType::ARCEN;
    const double projectileMaxRange = arcenRival
        ? (context.attackRange + 430.0) * 3.0
        : idealMax + 150.0;

    if (profile.hasProjectile && context.projectileReady
        && distance > context.attackRange * 0.72
        && distance <= projectileMaxRange
        && chance(projectile + (distance > idealMin ? 0.12 : 0.0))) {
        return makeDecision(FighterAiState::ProjectileAttack, profile, tuning, context, false, true, false, 0.35);
    }

    if (distance <= context.attackRange) {
        const bool berserker = profile.personality == FighterAiPersonality::Berserker;
        const bool rangedPressureFighter =
            profile.hasProjectile &&
            profile.projectileBias > 0.55 &&
            profile.idealMinRange > context.attackRange;
        if ((!berserker || rangedPressureFighter)
            && distance < idealMin * 0.78
            && chance(profile.retreatBias + profile.spacing * 0.18 + (enemyHealth < 0.35 ? 0.20 : 0.0))) {
            return makeDecision(FighterAiState::Retreat, profile, tuning, context, false, false, false, 1.05);
        }
        return makeDecision(FighterAiState::MeleeAttack, profile, tuning, context, true, false, false, 0.45);
    }

    if (distance > idealMax) {
        return makeDecision(FighterAiState::Approach, profile, tuning, context, false, false, false, 1.0 + aggression * 0.35);
    }

    if ((profile.personality == FighterAiPersonality::Trickster
         || profile.personality == FighterAiPersonality::Duelist
         || profile.personality == FighterAiPersonality::Skirmisher)
        && context.playerRecentlyAttacked
        && chance(profile.punishBias + 0.10)) {
        return makeDecision(FighterAiState::Bait, profile, tuning, context, false, false, false, 0.62);
    }

    if (distance < idealMin && chance(profile.retreatBias + profile.spacing * 0.22)) {
        return makeDecision(FighterAiState::Retreat, profile, tuning, context, false, false, false, 0.88);
    }

    if (chance(aggression * 0.35)) {
        return makeDecision(FighterAiState::Approach, profile, tuning, context, false, false, false, 0.72);
    }

    return makeDecision(FighterAiState::HoldRange, profile, tuning, context, false, false, false, 0.55);
}
