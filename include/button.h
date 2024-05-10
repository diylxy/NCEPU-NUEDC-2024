#pragma once

class Button
{
private:
    /* data */
    int m_pin;
    bool m_is_pressed = false;
    int current_pressing_time = 0;
    constexpr static int SHORT_PRESS_TIME = 2;      // 2 ms
    constexpr static int LONG_PRESS_TIME = 60;      // 600 ms
public:
    void init(int pin);
    void update();
    bool isLongPressed();
    bool isPressed();
    int getPressingTime();
    bool waitLongPress();
    void waitRelease();
};
