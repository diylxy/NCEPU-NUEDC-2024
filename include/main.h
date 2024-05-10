#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include "AutoAnalogAudio.h"
#include "protocol.h"
#include "button.h"
#include "audio.h"
#include "beeper.h"
// #define DEVICE_IS_RECEIVER
#define STOP_BIT_LEN_2
// #define TX_DEBUG_MODE
// #define RX_DEBUG_MODE

#define PIN_DUMMY 2
#ifdef DEVICE_IS_RECEIVER
#define PIN_RX2 27 // 用于接收端接收串口数据
#else
#define OUTPUT_PIN 27
#define PLL_PIN 14
#define OUT_LEVEL_0 0
#define OUT_LEVEL_1 1
#ifdef TX_DEBUG_MODE
#define PIN_RX2 27 // 用于接收端接收串口数据
#endif
#endif

#define PIN_OLED_CS 19
#define PIN_OLED_DC 5
#define PIN_OLED_RST 18
#define PIN_OLED_CLK 17  // D0
#define PIN_OLED_MOSI 16 // D1

#define PIN_LED1 23
#define PIN_LED2 22

#define PIN_KEY1 4
#define PIN_KEY2 13

#define PIN_MIC 32

extern U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2;

extern int total_opus_pkt;
void ledRed_on();
void ledRed_off();
void ledGreen_on();
void ledGreen_off();

void playAudioBuffer();
void appendOpusPacket(uint8_t *pkt, size_t size);
void resetOpusList();
/**
 * @brief 根据摩尔斯电码查找对应的字符，找不到则返回0
*/
char findMorse(const char *morse);
/**
 * @brief 根据字符查找对应的摩尔斯电码，找不到则返回NULL
*/
char *findMorseReverse(char ascii);
#define LONG_THRESHOLD 200
#define GAP_THRESHOLD 450

extern AutoAnalog aaAudio;