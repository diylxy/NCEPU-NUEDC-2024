#include "main.h"
#include "protocol.h"
// 高电平为常态（OUT_LEVEL_1），低电平为有数据（类似异步串口）
#ifndef DEVICE_IS_RECEIVER
inline void sync() // 锁相
{
    digitalWrite(OUTPUT_PIN, OUT_LEVEL_1);
    while (digitalRead(PLL_PIN) == 1)
        ;
    while (digitalRead(PLL_PIN) == 0)
        ; // 等待上升沿
}
inline void tx_1()
{
    ledGreen_off();
    digitalWrite(OUTPUT_PIN, OUT_LEVEL_1);
    while (digitalRead(PLL_PIN) == 1)
        ; // 1个波长
    while (digitalRead(PLL_PIN) == 0)
        ;
    while (digitalRead(PLL_PIN) == 1)
        ; // 2个波长
    while (digitalRead(PLL_PIN) == 0)
        ;
}
inline void tx_0()
{
    ledGreen_on();
    digitalWrite(OUTPUT_PIN, OUT_LEVEL_0);
    while (digitalRead(PLL_PIN) == 1)
        ; // 1个波长
    while (digitalRead(PLL_PIN) == 0)
        ;
    while (digitalRead(PLL_PIN) == 1)
        ; // 2个波长
    while (digitalRead(PLL_PIN) == 0)
        ;
}
static void sendData(uint8_t dat)
{
#ifdef TX_DEBUG_MODE
    Serial2.write(&dat, 1);
    return;
#else
    sync();
    tx_0();
    for (int i = 0; i < 8; ++i)
    {
        if (dat & 0x01)
            tx_1();
        else
            tx_0();
        dat >>= 1;
    }
    tx_1();
#ifdef STOP_BIT_LEN_2
    while (digitalRead(PLL_PIN) == 1)
        ; // 1个波长
    while (digitalRead(PLL_PIN) == 0)
        ;
    while (digitalRead(PLL_PIN) == 1)
        ; // 1个波长
    while (digitalRead(PLL_PIN) == 0)
        ;
    while (digitalRead(PLL_PIN) == 1)
        ; // 1个波长
    while (digitalRead(PLL_PIN) == 0)
        ;
#endif
#endif
}
#else
static void sendData(uint8_t dat)
{
}
#endif

uint8_t packet_buffer[1050];
uint16_t packet_size = 0;

static uint8_t calculate_checksum8(uint8_t *data, uint16_t size)
{
    uint8_t checksum = 0;
    for (int i = 0; i < size; ++i)
    {
        checksum += data[i];
    }
    return checksum;
}

void encode_packet(uint8_t type, const uint8_t *dat, uint16_t size)
{
    packet_buffer[0] = 0x55;
    packet_buffer[1] = type;
    memcpy(packet_buffer + 2, dat, size);
    packet_buffer[size + 2] = calculate_checksum8(packet_buffer + 2, size);
    packet_buffer[size + 3] = 0xAA;
    packet_size = size + 4;
}
void encode_packet_opus(uint8_t type, const uint8_t *dat, uint16_t size)
{
    if (size > 1020)
    {
        Serial.println("opus packet size too large");
        return;
    }
    packet_buffer[0] = 0x55;
    packet_buffer[1] = type;
    packet_buffer[2] = size >> 8;
    packet_buffer[3] = size & 0xff;
    memcpy(packet_buffer + 4, dat, size);
    packet_buffer[size + 4] = calculate_checksum8(packet_buffer + 2, size + 2);
    packet_buffer[size + 5] = 0xAA;
    packet_size = size + 6;
}
void channel_sync(uint8_t bytes)
{
    ledRed_on();
    for (int i = 0; i < bytes; ++i)
    {
        sendData(0x80);
    }
    ledRed_off();
}
void send_packet()
{
    for (int i = 0; i < packet_size; ++i)
    {
        sendData(packet_buffer[i]);
    }
    ledGreen_off();
}

uint8_t last_pkt_type = 0xff;
int last_pkt_millis = 0;
#define OLED_PACKET_SIZE 128

