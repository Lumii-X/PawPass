#pragma once
#include <stdint.h>

constexpr uint8_t MAX_INTERVALS = 10;
constexpr uint8_t MAX_TAGS = 8;
constexpr unsigned long COORDINATOR_CHECK_INTERVAL_MS = 60000UL;
constexpr uint8_t BT_RX_PIN = 2;
constexpr uint8_t BT_TX_PIN = 3;
constexpr unsigned long BT_BAUD = 9600UL;
constexpr uint8_t SERVO_PIN = 9;
constexpr unsigned long DOOR_DELAY_MS = 3000UL;

// Debug mode: 0 = disabled, 1 = enabled
#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

#if DEBUG_MODE
#define DBG_S(x) Serial.print(F(x))
#define DBG_SL(x) Serial.println(F(x))
#define DBG_V(x) Serial.print(x)
#define DBG_VL(x) Serial.println(x)
#define DBG_HEX(x) Serial.print(x, HEX)
#define DBG_HEXL(x) Serial.println(x, HEX)
#else
#define DBG_S(x) (void)0
#define DBG_SL(x) (void)0
#define DBG_V(x) (void)0
#define DBG_VL(x) (void)0
#define DBG_HEX(x) (void)0
#define DBG_HEXL(x) (void)0
#endif

#include <avr/pgmspace.h>
static const uint8_t DEFAULT_TAGS[][12] PROGMEM = {
    {0xE2, 0x00, 0x47, 0x09, 0x3E, 0xB0, 0x64, 0x26, 0xB8, 0x4A, 0x01, 0x13},
    {0xE2, 0x00, 0x47, 0x10, 0x80, 0x30, 0x60, 0x26, 0x99, 0x06, 0x01, 0x0B}};
static constexpr uint8_t DEFAULT_TAGS_COUNT = sizeof(DEFAULT_TAGS) / sizeof(DEFAULT_TAGS[0]);