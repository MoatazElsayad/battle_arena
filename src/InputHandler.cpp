// InputHandler.cpp - Input handler implementation

#include "InputHandler.h"

void InputHandler::keyDown(int k) {
    keys.insert(k);
}

void InputHandler::keyUp(int k) {
    keys.erase(k);
}

bool InputHandler::isPressed(int k) const {
    return keys.count(k) > 0;
}
