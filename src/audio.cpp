#include "main.h"

const int mode=CODEC2_MODE_1600;
const int natural=1;

CODEC2 *codec2;
int codec_samples_cnt, codec_encoded_bits_cnt, codec_encoded_byte, i, frames, bits_proc, bit_errors, error_mode;
int codec_encoded_bits_start, codec_encoded_bits_end, bit_rate;
short *codec_decoded_buffer;
unsigned char *codec_encoded_buffer;
float *softdec_bits;

ADCSampler *adcSampler = NULL;
// i2s config for using the internal ADC
i2s_config_t adcI2SConfig = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_LSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 32,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};
void audio_codec_init()
{
  codec2 = codec2_create(mode);
  codec_samples_cnt = codec2_samples_per_frame(codec2);
  codec_encoded_bits_cnt = codec2_bits_per_frame(codec2);
  codec_decoded_buffer = (short*)malloc(codec_samples_cnt*sizeof(short));
  codec_encoded_byte = (codec_encoded_bits_cnt + 7) / 8;
  codec_encoded_buffer = (unsigned char*)malloc(codec_encoded_byte*sizeof(char));
  frames = bit_errors = bits_proc = 0;
  codec_encoded_bits_start = 0;
  codec_encoded_bits_end = codec_encoded_bits_cnt-1;

  codec2_set_natural_or_gray(codec2, !natural);  
}
void audio_dac_init()
{
}
void audio_adc_init()
{
    adcSampler = new ADCSampler(ADC_UNIT_1, ADC1_CHANNEL_4, adcI2SConfig);
    adcSampler->start();
}
#include "sampleaudio.h"
static int current_pos = 0;
uint8_t *audio_encode_packet(int *len)
{
    //adcSampler->read(codec_decoded_buffer, codec_samples_cnt);
    memcpy(codec_decoded_buffer, __out_pcm + current_pos, codec_samples_cnt * 2);
    current_pos += codec_samples_cnt * 2;
    if(current_pos >= __out_pcm_len)
        current_pos = 0;
    codec2_encode(codec2, codec_encoded_buffer, codec_decoded_buffer);
    *len = codec_encoded_byte;
    return codec_encoded_buffer;
}

void audio_decode_packet(uint8_t *data, int len, uint8_t *output_buffer, int *output_size)
{
    if(len != codec_encoded_byte)
    {
        Serial.println("error");
        return;
    }
    codec2_decode(codec2, codec_decoded_buffer, data);
    size_t bytes_written = 0;
    for (int i = 0; i < codec_samples_cnt; i++)
    {
        output_buffer[i] = (codec_decoded_buffer[i] + 32768) >> 8;
    }
    *output_size = codec_samples_cnt;
}
