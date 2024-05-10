#pragma once
#include <Arduino.h>
#include <driver/dac.h>
#include "ADC/ADCSampler.h"
#include <opus.h>
#define AUDIO_SAMPLE_RATE     8000  // 44100
#define AUDIO_OPUS_FRAME_MS   40    // one of 2.5, 5, 10, 20, 40, 60, 80, 100, 120
#define AUDIO_OPUS_BITRATE    3200  // bit rate from 2400 to 512000
#define AUDIO_OPUS_COMPLEXITY 0     // from 0 to 10
void audio_codec_init();
void audio_dac_init();
void audio_adc_init();
uint8_t *audio_encode_packet(int *len);
void audio_decode_packet(uint8_t *data, int len, uint8_t *output_buffer, int *output_size);