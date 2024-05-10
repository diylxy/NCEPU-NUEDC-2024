#include "main.h"
#define MORSE_TOTAL 36

const char *MorseTable[MORSE_TOTAL] = {
    ".-",
    "-...",
    "-.-.",
    "-..",
    ".",
    "..-.",
    "--.",
    "....",
    "..",
    ".---",
    "-.-",
    ".-..",
    "--",
    "-.",
    "---",
    ".--.",
    "--.-",
    ".-.",
    "...",
    "-",
    "..-",
    "...-",
    ".--",
    "-..-",
    "-.--",
    "--..",
    "-----",
    ".----",
    "..---",
    "...--",
    "....-",
    ".....",
    "-....",
    "--...",
    "---..",
    "----."};
const char *AsciiTable = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
char findMorse(const char *morse)
{
    for (int i = 0; i < MORSE_TOTAL; i++)
    {
        if (strcmp(morse, MorseTable[i]) == 0)
        {
            return AsciiTable[i];
        }
    }
    return '\0';
}
char *findMorseReverse(char ascii)
{
    for (int i = 0; i < MORSE_TOTAL; i++)
    {
        if (AsciiTable[i] == ascii)
        {
            return (char *)MorseTable[i];
        }
    }
    return NULL;
}

