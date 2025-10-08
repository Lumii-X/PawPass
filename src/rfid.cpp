// ...existing code...
#include "rfid.h"
#include "config.h"
#include <string.h>

#if defined(__AVR__)
#include <avr/pgmspace.h>
#endif

static constexpr uint8_t READ_MULTI[READ_CMD_LEN] = {
    0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0xFF, 0xFF, 0x4A, 0x7E};

#if defined(__AVR__)
static const uint8_t AUTHORIZED_TAGS[][EPC_LENGTH] PROGMEM = {
    {0xE2, 0x00, 0x47, 0x09, 0x3E, 0xB0, 0x64, 0x26, 0xB8, 0x4A, 0x01, 0x13}};
#else
static constexpr uint8_t AUTHORIZED_TAGS[][EPC_LENGTH] = {
    {0xE2, 0x00, 0x47, 0x09, 0x3E, 0xB0, 0x64, 0x26, 0xB8, 0x4A, 0x01, 0x13}};
#endif

static constexpr size_t AUTHORIZED_TAG_COUNT = sizeof(AUTHORIZED_TAGS) / sizeof(AUTHORIZED_TAGS[0]);

void RFIDManager::begin(unsigned long baud)
{
    doorServo.attach(PIN_SERVO);
    doorServo.writeMicroseconds(SERVO_CLOSE_US);
    gateState = GateState::CLOSED;
    systemState = SystemState::READING;

    Serial.begin(baud);
    LOGLNF("Sistema RFID iniciado.");

    sendReadCommand();
    lastSendMs = millis();
    lastByteMs = millis();
}

void RFIDManager::update()
{
    const unsigned long now = millis();

#ifdef RFID_SIMULATION
    if (Serial.available())
    {
        String s = Serial.readStringUntil('\n');
        if (s == "TAG_TEST")
        {
            simulateTag(nullptr);
        }
    }
#endif

    switch (systemState)
    {
    case SystemState::IDLE:
        if ((now - lastSendMs) >= READ_INTERVAL_MS)
        {
            lastSendMs = now;
            sendReadCommand();
            systemState = SystemState::READING;
        }
        break;

    case SystemState::READING:
        if ((now - lastSendMs) >= READ_INTERVAL_MS)
        {
            lastSendMs = now;
            sendReadCommand();
        }
        while (Serial.available())
        {
            uint8_t b = static_cast<uint8_t>(Serial.read());
            lastByteMs = now;
            processByte(b);
        }
        if ((state != ParseState::WAIT_START) && ((now - lastByteMs) > PARSER_TIMEOUT_MS))
        {
            LOGF("Parser timeout, resetting parser\n");
            resetParser();
        }
        break;

    case SystemState::DOOR_OPEN:
        if ((now - lastSendMs) >= READ_INTERVAL_MS)
        {
            lastSendMs = now;
            sendReadCommand();
        }
        while (Serial.available())
        {
            uint8_t b = static_cast<uint8_t>(Serial.read());
            lastByteMs = now;
            processByte(b);
        }
        break;

    case SystemState::DOOR_CLOSING:
        closeDoor();
        systemState = SystemState::READING;
        break;
    }

    checkTagTimeout();
}

inline void RFIDManager::sendReadCommand()
{
    Serial.write(READ_MULTI, READ_CMD_LEN);
}

void RFIDManager::processByte(uint8_t b)
{
    if (dataAdd >= MAX_PACKET_LEN)
    {
        resetParser();
        return;
    }

    switch (state)
    {
    case ParseState::WAIT_START:
        if (b == 0x02)
        {
            state = ParseState::GOT_START;
            dataAdd = 0;
            epcIndex = 0;
        }
        break;

    case ParseState::GOT_START:
        if (b == 0x22)
        {
            state = ParseState::GOT_CODE;
            dataAdd = 3;
            epcIndex = 0;
        }
        else
        {
            resetParser();
        }
        break;

    case ParseState::GOT_CODE:
        dataAdd++;
        if ((dataAdd >= 9) && (dataAdd <= 20) && (epcIndex < EPC_LENGTH))
            epcBuf[epcIndex++] = b;
        if (dataAdd >= 21)
        {
            handleTagDetection();
            resetParser();
        }
        break;
    }
}

void RFIDManager::handleTagDetection()
{
    if (epcIndex != EPC_LENGTH)
        return;

    if (isAuthorizedTag())
    {
        if (!tagDetected)
        {
            LOGLNF("Tag detectado. Abriendo puerta.");
            openDoor();
            systemState = SystemState::DOOR_OPEN;
        }
        tagDetected = true;
        lastTagTime = millis();
    }
    else
    {
        LOGF("Tag no autorizado: ");
        for (uint8_t i = 0; i < EPC_LENGTH; ++i)
        {
            LOG(epcBuf[i], HEX);
            LOG(' ');
        }
        LOGLN("");
    }
}

bool RFIDManager::isAuthorizedTag() const
{
#if defined(__AVR__)
    for (size_t i = 0; i < AUTHORIZED_TAG_COUNT; ++i)
    {
        bool ok = true;
        for (uint8_t j = 0; j < EPC_LENGTH; ++j)
        {
            uint8_t v = pgm_read_byte(&AUTHORIZED_TAGS[i][j]);
            if (v != epcBuf[j])
            {
                ok = false;
                break;
            }
        }
        if (ok)
            return true;
    }
    return false;
#else
    for (size_t i = 0; i < AUTHORIZED_TAG_COUNT; ++i)
    {
        bool ok = true;
        for (uint8_t j = 0; j < EPC_LENGTH; ++j)
        {
            if (AUTHORIZED_TAGS[i][j] != epcBuf[j])
            {
                ok = false;
                break;
            }
        }
        if (ok)
            return true;
    }
    return false;
#endif
}

inline void RFIDManager::openDoor()
{
    if (gateState != GateState::OPEN)
    {
        doorServo.writeMicroseconds(SERVO_OPEN_US);
        gateState = GateState::OPEN;
    }
}

inline void RFIDManager::closeDoor()
{
    if (gateState != GateState::CLOSED)
    {
        doorServo.writeMicroseconds(SERVO_CLOSE_US);
        gateState = GateState::CLOSED;
    }
}

void RFIDManager::checkTagTimeout()
{
    if (tagDetected && ((millis() - lastTagTime) > TAG_TIMEOUT_MS))
    {
        LOGLNF("Tag no detectado. Cerrando puerta.");
        tagDetected = false;
        systemState = SystemState::DOOR_CLOSING;
    }
}

inline void RFIDManager::resetParser()
{
    state = ParseState::WAIT_START;
    dataAdd = 0;
    epcIndex = 0;
}

#ifdef RFID_SIMULATION
void RFIDManager::simulateTag(const uint8_t *tag)
{
    if (!tag)
    {
#if defined(__AVR__)
        uint8_t tmp[EPC_LENGTH];
        for (uint8_t i = 0; i < EPC_LENGTH; ++i)
            tmp[i] = pgm_read_byte(&AUTHORIZED_TAGS[0][i]);
        memcpy(epcBuf, tmp, EPC_LENGTH);
#else
        memcpy(epcBuf, AUTHORIZED_TAGS[0], EPC_LENGTH);
#endif
    }
    else
    {
        memcpy(epcBuf, tag, EPC_LENGTH);
    }
    epcIndex = EPC_LENGTH;
    handleTagDetection();
}
#endif