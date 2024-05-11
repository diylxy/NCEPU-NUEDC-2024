#pragma once
#include <Arduino.h>
#include <driver/dac.h>
#include "ADC/ADCSampler.h"
#include "codec2.h"

void audio_codec_init();
void audio_dac_init();
void audio_adc_init();
uint8_t *audio_encode_packet(int *len);
void audio_decode_packet(uint8_t *data, int len, uint8_t *output_buffer, int *output_size);

void audio_adc_manual_init();
void audio_adc_manual_reset();
void audio_adc_continue_40ms();
uint8_t *audio_encode_packet_manual(int *len);
