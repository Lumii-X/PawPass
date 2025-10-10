#include "Core/SystemCoordinator.h"
#include <string.h>
#include <avr/pgmspace.h>
#include <Servo.h>

static Servo doorServo;

void SystemCoordinator::begin()
{
    intervalCount = 0;
    numTags = 0;
    memset(tagPresent, 0, sizeof(tagPresent));
    for (uint8_t i = 0; i < MAX_TAGS; ++i)
        lastSeen[i] = 0;
    rfidEnabled = false;
    doorOpen = false;
    lastCheckMs = millis();
    doorServo.attach(SERVO_PIN);
    doorServo.writeMicroseconds(500);
    doorOpen = false;

#if DEBUG_MODE
    DBG_SL("[SYS] init");
#endif
    uint8_t toLoad = DEFAULT_TAGS_COUNT;
    if (toLoad > MAX_TAGS)
        toLoad = MAX_TAGS;
    for (uint8_t i = 0; i < toLoad; ++i)
    {
        uint8_t buf[12];
        memcpy_P(buf, &DEFAULT_TAGS[i], 12);
        addTag(buf, 12);
    }

    evaluateNow(true);
}

int8_t SystemCoordinator::findTagIndex(const uint8_t *epc, uint8_t len) const
{
    if (!epc || len != 12)
        return -1;
    for (uint8_t i = 0; i < numTags; ++i)
    {
        if (memcmp(tags[i], epc, 12) == 0)
            return i;
    }
    return -1;
}

bool SystemCoordinator::isValidTag(const uint8_t *epc, uint8_t len) const
{
    return findTagIndex(epc, len) >= 0;
}

bool SystemCoordinator::addTag(const uint8_t *epc, uint8_t len)
{
    if (!epc || len != 12)
        return false;
    if (numTags >= MAX_TAGS)
        return false;
    memcpy(tags[numTags], epc, 12);
    tagPresent[numTags] = false;
    lastSeen[numTags] = 0;
    ++numTags;
#if DEBUG_MODE
    DBG_S("[SYS] tag added idx=");
    DBG_VL(numTags - 1);
#endif
    return true;
}

void SystemCoordinator::onTagDetected(const uint8_t *epc, uint8_t len)
{
    if (!epc || len != 12)
        return;
    int8_t idx = findTagIndex(epc, len);
    if (idx < 0)
        return;
    tagPresent[idx] = true;
    lastSeen[idx] = millis();
#if DEBUG_MODE
    DBG_S("[SYS] seen idx=");
    DBG_VL(idx);
#endif
    if (!doorOpen)
        openDoor();
}

void SystemCoordinator::openDoor()
{
    if (doorOpen)
        return;
    doorServo.writeMicroseconds(2500);
    doorOpen = true;
#if DEBUG_MODE
    DBG_SL("[SYS] open");
#endif
}

void SystemCoordinator::closeDoor()
{
    if (!doorOpen)
        return;
    doorServo.writeMicroseconds(500);
    doorOpen = false;
#if DEBUG_MODE
    DBG_SL("[SYS] close");
#endif
}

void SystemCoordinator::checkTagTimeouts()
{
    bool anyPresent = false;
    unsigned long now = millis();
    for (uint8_t i = 0; i < numTags; ++i)
    {
        if (tagPresent[i])
        {
            if ((now - lastSeen[i]) > DOOR_DELAY_MS)
            {
                tagPresent[i] = false;
#if DEBUG_MODE
                DBG_S("[SYS] t/o idx=");
                DBG_VL(i);
#endif
            }
            else
            {
                anyPresent = true;
            }
        }
    }
    if (!anyPresent && doorOpen)
        closeDoor();
}

int8_t SystemCoordinator::findIndexById(const char *id) const
{
    for (uint8_t i = 0; i < intervalCount; ++i)
    {
        if (strncmp(intervals[i].id, id, sizeof(intervals[i].id)) == 0)
            return i;
    }
    return -1;
}

