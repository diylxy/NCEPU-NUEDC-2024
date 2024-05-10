#pragma once
#define PKT_TYPE_KEY_STATUS 0x01
#define PKT_TYPE_CURRENT_MODE 0x02
#define PKT_TYPE_CURRENT_TIME 0x03
#define PKT_TYPE_OLED_DISPLAY_START 0x10
void protocol_init();

void encode_packet(uint8_t type, const uint8_t *dat, uint16_t size);
void encode_packet_opus(uint8_t type, const uint8_t *dat, uint16_t size);
void channel_sync(uint8_t bytes);
void send_packet();
