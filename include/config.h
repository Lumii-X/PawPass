#ifndef PAWPASS_CONFIG_H
#define PAWPASS_CONFIG_H

#include <Arduino.h>

constexpr uint8_t ENABLE_DEBUG = 1;

#if ENABLE_DEBUG
#pragma message("Debug mode enabled for RFIDManager")
#define LOG(...) Serial.print(__VA_ARGS__)
#define LOGLN(...) Serial.println(__VA_ARGS__)
#define LOGF(x) Serial.print(F(x))
#define LOGLNF(x) Serial.println(F(x))
#else
#define LOG(...) ((void)0)
#define LOGLN(...) ((void)0)
#define LOGF(x) ((void)0)
#define LOGLNF(x) ((void)0)
#endif

constexpr uint8_t PIN_SERVO = 9;
constexpr unsigned long READ_INTERVAL_MS = 1000UL;
constexpr unsigned long TAG_TIMEOUT_MS = 3000UL;
constexpr unsigned long PARSER_TIMEOUT_MS = 200UL;
constexpr int SERVO_OPEN_US = 2500;
constexpr int SERVO_CLOSE_US = 500;
constexpr uint8_t EPC_LENGTH = 12;
constexpr uint8_t READ_CMD_LEN = 10;
constexpr uint8_t MAX_PACKET_LEN = 64;

#endif // PAWPASS_CONFIG_H