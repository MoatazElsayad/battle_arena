// Enums.h - Enumeration definitions

#ifndef ENUMS_H
#define ENUMS_H

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

enum class CharacterType {
    ARCEN,
    DEMON_SLAYER,
    FANTASY_WARRIOR,
    HUNTRESS,
    KNIGHT,
    MARTIAL,
    MARTIAL_HERO,
    MEDIEVAL_WARRIOR,
    WIZARD,
    FLYING_DEMON
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
