#include "RFID/RFIDManager.h"
#include <string.h>

static const uint8_t ReadMultiCmd[10] = {
    0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0xFF, 0xFF, 0x4A, 0x7E};
static unsigned long lastCommandTime = 0;
static constexpr unsigned long READ_CMD_PERIOD_MS = 1000UL;

RFIDManager::RFIDManager(SystemCoordinator &coord)
    : coordinator(coord)
{
}

void RFIDManager::begin()
{
#if DEBUG_MODE
    DBG_SL("[RFID] begin");
#endif
    resetStates();
    lastCommandTime = 0;
}

void RFIDManager::resetStates()
{
    state = 0;
    codeState = 0;
    dataAdd = 0;
    epcIndex = 0;
    memset(epcBuf, 0, sizeof(epcBuf));
}

static void SendReadCommand()
{
    Serial.write(ReadMultiCmd, sizeof(ReadMultiCmd));
#if DEBUG_MODE
    DBG_SL("[RFID] sent cmd");
#endif
}

void RFIDManager::handleEpcComplete()
{
#if DEBUG_MODE
    DBG_S("[RFID] EPC:");
    for (uint8_t i = 0; i < 12; ++i)
    {
        DBG_HEX(epcBuf[i]);
        DBG_S(" ");
    }
    DBG_S("\n");
#endif
    if (coordinator.isValidTag(epcBuf, 12))
    {
#if DEBUG_MODE
        DBG_SL("[RFID] known -> notify");
#endif
        coordinator.onTagDetected(epcBuf, 12);
    }
    else
    {
#if DEBUG_MODE
        DBG_SL("[RFID] unknown");
#endif
    }
}

void RFIDManager::loop()
{
    if (!coordinator.isRfidEnabled())
    {
        resetStates();
        return;
    }

    unsigned long now = millis();
    if ((now - lastCommandTime) >= READ_CMD_PERIOD_MS)
    {
        lastCommandTime = now;
        SendReadCommand();
    }

    while (Serial.available() > 0)
    {
        int in = Serial.read();
        if (in < 0)
            break;
        uint8_t b = static_cast<uint8_t>(in);

        if ((b == 0x02) && (state == 0))
        {
            state = 1;
            continue;
        }
        else if ((state == 1) && (b == 0x22) && (codeState == 0))
        {
            codeState = 1;
            dataAdd = 3;
            epcIndex = 0;
            continue;
        }
        else if (codeState == 1)
        {
            dataAdd++;

            if ((dataAdd >= 9) && (dataAdd <= 20))
            {
                if (epcIndex < sizeof(epcBuf))
                {
                    epcBuf[epcIndex++] = b;
                }
            }
            else if (dataAdd >= 21)
            {
                if (epcIndex == (int)sizeof(epcBuf))
                {
                    handleEpcComplete();
                }
                resetStates();
            }
        }
        else
        {
            resetStates();
        }
    }
}