#include "main.h"
#include "beeper.h"
static const uint8_t sinewave[] = {
    127, 217, 255, 217, 127, 90, 0, 90,
    127, 217, 255, 217, 127, 90, 0, 90,
    127, 217, 255, 217, 127, 90, 0, 90,
    127, 217, 255, 217, 127, 90, 0, 90,
    127, 217, 255, 217, 127, 90, 0, 90,
    127, 217, 255, 217, 127, 90, 0, 90,
    127, 217, 255, 217, 127, 90, 0, 90,
    127, 217, 255, 217, 127, 90, 0, 90,
    127, 217, 255, 217, 127, 90, 0, 90,
    127, 217, 255, 217, 127, 90, 0, 90};
static void sendSineWave10ms()
{
    ;
}
Beeper beeper;
int beep_remain = 0;
void task_beeper(void *)
{
    while (1)
    {
        if (beep_remain <= 0)
        {
            beep_remain = 0;
            delay(10);
            continue;
        }
        else
        {
            sendSineWave10ms();
            beep_remain--;
        }
    }
}
void Beeper::init()
{
    xTaskCreatePinnedToCore(task_beeper, "beeper", 2048, NULL, 5, NULL, 1);
}
void Beeper::beep(int cnt)
{
    beep_remain = cnt;
}
void Beeper::stop()
{
    beep_remain = 0;
}