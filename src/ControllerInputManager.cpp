#include "ControllerInputManager.h"

#include <QDebug>

#include <array>

namespace {
constexpr int buttonCount() {
    return static_cast<int>(ControllerButton::Count);
}

int buttonIndex(ControllerButton button) {
    return static_cast<int>(button);
}
}

#ifdef GLADIATORS_HAS_SDL2

#include <SDL.h>

namespace {
constexpr Sint16 AXIS_DEADZONE = 9000;

bool controllerButton(SDL_GameController* controller, SDL_GameControllerButton button) {
    return controller && SDL_GameControllerGetButton(controller, button) != 0;
}
}

struct ControllerInputManager::Impl {
    SDL_GameController* controller = nullptr;
    SDL_Joystick* joystick = nullptr;
    SDL_JoystickID controllerId = -1;
    bool sdlInitialized = false;
    bool loggedNoController = false;
    QString name;
    double horizontal = 0.0;
    bool dpadLeft = false;
    bool dpadRight = false;
    std::array<bool, buttonCount()> current{};
    std::array<bool, buttonCount()> previous{};
    std::array<bool, buttonCount()> pressed{};

    Impl() {
        sdlInitialized = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS) == 0;
        if (sdlInitialized) {
            openFirstController();
        }
    }

    ~Impl() {
        closeController();
        if (sdlInitialized) {
            SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS);
        }
    }

    void closeController() {
        if (controller) {
            SDL_GameControllerClose(controller);
            controller = nullptr;
            joystick = nullptr;
        } else if (joystick) {
            SDL_JoystickClose(joystick);
            joystick = nullptr;
        }
        controllerId = -1;
        name.clear();
        horizontal = 0.0;
        dpadLeft = false;
        dpadRight = false;
        current.fill(false);
        previous.fill(false);
        pressed.fill(false);
    }

    void openFirstController() {
        if (controller || joystick || !sdlInitialized) {
            return;
        }

        const int joystickCount = SDL_NumJoysticks();
        if (joystickCount <= 0 && !loggedNoController) {
            qDebug() << "ControllerInputManager: SDL2 is enabled, but no joystick/gamepad devices are visible.";
            loggedNoController = true;
            return;
        }

        for (int i = 0; i < joystickCount; ++i) {
            if (SDL_IsGameController(i)) {
                controller = SDL_GameControllerOpen(i);
                if (!controller) {
                    continue;
                }

                joystick = SDL_GameControllerGetJoystick(controller);
                controllerId = joystick ? SDL_JoystickInstanceID(joystick) : -1;
                const char* controllerName = SDL_GameControllerName(controller);
                name = QString::fromUtf8(controllerName ? controllerName : "Controller");
                qDebug() << "ControllerInputManager: connected game controller" << name;
                return;
            }

            joystick = SDL_JoystickOpen(i);
            if (!joystick) {
                continue;
            }

            controllerId = SDL_JoystickInstanceID(joystick);
            const char* joystickName = SDL_JoystickName(joystick);
            name = QString::fromUtf8(joystickName ? joystickName : "Joystick");
            qDebug() << "ControllerInputManager: connected raw joystick fallback" << name
                     << "axes" << SDL_JoystickNumAxes(joystick)
                     << "buttons" << SDL_JoystickNumButtons(joystick)
                     << "hats" << SDL_JoystickNumHats(joystick);
            return;
        }

        if (!loggedNoController) {
            qDebug() << "ControllerInputManager: SDL2 sees" << joystickCount
                     << "device(s), but none could be opened.";
            loggedNoController = true;
        }
    }

    void resetTransient() {
        pressed.fill(false);
    }

    void handleEvents() {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_CONTROLLERDEVICEADDED) {
                openFirstController();
            } else if (event.type == SDL_CONTROLLERDEVICEREMOVED &&
                       event.cdevice.which == controllerId) {
                closeController();
                openFirstController();
            }
        }
    }

    void updateButtons() {
        previous = current;
        current.fill(false);

        if (controller) {
            current[buttonIndex(ControllerButton::Attack1)] =
                controllerButton(controller, SDL_CONTROLLER_BUTTON_X);
            current[buttonIndex(ControllerButton::Attack2)] =
                controllerButton(controller, SDL_CONTROLLER_BUTTON_Y);
            current[buttonIndex(ControllerButton::Attack3)] =
                controllerButton(controller, SDL_CONTROLLER_BUTTON_B);
            current[buttonIndex(ControllerButton::Jump)] =
                controllerButton(controller, SDL_CONTROLLER_BUTTON_A);
            current[buttonIndex(ControllerButton::Heal)] =
                controllerButton(controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
            current[buttonIndex(ControllerButton::Pause)] =
                controllerButton(controller, SDL_CONTROLLER_BUTTON_START);
        } else if (joystick) {
            const auto rawButton = [this](int index) {
                return index >= 0 && index < SDL_JoystickNumButtons(joystick)
                    && SDL_JoystickGetButton(joystick, index) != 0;
            };

            // Raw DualShock fallback, used when SDL has no mapping for the device.
            current[buttonIndex(ControllerButton::Jump)] = rawButton(0);    // Cross
            current[buttonIndex(ControllerButton::Attack3)] = rawButton(1); // Circle
            current[buttonIndex(ControllerButton::Attack1)] = rawButton(2); // Square
            current[buttonIndex(ControllerButton::Attack2)] = rawButton(3); // Triangle
            current[buttonIndex(ControllerButton::Pause)] = rawButton(6);   // Options
            current[buttonIndex(ControllerButton::Heal)] = rawButton(10);   // R1
        }

        for (int i = 0; i < buttonCount(); ++i) {
            pressed[i] = current[i] && !previous[i];
        }
    }

    void updateMovement() {
        horizontal = 0.0;
        dpadLeft = false;
        dpadRight = false;

        if (!controller && !joystick) {
            return;
        }

        const Sint16 axis = controller
            ? SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX)
            : (SDL_JoystickNumAxes(joystick) > 0 ? SDL_JoystickGetAxis(joystick, 0) : 0);
        if (axis < -AXIS_DEADZONE || axis > AXIS_DEADZONE) {
            horizontal = axis / 32767.0;
        }

        if (controller) {
            dpadLeft = controllerButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
            dpadRight = controllerButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        } else {
            if (SDL_JoystickNumHats(joystick) > 0) {
                const Uint8 hat = SDL_JoystickGetHat(joystick, 0);
                dpadLeft = (hat & SDL_HAT_LEFT) != 0;
                dpadRight = (hat & SDL_HAT_RIGHT) != 0;
            }
            if (!dpadLeft && SDL_JoystickNumButtons(joystick) > 13) {
                dpadLeft = SDL_JoystickGetButton(joystick, 13) != 0;
            }
            if (!dpadRight && SDL_JoystickNumButtons(joystick) > 14) {
                dpadRight = SDL_JoystickGetButton(joystick, 14) != 0;
            }
        }
    }

    void poll() {
        resetTransient();
        if (!sdlInitialized) {
            return;
        }

        handleEvents();
        openFirstController();
        SDL_GameControllerUpdate();

        if (!controller && !joystick) {
            previous = current;
            current.fill(false);
            horizontal = 0.0;
            dpadLeft = false;
            dpadRight = false;
            return;
        }

        updateButtons();
        updateMovement();
    }
};