static char morse_buffer[10];
static int morse_buffer_pointer = 0;
static char morse_input[100] = "";
static int morse_input_pointer = 0;
bool key_isPressing = false;
bool key_isLongPressing = false;
void check_0x02_timeout()
{
    if (millis() - last_pkt_millis > 90)
    {
        ledRed_off();
        ledGreen_off();
        key_isPressing = false;
    }
    else if (last_pkt_type > 0x04)
    {
        ledRed_on();
        ledGreen_off();
    }
}
void pkt_processor_task(void *)
{
    static bool key_isPressing_previous = false;
    static uint32_t key_press_millis = -1;
    static uint32_t key_release_millis = -1;
    while (1)
    {
        check_0x02_timeout();
        if (last_pkt_type != 0x02 && last_pkt_type != 0x03)
        {
            key_isPressing = false;
            key_isLongPressing = false;
            key_isPressing_previous = false;
            key_press_millis = -1;
            key_release_millis = -1;
        }
        else
        {
            if (key_isPressing)
            {
                // process morse code
                if (key_isPressing_previous == false)
                {
                    key_isPressing_previous = true;
                    morse_buffer[morse_buffer_pointer] = '.';
                    key_press_millis = millis();
                    key_release_millis = -1;
                }
                if (millis() - key_press_millis >= LONG_THRESHOLD)
                {
                    ledGreen_off();
                    ledRed_on();
                    if (key_isLongPressing == false)
                    {
                        key_isLongPressing = true;
                        morse_buffer[morse_buffer_pointer] = '-';
                    }
                }
                else
                {
                    ledRed_off();
                    ledGreen_on();
                    key_isLongPressing = false;
                }
            }
            else
            {
                ledRed_off();
                ledGreen_off();
                beeper.stop();
                if (key_isPressing_previous)
                {
                    key_isPressing_previous = false;
                    key_release_millis = millis();
                    key_press_millis = -1;
                    if (morse_buffer_pointer < sizeof(morse_buffer))
                        ++morse_buffer_pointer;
                    morse_buffer[morse_buffer_pointer] = '\0';
                }
                if (millis() - key_release_millis >= GAP_THRESHOLD)
                {
                    if (morse_buffer_pointer > 0)
                    {
                        char c = findMorse(morse_buffer);
                        if (c != '\0')
                        {
                            morse_input[morse_input_pointer] = c;
                            ++morse_input_pointer;
                            morse_input[morse_input_pointer] = '\0';
                            Serial.println(morse_input);
                            if (last_pkt_type == 0x03)
                            {
                                u8g2.clearBuffer();
                                u8g2.setFont(u8g2_font_wqy14_t_gb2312);
                                u8g2.drawUTF8(10, 18, "功能6 自动模式");
                                u8g2.drawUTF8(10, 36, morse_input);
                                u8g2.sendBuffer();
                            }
                            else if (last_pkt_type == 0x02)
                            {
                                u8g2.clearBuffer();
                                u8g2.setFont(u8g2_font_wqy14_t_gb2312);
                                u8g2.drawUTF8(10, 18, "功能5 长短模式");
                                u8g2.drawUTF8(10, 36, morse_input);
                                u8g2.sendBuffer();
                            }
                        }
                        morse_buffer[0] = '\0';
                        morse_buffer_pointer = 0;
                    }
                }
            }
        }
        delay(10);
    }
}
void pkt_processor_0x01()
{
    last_pkt_millis = millis();
    if (last_pkt_type != 0x01)
    {
        last_pkt_type = 0x01;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_wqy14_t_gb2312);
        u8g2.drawUTF8(10, 18, "功能3/4 直通模式");
        u8g2.sendBuffer();
    }
    beeper.beep(15);
    last_pkt_millis = millis();
    ledRed_on();
}
void pkt_processor_0x02()
{
    static int last_input_buffer_pointer = -1;
    last_pkt_millis = millis();
    if (last_pkt_type != 0x02)
    {
        last_pkt_type = 0x02;
        morse_input[0] = 0;
        morse_input_pointer = 0;
        morse_buffer[0] = 0;
        morse_buffer_pointer = 0;
        last_input_buffer_pointer = -1;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_wqy14_t_gb2312);
        u8g2.drawUTF8(10, 18, "功能5 长短模式");
        u8g2.sendBuffer();
    }
    if (last_input_buffer_pointer != morse_input_pointer)
    {
        last_input_buffer_pointer = morse_input_pointer;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_wqy14_t_gb2312);
        u8g2.drawUTF8(10, 18, "功能5 长短模式");
        u8g2.drawUTF8(10, 36, morse_input);
        u8g2.sendBuffer();
    }
    if (packet_buffer[2] == 0xaa) // 按键松开指示
    {
        key_isPressing = false;
        beeper.stop();
        return;
    }
    else if (packet_buffer[2] == 0x55)
        key_isPressing = true;
    beeper.beep(15);
}
void pkt_processor_0x03()
{
    static int last_input_buffer_pointer = -1;
    last_pkt_millis = millis();
    if (last_pkt_type != 0x03)
    {
        last_pkt_type = 0x03;
        morse_input[0] = 0;
        morse_input_pointer = 0;
        morse_buffer[0] = 0;
        morse_buffer_pointer = 0;
        last_input_buffer_pointer = -1;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_wqy14_t_gb2312);
        u8g2.drawUTF8(10, 18, "功能6 自动模式");
        u8g2.sendBuffer();
    }
    if (last_input_buffer_pointer != morse_input_pointer)
    {
        last_input_buffer_pointer = morse_input_pointer;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_wqy14_t_gb2312);
        u8g2.drawUTF8(10, 18, "功能6 自动模式");
        u8g2.drawUTF8(10, 36, morse_input);
        u8g2.sendBuffer();
    }
    if (packet_buffer[2] == 0xaa) // 按键松开指示
    {
        key_isPressing = false;
        beeper.stop();
        return;
    }
    else if (packet_buffer[2] == 0x55)
        key_isPressing = true;
    beeper.beep(15);
}
void pkt_processor_0x04()
{
    pkt_processor_0x03();
}
void pkt_processor_0x05()
{
    uint32_t remote_time = 0;
    memcpy(&remote_time, packet_buffer + 2, 4);
    if (last_pkt_type != 0x05)
    {
        last_pkt_type = 0x05;
        beeper.beep(15);
    }
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2.drawUTF8(10, 18, "功能7a 当前时间");
    u8g2.enableUTF8Print();
    u8g2.setCursor(10, 36);
    u8g2.printf("%d ms", remote_time);
    u8g2.setCursor(10, 54);
    remote_time /= 1000;
    u8g2.printf("比赛用时：%02d:%02d", remote_time / 60, remote_time % 60);
    u8g2.sendBuffer();
}
void pkt_processor_0x10(uint16_t line)
{
    if (last_pkt_type != 0x10)
    {
        last_pkt_type = 0x10;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_wqy14_t_gb2312);
        u8g2.drawUTF8(10, 36, "功能7b 图像传输");
        u8g2.drawUTF8(10, 54, "正在逐页接收数据");
    }
    memcpy(u8g2.getBufferPtr() + (line * 128), packet_buffer + 2, OLED_PACKET_SIZE);
    u8g2.sendBuffer();
}
void pkt_processor_0x20()
{
    if (last_pkt_type != 0x20)
    {
        last_pkt_type = 0x20;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_wqy14_t_gb2312);
        u8g2.drawUTF8(10, 18, "功能 音频传输");
        u8g2.drawUTF8(10, 36, "准备接收数据包");
        u8g2.sendBuffer();
    }
    resetOpusList();
    beeper.beep(10);
}
void pkt_processor_0x21()
{
    uint16_t opus_size = (packet_buffer[2] << 8) | packet_buffer[3];
    if (last_pkt_type != 0x21)
    {
        last_pkt_type = 0x21;
    }
    appendOpusPacket(packet_buffer + 4, opus_size);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy14_t_gb2312);
    u8g2.drawUTF8(10, 18, "功能 音频传输");
    u8g2.drawUTF8(10, 36, "正在接收数据包");
    u8g2.enableUTF8Print();
    u8g2.setCursor(10, 54);
    u8g2.printf("已接收：%d个", total_opus_pkt);
    u8g2.sendBuffer();
}
void pkt_processor_0x22()
{
    if (last_pkt_type != 0x22)
    {
        last_pkt_type = 0x22;
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_wqy14_t_gb2312);
        u8g2.drawUTF8(10, 18, "功能 音频传输");
        u8g2.drawUTF8(10, 36, "解码已开始");
        u8g2.enableUTF8Print();
        u8g2.sendBuffer();
        playAudioBuffer();
    }
}
static void process_packet()
{
    switch (packet_buffer[1])
    {
    case 0x01:
        pkt_processor_0x01();
        break;
    case 0x02:
        pkt_processor_0x02();
        break;
    case 0x03:
        pkt_processor_0x03();
        break;
    case 0x04:
        pkt_processor_0x04();
        break;
    case 0x05:
        pkt_processor_0x05();
        break;
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17:
        pkt_processor_0x10(packet_buffer[1] & 0x0f);
        break;
    case 0x20:
        pkt_processor_0x20();
        break;
    case 0x21:
        pkt_processor_0x21();
        break;
    case 0x22:
        pkt_processor_0x22();
        break;
    default:
        break;
    }
}
static uint8_t serial_read()
{
    uint8_t dat = 0x00;
    while (Serial2.readBytes(&dat, 1) == 0)
        ;
    return dat;
}

