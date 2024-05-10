#pragma once
#include <Arduino.h>

class Beeper
{
public:
    void init();
    void beep(int cnt);
    void stop();
};

extern Beeper beeper;