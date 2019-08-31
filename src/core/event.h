#pragma once

#include <GLFW/glfw3.h>

enum class MouseButton : int {
    Left = GLFW_MOUSE_BUTTON_LEFT,
    Right = GLFW_MOUSE_BUTTON_RIGHT,
    Middle = GLFW_MOUSE_BUTTON_MIDDLE,
};

enum class MouseAction : int {
    Press = GLFW_PRESS,
    Release = GLFW_RELEASE,
    Repeat = GLFW_REPEAT,
    Move = GLFW_REPEAT + 1,
};

enum class KeyModifier : int {
    Shift = GLFW_MOD_SHIFT,
    Alt = GLFW_MOD_ALT,
    Control = GLFW_MOD_CONTROL,
    Super = GLFW_MOD_SUPER,
};

class MouseEvent {
public:
    // PUBLIC methods
    MouseEvent() {}

    MouseEvent(MouseButton button, MouseAction action, KeyModifier modifier, double x, double y, int width, int height)
        : button_{ button }
        , action_{ action }
        , modifier_{ modifier }
        , x_{ x }
        , y_{ y }
        , width_{ width }
        , height_{ height } {}

    inline MouseButton button() const { return button_; }

    inline MouseAction action() const { return action_; }

    inline KeyModifier modifier() const { return modifier_; }

    inline double x() const { return x_; }

    inline double y() const { return y_; }

    //! Current window width
    inline int width() const { return width_; }

    //! Current window height
    inline int height() const { return height_; }

private:
    // PRIVATE parameters
    MouseButton button_ = MouseButton(0);
    MouseAction action_ = MouseAction(0);
    KeyModifier modifier_ = KeyModifier(0);
    double x_ = 0.0;
    double y_ = 0.0;
    int width_ = 0;
    int height_ = 0;
};
