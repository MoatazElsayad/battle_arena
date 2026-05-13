// Enums.h - Enumeration definitions

#ifndef ENUMS_H
#define ENUMS_H

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

enum class PlayerType {
    ARCEN,
    DEMON_SLAYER,
    FANTASY_WARRIOR,
    HUNTRESS,
    KNIGHT,
    MARTIAL,
    MARTIAL_HERO,
    MEDIEVAL_WARRIOR,
    WIZARD
};

enum class EnemyType {
    FIRE_WORM,
    FIRE_WIZARD,
    FLYING_DEMON,
    NIGHTWEAVER,
    EVIL_WIZARD,
    BLACK_WEREWOLF,
    RED_WEREWOLF,
    WHITE_WEREWOLF,
    ZOMBIE_1,
    ZOMBIE_2,
    ZOMBIE_3,
    ZOMBIE_4,
    ADVANCED_ZOMBIE_1,
    ADVANCED_ZOMBIE_2,
    ADVANCED_ZOMBIE_3
};

enum class DifficultyLevel {
    EASY,
    NORMAL,
    HARD
};

enum class AnimationState {
    IDLE,
    RUN,
    ATTACK1,
    ATTACK2,
    ATTACK3,
    STRONG_ATTACK,
    HURT,
    DEATH,
    DEFEND
};

#endif // ENUMS_H
