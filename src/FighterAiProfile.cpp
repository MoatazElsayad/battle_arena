#include "FighterAiProfile.h"

FighterAiProfile fighterAiProfileFor(PlayerType type) {
    switch (type) {
        case PlayerType::KNIGHT:
            return {FighterAiPersonality::Tank, 2, 1, 3, false,
                    0.48, 0.78, 0.58, 0.0, 0.48, 0.56, 0.52, 0.10,
                    92.0, 145.0, 0.92};
        case PlayerType::DEMON_SLAYER:
            return {FighterAiPersonality::Berserker, 4, 6, 3, true,
                    0.76, 0.36, 0.70, 0.86, 0.70, 0.66, 0.58, 0.14,
                    165.0, 360.0, 1.06};
        case PlayerType::FANTASY_WARRIOR:
            return {FighterAiPersonality::Duelist, 3, 5, 3, false,
                    0.66, 0.54, 0.54, 0.0, 0.68, 0.64, 0.42, 0.12,
                    82.0, 142.0, 1.0};
        case PlayerType::HUNTRESS:
            return {FighterAiPersonality::Skirmisher, 4, 7, 3, false,
                    0.58, 0.48, 0.82, 0.0, 0.58, 0.62, 0.72, 0.16,
                    130.0, 205.0, 1.16};
        case PlayerType::ARCEN:
            return {FighterAiPersonality::Zoner, 5, 8, 3, true,
                    0.38, 0.44, 0.96, 0.96, 0.42, 0.60, 0.86, 0.12,
                    220.0, 470.0, 0.96};
        case PlayerType::MARTIAL:
            return {FighterAiPersonality::Duelist, 3, 4, 3, false,
                    0.72, 0.46, 0.48, 0.0, 0.76, 0.74, 0.38, 0.11,
                    70.0, 124.0, 1.16};
        case PlayerType::MARTIAL_HERO:
            return {FighterAiPersonality::Duelist, 3, 3, 3, false,
                    0.68, 0.50, 0.56, 0.0, 0.66, 0.70, 0.48, 0.12,
                    78.0, 132.0, 1.12};
        case PlayerType::MEDIEVAL_WARRIOR:
            return {FighterAiPersonality::Tank, 2, 2, 3, false,
                    0.54, 0.74, 0.52, 0.0, 0.56, 0.58, 0.40, 0.09,
                    84.0, 138.0, 0.94};
        case PlayerType::WIZARD:
            return {FighterAiPersonality::Caster, 5, 9, 3, false,
                    0.38, 0.48, 0.88, 0.0, 0.54, 0.64, 0.82, 0.14,
                    145.0, 240.0, 0.92};
    }

    return {};
}

FighterAiProfile fighterAiProfileFor(EnemyType type) {
    switch (type) {
        case EnemyType::FIRE_WORM:
            return {FighterAiPersonality::Zoner, 2, 0, 3, true,
                    0.30, 0.46, 0.98, 0.98, 0.34, 0.44, 0.92, 0.11,
                    220.0, 490.0, 0.84};
        case EnemyType::FIRE_WIZARD:
            return {FighterAiPersonality::Caster, 3, 0, 3, false,
                    0.46, 0.54, 0.72, 0.0, 0.54, 0.56, 0.62, 0.13,
                    118.0, 220.0, 0.92};
        case EnemyType::FLYING_DEMON:
            return {FighterAiPersonality::Skirmisher, 3, 0, 3, true,
                    0.58, 0.42, 0.78, 0.66, 0.58, 0.60, 0.70, 0.18,
                    132.0, 270.0, 1.08};
        case EnemyType::NIGHTWEAVER:
            return {FighterAiPersonality::Trickster, 4, 0, 3, true,
                    0.52, 0.52, 0.86, 0.72, 0.62, 0.82, 0.76, 0.16,
                    138.0, 300.0, 1.03};
        case EnemyType::EVIL_WIZARD:
            return {FighterAiPersonality::Trickster, 5, 0, 3, false,
                    0.56, 0.58, 0.70, 0.0, 0.70, 0.84, 0.62, 0.12,
                    108.0, 205.0, 1.0};
        case EnemyType::BLACK_WEREWOLF:
            return {FighterAiPersonality::Berserker, 5, 0, 3, false,
                    0.88, 0.34, 0.36, 0.0, 0.76, 0.70, 0.26, 0.14,
                    70.0, 124.0, 1.16};
        case EnemyType::RED_WEREWOLF:
            return {FighterAiPersonality::Berserker, 6, 0, 3, false,
                    0.92, 0.30, 0.34, 0.0, 0.80, 0.76, 0.22, 0.13,
                    68.0, 122.0, 1.20};
        case EnemyType::WHITE_WEREWOLF:
            return {FighterAiPersonality::Berserker, 7, 0, 3, false,
                    0.95, 0.28, 0.34, 0.0, 0.84, 0.82, 0.20, 0.11,
                    66.0, 120.0, 1.24};
        case EnemyType::ZOMBIE_1:
        case EnemyType::ZOMBIE_2:
        case EnemyType::ZOMBIE_3:
        case EnemyType::ZOMBIE_4:
            return {FighterAiPersonality::Berserker, 2, 0, 1, false,
                    0.78, 0.20, 0.22, 0.0, 0.66, 0.48, 0.14, 0.18,
                    58.0, 112.0, 0.86};
        case EnemyType::ADVANCED_ZOMBIE_1:
        case EnemyType::ADVANCED_ZOMBIE_2:
        case EnemyType::ADVANCED_ZOMBIE_3:
            return {FighterAiPersonality::Berserker, 4, 0, 3, false,
                    0.84, 0.26, 0.34, 0.0, 0.74, 0.62, 0.22, 0.13,
                    62.0, 118.0, 1.06};
    }

    return {};
}
