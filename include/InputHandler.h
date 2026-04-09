// InputHandler.h - User input handling

#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include <set>

class InputHandler {
private:
    std::set<int> keys;

public:
    void keyDown(int k);
    void keyUp(int k);
    bool isPressed(int k) const;
};

#endif // INPUTHANDLER_H
