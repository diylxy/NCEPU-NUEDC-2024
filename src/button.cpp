#include "main.h"
#include "button.h"

void Button::init(int pin)
{
    m_pin = pin;
    pinMode(pin, INPUT_PULLUP);
}

void Button::update()
{
    if (digitalRead(m_pin) == LOW)
    {
        if (m_is_pressed == false)
        {
            m_is_pressed = true;
            current_pressing_time = 0;
        }
        else
            current_pressing_time++;
    }
    else
    {
        m_is_pressed = false;
        current_pressing_time = 0;
    }
}

bool Button::isLongPressed()
{
    if (m_is_pressed && current_pressing_time >= LONG_PRESS_TIME)
        return true;
    return false;
}

bool Button::isPressed()
{
    if (m_is_pressed && current_pressing_time >= SHORT_PRESS_TIME && digitalRead(m_pin) == 0)
        return true;
    return false;
}

int Button::getPressingTime()
{
    return current_pressing_time;
}

bool Button::waitLongPress()
{
    while (digitalRead(m_pin) == LOW)
    {
        if (current_pressing_time >= LONG_PRESS_TIME)
            return true;
        delay(10);
    }
    return false;
}

void Button::waitRelease()
{
    while (digitalRead(m_pin) == LOW)
    {
        delay(10);
    }
    delay(10);
}
