#pragma once
#include <Arduino.h>
#include "../Core/SystemCoordinator.h"

class RFIDManager
{
public:
    RFIDManager(SystemCoordinator &coord);
    void begin();
    void loop();

private:
    SystemCoordinator &coordinator;
    uint8_t state = 0;
    uint8_t codeState = 0;
    uint8_t dataAdd = 0;
    uint8_t epcBuf[12];
    uint8_t epcIndex = 0;
    void resetStates();
    void handleEpcComplete();
};