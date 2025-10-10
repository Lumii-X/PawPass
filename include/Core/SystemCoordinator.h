#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "../Core/Config.h"

struct IntervalRecord
{
    char id[12];
    uint16_t startMin;
    uint16_t endMin;
    uint8_t daysMask;
    bool enabled;
};

class SystemCoordinator
{
public:
    void begin();
    bool addInterval(const char *id, uint16_t startMin, uint16_t endMin, uint8_t daysMask);
    bool updateIntervalTime(const char *id, uint16_t startMin, uint16_t endMin, uint8_t daysMask);
    bool deleteInterval(const char *id);
    bool setIntervalStatus(const char *id, bool enabled);
    bool setIntervalEnabled(const char *id, bool enabled);
    bool setIntervalStart(const char *id, uint16_t startMin);
    bool setIntervalEnd(const char *id, uint16_t endMin);
    bool setIntervalDays(const char *id, uint8_t daysMask);
    void onTagDetected(const uint8_t *epc, uint8_t len);
    bool addTag(const uint8_t *epc, uint8_t len = 12);
    uint8_t getNumTags() const { return numTags; }
    bool isValidTag(const uint8_t *epc, uint8_t len) const;
    void loop();
    bool isRfidEnabled() const { return rfidEnabled; }

private:
    IntervalRecord intervals[MAX_INTERVALS];
    uint8_t intervalCount = 0;
    uint8_t tags[MAX_TAGS][12];
    uint8_t numTags = 0;
    bool tagPresent[MAX_TAGS] = {0};
    unsigned long lastSeen[MAX_TAGS] = {0};
    bool rfidEnabled = false;
    unsigned long lastCheckMs = 0;
    bool doorOpen = false;
    int8_t findIndexById(const char *id) const;
    int8_t findTagIndex(const uint8_t *epc, uint8_t len) const;
    void evaluateNow(bool forced = false);
    void checkTagTimeouts();
    void openDoor();
    void closeDoor();
    uint16_t getCurrentMinutes() const;
    uint8_t getTodayMaskBit() const;
};