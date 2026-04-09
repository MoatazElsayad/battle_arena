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
    TANK,
    FAST,
    BALANCED
};

enum class DifficultyLevel {
    EASY,
    NORMAL,
    HARD
};

#endif // ENUMS_H