static void task_serial2(void *)
{
    static uint16_t payload_len;
    char c;
    while (1)
    {
        Serial.println("开始");
        if (serial_read() != 0x80)
            continue;
#ifdef RX_DEBUG_MODE
        Serial.println("收到同步字节");
#endif
        while ((c = serial_read()) == 0x80)
            ;
        if (c != 0x55)
            continue;
#ifdef RX_DEBUG_MODE
        Serial.println("收到起始字节");
#endif
        packet_buffer[0] = 0x55;
        packet_buffer[1] = serial_read();
        switch (packet_buffer[1])
        {
        case 0x01:
            payload_len = 1;
            break;
        case 0x02:
            payload_len = 1;
            break;
        case 0x03:
            payload_len = 1;
            break;
        case 0x04:
            payload_len = 1;
            break;
        case 0x05:
            payload_len = 4;
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
        case 0x15:
        case 0x16:
        case 0x17:
            payload_len = OLED_PACKET_SIZE;
            break;
        case 0x20:
            payload_len = 1;
            break;
        case 0x21:
            payload_len = 0xffff;
            break;
        case 0x22:
            payload_len = 1;
            break;
        default:
            continue;
        }
        if (payload_len != 0xffff)
        {
            for (int i = 0; i < payload_len; ++i)
            {
                packet_buffer[i + 2] = serial_read();
            }
        }
        else
        {
            packet_buffer[2] = serial_read();
            packet_buffer[3] = serial_read();
            payload_len = (packet_buffer[2] << 8) | packet_buffer[3];
            if(payload_len > 1020)
                continue;
            for (int i = 0; i < payload_len; ++i)
            {
                packet_buffer[i + 4] = serial_read();
            }
            payload_len += 2;
        }
        packet_buffer[payload_len + 2] = serial_read();
#ifdef RX_DEBUG_MODE
        Serial.println("checksum");
#endif
        if (calculate_checksum8(packet_buffer + 2, payload_len) != packet_buffer[payload_len + 2])
            continue;
#ifdef RX_DEBUG_MODE
        Serial.println("pass");
#endif
        packet_buffer[payload_len + 3] = serial_read();
        if (packet_buffer[payload_len + 3] != 0xAA)
            continue;
#ifdef RX_DEBUG_MODE
        Serial.println("finish");
#endif
        process_packet();
    }
}

void protocol_init()
{
#ifdef DEVICE_IS_RECEIVER
    Serial2.begin(5000, SERIAL_8N1, PIN_RX2, PIN_DUMMY);
    Serial2.setTimeout(portMAX_DELAY);
    xTaskCreatePinnedToCore(task_serial2, "task_serial2", 8192, NULL, 30, NULL, 1);
    xTaskCreatePinnedToCore(pkt_processor_task, "pkt_processor_task", 4096, NULL, 6, NULL, 1);
#else
#ifdef TX_DEBUG_MODE
    Serial2.begin(5000, SERIAL_8N1, PIN_DUMMY, PIN_RX2);
#else
    digitalWrite(OUTPUT_PIN, OUT_LEVEL_1);
    pinMode(OUTPUT_PIN, OUTPUT);
    pinMode(PLL_PIN, INPUT_PULLUP);
#endif
#endif
}