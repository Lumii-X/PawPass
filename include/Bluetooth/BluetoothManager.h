#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include "Core/SystemCoordinator.h"

class BluetoothManager
{
public:
    BluetoothManager(SystemCoordinator &coord);
    void begin();
    void loop();

private:
    SystemCoordinator &coordinator;
    SoftwareSerial btSerial;
    static constexpr size_t LINE_BUF = 64;
    bool readLine(char *outBuf, size_t maxLen, unsigned long timeoutMs = 800);
    uint16_t parse12hToMinutes(const char *token) const;
    uint8_t parseDaysListToMask(const char *token) const;
    void handleCreateTokens(char *tokens[], int count);
    void handleUpdateTokens(char *tokens[], int count);
    void handleDeleteTokens(char *tokens[], int count);
    static void trimInPlace(char *s);
};