bool SystemCoordinator::addInterval(const char *id, uint16_t startMin, uint16_t endMin, uint8_t daysMask)
{
    if (intervalCount >= MAX_INTERVALS)
    {
#if DEBUG_MODE
        DBG_SL("[SYS] addInterval: full");
#endif
        return false;
    }
    // reject duplicate id
    if (findIndexById(id) >= 0)
    {
#if DEBUG_MODE
        DBG_S("[SYS] addInterval dup id=");
        DBG_VL(id);
#endif
        return false;
    }

    IntervalRecord &rec = intervals[intervalCount++];
    strncpy(rec.id, id, sizeof(rec.id) - 1);
    rec.id[sizeof(rec.id) - 1] = '\0';
    rec.startMin = startMin;
    rec.endMin = endMin;
    rec.daysMask = daysMask;
    rec.enabled = true;
#if DEBUG_MODE
    DBG_S("[SYS] add id=");
    DBG_V(id);
    DBG_S(" s=");
    DBG_VL(startMin);
#endif
    evaluateNow(true);
    return true;
}

bool SystemCoordinator::updateIntervalTime(const char *id, uint16_t startMin, uint16_t endMin, uint8_t daysMask)
{
    int8_t idx = findIndexById(id);
    if (idx < 0)
    {
#if DEBUG_MODE
        DBG_S("[SYS] upd notfound id=");
        DBG_VL(id);
#endif
        return false;
    }
    intervals[idx].startMin = startMin;
    intervals[idx].endMin = endMin;
    intervals[idx].daysMask = daysMask;
#if DEBUG_MODE
    DBG_S("[SYS] upd id=");
    DBG_V(id);
    DBG_S(" s=");
    DBG_V(startMin);
    DBG_S(" e=");
    DBG_VL(endMin);
#endif

    evaluateNow(true);
    return true;
}

bool SystemCoordinator::deleteInterval(const char *id)
{
    int8_t idx = findIndexById(id);
    if (idx < 0)
    {
#if DEBUG_MODE
        DBG_S("[SYS] del notfound id=");
        DBG_VL(id);
#endif

        return false;
    }
    for (uint8_t j = idx; j + 1 < intervalCount; ++j)
        intervals[j] = intervals[j + 1];
    --intervalCount;
#if DEBUG_MODE
    DBG_S("[SYS] del id=");
    DBG_VL(id);
#endif

    evaluateNow(true);
    return true;
}

bool SystemCoordinator::setIntervalStatus(const char *id, bool enabled)
{
    int8_t idx = findIndexById(id);
    if (idx < 0)
    {
#if DEBUG_MODE
        DBG_S("[SYS] setStatus notfound id=");
        DBG_VL(id);
#endif
        return false;
    }
    intervals[idx].enabled = enabled;
#if DEBUG_MODE
    DBG_S("[SYS] setStatus id=");
    DBG_V(id);
    DBG_S(" en=");
    DBG_VL(enabled ? 1 : 0);
#endif

    evaluateNow(true);
    return true;
}

bool SystemCoordinator::setIntervalEnabled(const char *id, bool enabled)
{
    bool ok = setIntervalStatus(id, enabled);
    return ok;
}

bool SystemCoordinator::setIntervalStart(const char *id, uint16_t startMin)
{
    int8_t idx = findIndexById(id);
    if (idx < 0)
    {
#if DEBUG_MODE
        DBG_S("[SYS] setStart notfound id=");
        DBG_VL(id);
#endif
        return false;
    }
    intervals[idx].startMin = startMin;
#if DEBUG_MODE
    DBG_S("[SYS] setStart id=");
    DBG_V(id);
    DBG_S(" s=");
    DBG_VL(startMin);
#endif

    evaluateNow(true);
    return true;
}

