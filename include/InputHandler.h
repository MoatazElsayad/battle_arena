/*
 * InputHandler.h - Simple User Input Management
 * Tracks which keys are currently pressed/released
 */

#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include <set>
#include <string>
#include <vector>

#include "Enums.h"

enum class PlayerAction {
    MOVE_LEFT,
    MOVE_RIGHT,
    ATTACK1,
    ATTACK2,
    ATTACK3,
    JUMP,
    HEAL,
    PAUSE
};

struct CharacterFeatureSet {
    int featureImageCount;
    std::vector<std::string> featureNames;
    int attackOptions;
    bool canJump;
    bool canHeal;

    int featureScore() const {
        return featureImageCount;
    }
};

class InputHandler {
public:
    // Common key codes
    static const int KEY_UP = 1000;
    static const int KEY_DOWN = 1001;
    static const int KEY_LEFT = 1002;
    static const int KEY_RIGHT = 1003;
    static const int KEY_W = 119;  // W key
    static const int KEY_A = 97;   // A key
    static const int KEY_S = 115;  // S key
    static const int KEY_D = 100;  // D key
    static const int KEY_H = 104;  // H key
    static const int KEY_J = 106;  // J key
    static const int KEY_K = 107;  // K key
    static const int KEY_L = 108;  // L key
    static const int KEY_SPACE = 32;
    static const int KEY_ENTER = 13;
    static const int KEY_ESC = 27;

public:
    // GamePage dev: use isPressed(KEY_W), wasJustPressed(KEY_SPACE) for game controls
    InputHandler();
    ~InputHandler() = default;

    // Core methods
    void keyDown(int k);
    void keyUp(int k);
    bool isPressed(int k) const;
    bool wasJustPressed(int k) const;
    void update();  // Call once per frame to update "just pressed" tracking
    void reset();

    static CharacterFeatureSet getCharacterFeatures(PlayerType type);
    static bool canPerformAction(PlayerType type, PlayerAction action);
    static int countActionFeatures(PlayerType type);
    static std::vector<std::string> getCharacterFeatureNames(PlayerType type);
    static std::string playerTypeToDisplayName(PlayerType type);
    static std::vector<PlayerType> getCharactersSortedByFeatures();

private:
    std::set<int> keys;         // Currently held keys
    std::set<int> justPressed;  // Keys pressed this frame
};

#endif // INPUTHANDLER_H
