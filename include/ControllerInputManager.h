#ifndef CONTROLLERINPUTMANAGER_H
#define CONTROLLERINPUTMANAGER_H

#include <QString>

#include <memory>

enum class ControllerButton {
    Attack1,
    Attack2,
    Attack3,
    Jump,
    Heal,
    Pause,
    Count
};

class ControllerInputManager {
public:
    ControllerInputManager();
    ~ControllerInputManager();

    void poll();
    void resetTransientState();

    bool isAvailable() const;
    QString controllerName() const;
    double horizontalAxis() const;
    bool moveLeft() const;
    bool moveRight() const;
    bool wasPressed(ControllerButton button) const;
    bool isPressed(ControllerButton button) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // CONTROLLERINPUTMANAGER_H