bool SystemCoordinator::setIntervalEnd(const char *id, uint16_t endMin)
{
    int8_t idx = findIndexById(id);
    if (idx < 0)
    {
#if DEBUG_MODE
        DBG_S("[SYS] setEnd notfound id=");
        DBG_VL(id);
#endif
        return false;
    }
    intervals[idx].endMin = endMin;
#if DEBUG_MODE
    DBG_S("[SYS] setEnd id=");
    DBG_V(id);
    DBG_S(" e=");
    DBG_VL(endMin);
#endif

    evaluateNow(true);
    return true;
}

bool SystemCoordinator::setIntervalDays(const char *id, uint8_t daysMask)
{
    int8_t idx = findIndexById(id);
    if (idx < 0)
    {
#if DEBUG_MODE
        DBG_S("[SYS] setDays notfound id=");
        DBG_VL(id);
#endif
        return false;
    }
    intervals[idx].daysMask = daysMask;
#if DEBUG_MODE
    DBG_S("[SYS] setDays id=");
    DBG_V(id);
    DBG_S(" m=0x");
    DBG_HEXL(daysMask);
#endif

    evaluateNow(true);
    return true;
}

void SystemCoordinator::evaluateNow(bool forced)
{
#if DEBUG_MODE
    uint16_t nowMinDbg = getCurrentMinutes();
    uint8_t hh24 = nowMinDbg / 60;
    uint8_t mm = nowMinDbg % 60;
    uint8_t hh12 = hh24 % 12;
    if (hh12 == 0)
        hh12 = 12;
    const char *ampm = (hh24 >= 12) ? "PM" : "AM";

    DBG_S("[SYS] check ");
    if (forced)
    {
        DBG_S("(forced) ");
    }
    DBG_S("time ");
    DBG_V(hh24);
    DBG_S(":");
    if (mm < 10)
        DBG_S("0");
    DBG_V(mm);
    DBG_S(" / ");
    DBG_V(hh12);
    DBG_S(":");
    if (mm < 10)
        DBG_S("0");
    DBG_S("");
    DBG_VL(ampm);
#endif

    if (intervalCount == 0)
    {
        rfidEnabled = false;
#if DEBUG_MODE
        DBG_SL("[SYS] eval none -> rfidOff");
#endif
        return;
    }

    uint16_t nowMin = getCurrentMinutes();
    uint8_t todayBit = getTodayMaskBit();

    bool active = false;
    for (uint8_t i = 0; i < intervalCount; ++i)
    {
        const IntervalRecord &rec = intervals[i];
        if (!rec.enabled)
            continue;
        if ((rec.daysMask & todayBit) == 0)
            continue;

        if (rec.startMin <= rec.endMin)
        {
            if (nowMin >= rec.startMin && nowMin < rec.endMin)
            {
                active = true;
                break;
            }
        }
        else
        {
            if (nowMin >= rec.startMin || nowMin < rec.endMin)
            {
                active = true;
                break;
            }
        }
    }

    if (rfidEnabled != active)
    {
#if DEBUG_MODE
        DBG_S("[SYS] eval time=");
        DBG_V(nowMin);
        DBG_S(" -> ");
        DBG_VL(active ? 1 : 0);
        if (active)
            DBG_SL("[SYS] rfid activated (inside interval)");
        else
            DBG_SL("[SYS] rfid deactivated (outside intervals)");
#endif
    }
    rfidEnabled = active;
}

void SystemCoordinator::loop()
{
    unsigned long now = millis();
    if ((now - lastCheckMs) >= COORDINATOR_CHECK_INTERVAL_MS)
    {
        lastCheckMs = now;
        evaluateNow(false);
    }
    checkTagTimeouts();
}

uint16_t SystemCoordinator::getCurrentMinutes() const
{
    unsigned long totalMinutes = (millis() / 60000UL);
    return static_cast<uint16_t>(totalMinutes % (24UL * 60UL));
}

uint8_t SystemCoordinator::getTodayMaskBit() const
{
    unsigned long daysSinceBoot = (millis() / 86400000UL);
    uint8_t dow = static_cast<uint8_t>(daysSinceBoot % 7UL);
    return static_cast<uint8_t>(1u << dow);
}