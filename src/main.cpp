#include "main.h"
#include "esp_task_wdt.h"
AutoAnalog aaAudio;
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/PIN_OLED_CS, /* dc=*/PIN_OLED_DC, /* reset=*/PIN_OLED_RST);
Button buttonMod, buttonTX;
void ledRed_on()
{
    digitalWrite(PIN_LED1, HIGH);
}
void ledRed_off()
{
    digitalWrite(PIN_LED1, LOW);
}
void ledGreen_on()
{
    digitalWrite(PIN_LED2, HIGH);
}
void ledGreen_off()
{
    digitalWrite(PIN_LED2, LOW);
}
void task_button(void *)
{
    while (1)
    {
        buttonMod.update();
        buttonTX.update();
        delay(10);
    }
}
SemaphoreHandle_t audio_start_decode;
uint8_t *opus_list[256] = {NULL};
int opus_list_size[256] = {0};
int total_opus_pkt = 0;
#ifndef DEVICE_IS_RECEIVER
int current_opus_index = 0;
bool start_transmit = false;
#endif
void task_audio(void *)
{
    while (1)
    {
#ifdef DEVICE_IS_RECEIVER
        xSemaphoreTake(audio_start_decode, portMAX_DELAY);
        // int len;
        // uint8_t *data_ptr = audio_encode_packet(&len);
        for (int current_opus_index = 0; current_opus_index < total_opus_pkt; ++current_opus_index)
        {
        }
#else
        if (total_opus_pkt > current_opus_index)
        {
            if (start_transmit)
            {
                encode_packet_opus(0x21, opus_list[current_opus_index], opus_list_size[current_opus_index]);
                channel_sync(3);
                send_packet();
                current_opus_index += 1;
            }
        }
        else if (total_opus_pkt < current_opus_index)
        {
            current_opus_index = 0;
        }
        delay(20);
#endif
    }
}
void resetOpusList()
{
    for (int i = 0; i < 256; ++i)
    {
        if (opus_list[i] != NULL)
        {
            free(opus_list[i]);
            opus_list[i] = NULL;
        }
    }
    total_opus_pkt = 0;
#ifndef DEVICE_IS_RECEIVER
    current_opus_index = 0;
#endif
}
void appendOpusPacket(uint8_t *pkt, size_t size)
{
    if (total_opus_pkt >= 256)
    {
        return;
    }
    opus_list[total_opus_pkt] = (uint8_t *)malloc(size);
    memcpy(opus_list[total_opus_pkt], pkt, size);
    opus_list_size[total_opus_pkt] = size;
    total_opus_pkt++;
}
void playAudioBuffer()
{
    xSemaphoreGive(audio_start_decode);
}
#ifndef DEVICE_IS_RECEIVER
void transmitter_fn_0x01()
{
    uint8_t data;
    Serial.println("直通模式");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2.drawUTF8(10, 18, "功能3/4 直通模式");
    u8g2.sendBuffer();
    while (1)
    {
        if (buttonTX.isPressed())
        {
            beeper.beep(10);
            while ((buttonTX.isPressed()))
            {
                data = 0x55;
                encode_packet(0x01, &data, 1);
                channel_sync(6);
                send_packet();
                delay(2);
                beeper.beep(10);
            }
            data = 0xaa;
            encode_packet(0x01, &data, 1);
            channel_sync(2);
            send_packet();
            beeper.stop();
        }
        if (buttonMod.isPressed())
        {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy14_t_gb2312);
            u8g2.drawUTF8(10, 18, "正在切换功能");
            u8g2.sendBuffer();
            buttonMod.waitRelease();
            break;
        }
        delay(10);
    }
}
void transmitter_fn_0x02()
{
    uint8_t data;
    Serial.println("长短模式");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2.drawUTF8(10, 18, "功能5 长短模式");
    u8g2.sendBuffer();
    channel_sync(10);
    while (1)
    {
        if (buttonTX.isPressed())
        {
            while ((buttonTX.isPressed()))
            {
                beeper.beep(10);
                data = 0x55;
                encode_packet(0x02, &data, 1);
                channel_sync(6);
                send_packet();
                delay(2);
            }
            data = 0xaa;
            encode_packet(0x02, &data, 1);
            channel_sync(2);
            send_packet();
            beeper.stop();
        }
        if (buttonMod.isPressed())
        {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy14_t_gb2312);
            u8g2.drawUTF8(10, 18, "正在切换功能");
            u8g2.sendBuffer();
            buttonMod.waitRelease();
            break;
        }
        delay(10);
    }
}
void morse_tx_string(const char *str)
{
    uint8_t data;
    uint32_t millis_target = 0;
    while (*str)
    {
        char *morse = findMorseReverse(*str);
        if (morse)
        {
            while (*morse)
            {
                if (*morse == '.')
                {
                    millis_target = millis() + LONG_THRESHOLD / 3;
                    while (millis_target - millis() <= LONG_THRESHOLD / 3)
                    {
                        beeper.beep(10);
                        data = 0x55;
                        encode_packet(0x03, &data, 1);
                        channel_sync(6);
                        send_packet();
                        delay(2);
                    }
                    data = 0xaa;
                    encode_packet(0x03, &data, 1);
                    channel_sync(2);
                    send_packet();
                    beeper.stop();
                }
                else if (*morse == '-')
                {
                    millis_target = millis() + LONG_THRESHOLD + 100;
                    while (millis_target - millis() <= LONG_THRESHOLD + 100)
                    {
                        beeper.beep(10);
                        data = 0x55;
                        encode_packet(0x03, &data, 1);
                        channel_sync(6);
                        send_packet();
                        delay(2);
                    }
                    data = 0xaa;
                    encode_packet(0x03, &data, 1);
                    channel_sync(2);
                    send_packet();
                    beeper.stop();
                }
                delay(LONG_THRESHOLD);
                morse++;
            }
            delay(GAP_THRESHOLD + 400);
        }
        ++str;
    }
}
void transmitter_fn_0x03()
{
    uint8_t data;
    Serial.println("字符串模式");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2.drawUTF8(10, 18, "功能6a 字符串模式");
    u8g2.drawUTF8(10, 36, "按下发送");
    u8g2.sendBuffer();
    channel_sync(10);
    while (1)
    {
        if (buttonTX.isPressed())
        {
            morse_tx_string("XS019");
        }
        if (buttonMod.isPressed())
        {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy14_t_gb2312);
            u8g2.drawUTF8(10, 18, "正在切换功能");
            u8g2.sendBuffer();
            buttonMod.waitRelease();
            break;
        }
        delay(10);
    }
}
void transmitter_fn_0x04()
{
    Serial.println("功能6b SOS");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2.drawUTF8(10, 18, "功能6b SOS");
    u8g2.drawUTF8(10, 36, "按下发送");
    u8g2.sendBuffer();
    channel_sync(10);
    while (1)
    {
        if (buttonTX.isPressed())
        {
            morse_tx_string("SOS");
        }
        if (buttonMod.isPressed())
        {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy14_t_gb2312);
            u8g2.drawUTF8(10, 18, "正在切换功能");
            u8g2.sendBuffer();
            buttonMod.waitRelease();
            break;
        }
        delay(10);
    }
}
void transmitter_fn_0x05()
{
    Serial.println("功能7a 当前时间");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2.drawUTF8(10, 18, "功能7a 当前时间");
    u8g2.enableUTF8Print();
    u8g2.setCursor(10, 36);
    u8g2.printf("%d ms", millis());
    u8g2.sendBuffer();
    channel_sync(10);
    while (1)
    {
        if (buttonTX.isPressed())
        {
            uint32_t data = millis();
            encode_packet(0x05, (uint8_t *)&data, 4);
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy14_t_gb2312);
            u8g2.drawUTF8(10, 18, "功能7a 当前时间");
            u8g2.enableUTF8Print();
            u8g2.setCursor(10, 36);
            u8g2.printf("%d ms", data);
            u8g2.sendBuffer();
            channel_sync(2);
            send_packet();
#ifdef TX_DEBUG_MODE
            delay(20);
#endif
        }
        if (buttonMod.isPressed())
        {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy14_t_gb2312);
            u8g2.drawUTF8(10, 18, "正在切换功能");
            u8g2.sendBuffer();
            buttonMod.waitRelease();
            break;
        }
        delay(10);
    }
}
const uint8_t OLEDImageBuffer[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0xE0, 0xC0, 0x00, 0x00, 0xE0,
    0xFE, 0x3E, 0x0C, 0x80, 0xF0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x7E, 0xFF, 0xCF, 0x07, 0x83, 0x63, 0xB0, 0x98, 0x3F,
    0xFF, 0xC6, 0xC7, 0xC7, 0x43, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xE0, 0x00, 0x00, 0x00, 0xFC, 0xF8, 0x00, 0x00, 0x00,
    0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x80, 0x80, 0xC0, 0xC0, 0xFF, 0x67, 0x60, 0x60, 0xE0, 0xF0, 0xF0, 0xF0, 0xF0, 0xE0, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xE0, 0xF0,
    0xE0, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xF0, 0xF8, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x40, 0xC0, 0xC0, 0xC0, 0xC0, 0x4F, 0x6F, 0x67, 0x61, 0x70, 0xFF, 0xFF, 0x7F,
    0x30, 0x39, 0x38, 0x38, 0x18, 0x18, 0x1C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x08, 0x1C, 0x7F, 0xFF, 0xFF, 0xFF, 0xE1, 0x20, 0x00, 0x08, 0xFF, 0xFF, 0xEE, 0xC6, 0xC7,
    0x83, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF,
    0x18, 0x18, 0x18, 0xFF, 0xFF, 0xFE, 0xFE, 0x7E, 0x1F, 0x0F, 0x07, 0x07, 0x03, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0xF6, 0xFF, 0x3F, 0x0F, 0x03, 0xC1,
    0xF1, 0xFF, 0x7F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x18,
    0x18, 0xF8, 0xFC, 0xFF, 0x3F, 0x0F, 0x8C, 0x86, 0x06, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x08, 0x1C, 0x0E, 0x07, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x02, 0x03, 0x03, 0x03, 0x01,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F,
    0x0F, 0x06, 0x07, 0x07, 0x1F, 0x19, 0x18, 0x18, 0x38, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x07, 0x0F, 0x0D, 0x0C, 0x0E, 0x0E, 0x0F, 0x07,
    0x07, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x04,
    0x0E, 0x0F, 0x07, 0x01, 0x00, 0x00, 0x0F, 0x0F, 0x1F, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x8F, 0x8F, 0x84,
    0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x80, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00,
    0x80, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x80, 0x00, 0x80,
    0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00,
    0x80, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x0F, 0x02, 0x0E, 0x00, 0x07, 0x0D, 0x08, 0x08, 0x07, 0x00, 0x07, 0x02, 0x0F,
    0x08, 0x00, 0x07, 0x00, 0x00, 0x0F, 0x02, 0x02, 0x0F, 0x00, 0x00, 0x00, 0x07, 0x08, 0x08, 0x04,
    0x00, 0x0F, 0x02, 0x02, 0x07, 0x00, 0x00, 0x0F, 0x03, 0x06, 0x0F, 0x00, 0x0C, 0x07, 0x07, 0x0C,
    0x00, 0x00, 0x07, 0x0B, 0x0A, 0x0A, 0x00, 0x0F, 0x08, 0x08, 0x00, 0x0F, 0x0A, 0x0A, 0x00, 0x07,
    0x08, 0x08, 0x0C, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x0F, 0x02, 0x06, 0x0D, 0x00, 0x0F, 0x00, 0x0F,
    0x08, 0x08, 0x00, 0x00, 0x00, 0x0F, 0x02, 0x02, 0x03, 0x00, 0x0F, 0x08, 0x08, 0x07, 0x00, 0x07,
    0x0E, 0x01, 0x0E, 0x0F, 0x00, 0x0F, 0x0A, 0x0A, 0x08, 0x00, 0x03, 0x02, 0x0F, 0x00, 0x00, 0x00,
    0x07, 0x08, 0x08, 0x07, 0x00, 0x0F, 0x03, 0x06, 0x0F, 0x00, 0x0F, 0x00, 0x07, 0x0C, 0x03, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void transmitter_fn_0x10()
{
    Serial.println("功能7b 屏幕缓冲区");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2.drawUTF8(10, 18, "功能7b 图像传输");
    u8g2.drawUTF8(10, 36, "按发送键发送");
    u8g2.sendBuffer();
    channel_sync(20);
    while (1)
    {
        if (buttonTX.isPressed())
        {
            u8g2.clearBuffer();
            memcpy(u8g2.getBufferPtr(), OLEDImageBuffer, 1024);
            u8g2.sendBuffer();
            channel_sync(10);
            for (int i = 0; i < 8; ++i)
            {
                encode_packet(0x10 | i, OLEDImageBuffer + (i * 128), 128);
                channel_sync(2);
                send_packet();
            }
            Serial.println("功能7b 屏幕缓冲区");
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy14_t_gb2312);
            u8g2.drawUTF8(10, 18, "功能7b 图像传输");
            u8g2.drawUTF8(10, 36, "按发送键发送");
            u8g2.sendBuffer();
        }
        if (buttonMod.isPressed())
        {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy14_t_gb2312);
            u8g2.drawUTF8(10, 18, "正在切换功能");
            u8g2.sendBuffer();
            buttonMod.waitRelease();
            break;
        }
        delay(10);
    }
}
void transmitter_fn_recorder()
{
    uint8_t data = 0x00;
    Serial.println("功能 音频传输");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2.drawUTF8(10, 18, "功能 音频传输");
    u8g2.drawUTF8(10, 36, "按发送键发送");
    u8g2.sendBuffer();
    channel_sync(20);
    while (1)
    {
        if (buttonTX.isPressed())
        {
            resetOpusList();
            delay(20);
            channel_sync(50);
            encode_packet(0x20, &data, 1);
            send_packet();
            start_transmit = false;
            while (buttonTX.isPressed())
            {
                int len;
                uint8_t *res = audio_encode_packet(&len);
                appendOpusPacket(res, len);
                u8g2.clearBuffer();
                u8g2.setFont(u8g2_font_wqy14_t_gb2312);
                u8g2.drawUTF8(10, 18, "功能 音频传输");
                u8g2.drawUTF8(10, 36, "正在录音并传输");
                u8g2.enableUTF8Print();
                u8g2.setCursor(10, 54);
                u8g2.printf("已录音 %d个", total_opus_pkt);
                u8g2.sendBuffer();
                delay(5);
            }
            start_transmit = true;
            while (total_opus_pkt != current_opus_index)
            {
                u8g2.clearBuffer();
                u8g2.setFont(u8g2_font_wqy14_t_gb2312);
                u8g2.drawUTF8(10, 18, "功能 音频传输");
                u8g2.drawUTF8(10, 36, "等待发送完成");
                u8g2.enableUTF8Print();
                u8g2.setCursor(10, 54);
                u8g2.printf("已发送 %d个", current_opus_index);
                u8g2.sendBuffer();
                delay(5);
            }
            channel_sync(20);
            encode_packet(0x22, &data, 1);
            send_packet();
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy14_t_gb2312);
            u8g2.drawUTF8(10, 18, "功能 音频传输");
            u8g2.drawUTF8(10, 36, "已通知对端解码");
            u8g2.sendBuffer();
        }
        if (buttonMod.isPressed())
        {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_wqy14_t_gb2312);
            u8g2.drawUTF8(10, 18, "正在切换功能");
            u8g2.sendBuffer();
            buttonMod.waitRelease();
            break;
        }
        delay(10);
    }
}
void task_loop(void *)
{
    while (1)
    {

        transmitter_fn_0x01();
        transmitter_fn_0x02();
        transmitter_fn_0x03();
        transmitter_fn_0x04();
        transmitter_fn_0x05();
        transmitter_fn_0x10();
        transmitter_fn_recorder();
        delay(10);
    }
}
#endif
void setup()
{
    esp_task_wdt_init(portMAX_DELAY, false);
    digitalWrite(PIN_LED1, LOW);
    digitalWrite(PIN_LED2, LOW);
    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);
    Serial.begin(115200);
    Serial.println("OK");
    SPI.begin(PIN_OLED_CLK, -1, PIN_OLED_MOSI, -1);
    u8g2.begin();
#ifdef DEVICE_IS_RECEIVER
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2.drawUTF8(0, 18, "电路一次就接队！");
    u8g2.drawUTF8(15, 36, "等待数据中");
    u8g2.sendBuffer();
#endif
    protocol_init();

    aaAudio.begin(1, 1);    // Start AAAudio with only the DAC (ADC,DAC,External ADC)
    aaAudio.autoAdjust = 0; // Disable automatic timer adjustment
    aaAudio.setSampleRate(16000);
    aaAudio.dacBitsPerSample = 8;
    audio_codec_init();
    buttonMod.init(PIN_KEY1);
    buttonTX.init(PIN_KEY2);
    xTaskCreatePinnedToCore(task_button, "task_button", 2048, NULL, 1, NULL, 1);
    beeper.init();
    audio_start_decode = xSemaphoreCreateBinary();
    delay(100);
    xTaskCreatePinnedToCore(task_audio, "task_audio", 36000, NULL, 10, NULL, 1);
#ifndef DEVICE_IS_RECEIVER
    xTaskCreatePinnedToCore(task_loop, "task_loop", 36000, NULL, 4, NULL, 1);
#endif
}
void loop()
{
    vTaskDelete(NULL);
}