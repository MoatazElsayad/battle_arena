#ifndef FIGHTERAIPROFILE_H
#define FIGHTERAIPROFILE_H

#include "Enums.h"

enum class FighterAiPersonality {
    Balanced,
    Berserker,
    Duelist,
    Tank,
    Skirmisher,
    Zoner,
    Trickster,
    Caster
};

enum class FighterAiState {
    Approach,
    HoldRange,
    Retreat,
    Bait,
    Punish,
    MeleeAttack,
    ProjectileAttack,
    Recover
};

struct FighterAiProfile {
    FighterAiPersonality personality = FighterAiPersonality::Balanced;
    int abilityTier = 1;
    int unlockTier = 1;
    int attackCount = 1;
    bool hasProjectile = false;

    double aggression = 0.5;
    double defense = 0.5;
    double spacing = 0.5;
    double projectileBias = 0.0;
    double comboBias = 0.5;
    double punishBias = 0.5;
    double retreatBias = 0.5;
    double randomness = 0.12;
    double idealMinRange = 95.0;
    double idealMaxRange = 165.0;
    double movementBias = 1.0;
};

FighterAiProfile fighterAiProfileFor(PlayerType type);
FighterAiProfile fighterAiProfileFor(EnemyType type);

#endif // FIGHTERAIPROFILE_H