#else

struct ControllerInputManager::Impl {
    QString name;
    double horizontal = 0.0;
    std::array<bool, buttonCount()> current{};
    std::array<bool, buttonCount()> pressed{};

    void poll() {}
    void resetTransient() {
        pressed.fill(false);
    }
};

#endif

ControllerInputManager::ControllerInputManager()
    : impl_(std::make_unique<Impl>()) {
}

ControllerInputManager::~ControllerInputManager() = default;

void ControllerInputManager::poll() {
    impl_->poll();
}

void ControllerInputManager::resetTransientState() {
    impl_->resetTransient();
}

bool ControllerInputManager::isAvailable() const {
#ifdef GLADIATORS_HAS_SDL2
    return impl_->controller != nullptr || impl_->joystick != nullptr;
#else
    return false;
#endif
}

QString ControllerInputManager::controllerName() const {
    return impl_->name;
}

double ControllerInputManager::horizontalAxis() const {
    return impl_->horizontal;
}

bool ControllerInputManager::moveLeft() const {
#ifdef GLADIATORS_HAS_SDL2
    return impl_->horizontal < -0.35 || impl_->dpadLeft;
#else
    return false;
#endif
}

bool ControllerInputManager::moveRight() const {
#ifdef GLADIATORS_HAS_SDL2
    return impl_->horizontal > 0.35 || impl_->dpadRight;
#else
    return false;
#endif
}

bool ControllerInputManager::wasPressed(ControllerButton button) const {
    const int index = buttonIndex(button);
    return index >= 0 && index < buttonCount() && impl_->pressed[index];
}

bool ControllerInputManager::isPressed(ControllerButton button) const {
    const int index = buttonIndex(button);
    return index >= 0 && index < buttonCount() && impl_->current[index];
}
