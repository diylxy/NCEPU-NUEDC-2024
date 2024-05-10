#include "main.h"
OpusEncoder *opus_encoder_;
OpusDecoder *opus_decoder_;

int16_t *opus_samples_;
int opus_samples_size_;

int16_t *opus_samples_out_;
int opus_samples_out_size_;

uint8_t *opus_bits_;
int opus_bits_size_ = 1024;

void audio_codec_init()
{
    // configure encoder
    int encoder_error;
    opus_encoder_ = opus_encoder_create(AUDIO_SAMPLE_RATE, 1, OPUS_APPLICATION_AUDIO, &encoder_error);
    if (encoder_error != OPUS_OK)
    {
        Serial.printf("Failed to create OPUS encoder, error", encoder_error);
        return;
    }
    encoder_error = opus_encoder_init(opus_encoder_, AUDIO_SAMPLE_RATE, 1, OPUS_APPLICATION_AUDIO);
    if (encoder_error != OPUS_OK)
    {
        Serial.printf("Failed to initialize OPUS encoder, error", encoder_error);
        return;
    }
    opus_encoder_ctl(opus_encoder_, OPUS_SET_BITRATE(AUDIO_OPUS_BITRATE));
    opus_encoder_ctl(opus_encoder_, OPUS_SET_COMPLEXITY(AUDIO_OPUS_COMPLEXITY));
    opus_encoder_ctl(opus_encoder_, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    // configure decoder
    int decoder_error;
    opus_decoder_ = opus_decoder_create(AUDIO_SAMPLE_RATE, 1, &decoder_error);
    if (decoder_error != OPUS_OK)
    {
        Serial.printf("Failed to create OPUS decoder, error", decoder_error);
        return;
    }

    opus_samples_size_ = (int)(AUDIO_SAMPLE_RATE / 1000 * AUDIO_OPUS_FRAME_MS);
    opus_samples_ = (int16_t *)malloc(sizeof(int16_t) * opus_samples_size_);
    opus_samples_out_size_ = 10 * opus_samples_size_;
    opus_samples_out_ = (int16_t *)malloc(sizeof(int16_t) * opus_samples_out_size_);

    opus_bits_ = (uint8_t *)malloc(sizeof(uint8_t) * opus_bits_size_);
}

uint8_t *audio_encode_packet(int *len)
{
    static bool firstadc = false;
    if(firstadc == false)
    {
        aaAudio.getADC(opus_samples_size_);
        firstadc = true;
    }
    aaAudio.getADC(opus_samples_size_);
    for(int i = 0; i < opus_samples_size_; i++)
    {
        Serial.printf("%d, ", aaAudio.adcBuffer16[i]);
        opus_samples_[i] = (int16_t)aaAudio.adcBuffer16[i] - 24823;
    }
    *len = opus_encode(opus_encoder_, opus_samples_, opus_samples_size_, opus_bits_, opus_bits_size_);
    return opus_bits_;
}

void audio_decode_packet(uint8_t *data, int len, uint8_t *output_buffer, int *output_size)
{
    int decoded_size = opus_decode(opus_decoder_, data, len, opus_samples_out_, opus_samples_out_size_, 0);
    size_t bytes_written = 0;
    for (int i = 0; i < decoded_size; i++)
    {
        output_buffer[i] = (opus_samples_out_[i] + 32768) >> 8;
    }
    if (decoded_size > 0)
        *output_size = decoded_size;
}
