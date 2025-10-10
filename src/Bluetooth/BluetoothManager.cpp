#include "Bluetooth/BluetoothManager.h"
#include <stdlib.h>
#include <string.h>

BluetoothManager::BluetoothManager(SystemCoordinator &coord)
    : coordinator(coord), btSerial(BT_RX_PIN, BT_TX_PIN)
{
}

void BluetoothManager::begin()
{
    btSerial.begin(BT_BAUD);
#if DEBUG_MODE
    DBG_SL("[BT] ok");
#endif
}

bool BluetoothManager::readLine(char *outBuf, size_t maxLen, unsigned long timeoutMs)
{
    if (maxLen == 0)
        return false;
    size_t idx = 0;
    unsigned long start = millis();
    bool sawChar = false;

    while (millis() - start < timeoutMs && idx + 1 < maxLen)
    {
        if (btSerial.available())
        {
            int c = btSerial.read();
            sawChar = true;
            if (c == '\r')
                continue;
            if (c == '\n')
                break;
            outBuf[idx++] = static_cast<char>(c);
        }
        else
        {
            delay(1);
        }
    }
    outBuf[idx] = '\0';
    trimInPlace(outBuf);
    return sawChar && outBuf[0] != '\0';
}

void BluetoothManager::trimInPlace(char *s)
{
    if (!s)
        return;
    char *start = s;
    while (*start && isspace((unsigned char)*start))
        ++start;
    if (start != s)
        memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
    {
        s[--len] = '\0';
    }
}

uint16_t BluetoothManager::parse12hToMinutes(const char *token) const
{
    if (!token || !*token)
        return 0;
    size_t L = strlen(token);
    char suf = tolower((unsigned char)token[L - 1]);
    char digits[5] = {0};
    size_t dlen = (L > 1) ? (L - 1) : 0;
    if (dlen > 4)
        dlen = 4;
    memcpy(digits, token, dlen);
    int val = atoi(digits);
    int hh = val / 100;
    int mm = val % 100;
    if (mm > 59)
        mm = 59;
    if (hh > 12)
        hh = 12;
    if (suf == 'p' && hh != 12)
        hh += 12;
    if (suf == 'a' && hh == 12)
        hh = 0;
    return static_cast<uint16_t>(hh * 60 + mm);
}

uint8_t BluetoothManager::parseDaysListToMask(const char *token) const
{
    if (!token)
        return 0;
    uint8_t mask = 0;
    const char *p = token;
    while (*p && (*p == ' ' || *p == '['))
        ++p;
    while (*p && *p != ']')
    {
        while (*p && isspace((unsigned char)*p))
            ++p;
        int n = 0;
        bool has = false;
        while (*p && isdigit((unsigned char)*p))
        {
            has = true;
            n = n * 10 + (*p - '0');
            ++p;
        }
        if (has && n >= 1 && n <= 7)
        {
            if (n == 7)
                mask |= (1u << 0);
            else
                mask |= (1u << (uint8_t)n);
        }
        while (*p && !isdigit((unsigned char)*p) && *p != ']')
            ++p;
    }
    return mask;
}

void BluetoothManager::handleCreateTokens(char *tokens[], int count)
{
#if DEBUG_MODE
    DBG_SL("[BT] create");
#endif
    if (count < 4)
        return;
    const char *id = tokens[1];
    uint16_t start = parse12hToMinutes(tokens[2]);
    uint16_t end = parse12hToMinutes(tokens[3]);
    uint8_t daysMask = 0x7F;
    if (count >= 5 && tokens[4] && tokens[4][0] != '\0')
        daysMask = parseDaysListToMask(tokens[4]);
    coordinator.addInterval(id, start, end, daysMask);
}

void BluetoothManager::handleUpdateTokens(char *tokens[], int count)
{
#if DEBUG_MODE
    DBG_SL("[BT] update");
#endif
    if (count < 4)
        return;
    const char *id = tokens[1];
    const char *dir = tokens[2];
    const char *val = tokens[3];

    if (strcasecmp(dir, "st") == 0)
    {
        bool enabled = (strcasecmp(val, "true") == 0 || strcmp(val, "1") == 0 || strcasecmp(val, "on") == 0);
        coordinator.setIntervalEnabled(id, enabled);
    }
    else if (strcasecmp(dir, "t1") == 0)
    {
        uint16_t start = parse12hToMinutes(val);
        coordinator.setIntervalStart(id, start);
    }
    else if (strcasecmp(dir, "t2") == 0)
    {
        uint16_t end = parse12hToMinutes(val);
        coordinator.setIntervalEnd(id, end);
    }
    else if (strcasecmp(dir, "dt") == 0)
    {
        uint8_t mask = parseDaysListToMask(val);
        coordinator.setIntervalDays(id, mask);
    }
}

void BluetoothManager::handleDeleteTokens(char *tokens[], int count)
{
#if DEBUG_MODE
    DBG_SL("[BT] delete");
#endif
    if (count < 2)
        return;
    coordinator.deleteInterval(tokens[1]);
}

void BluetoothManager::loop()
{
    char line[LINE_BUF];
    while (btSerial.available())
    {
        if (!readLine(line, sizeof(line), 2000))
            break;
#if DEBUG_MODE
        DBG_S("[BT] RX:");
        DBG_VL(line);
#endif
        if (line[0] == '\0')
            continue;
        char *tokens[6] = {0};
        int tokCount = 0;
        char *p = line;
        while (*p && isspace((unsigned char)*p))
            ++p;
        while (*p && tokCount < 4)
        {
            tokens[tokCount++] = p;
            while (*p && !isspace((unsigned char)*p))
                ++p;
            if (!*p)
                break;
            *p++ = '\0';
            while (*p && isspace((unsigned char)*p))
                ++p;
        }
        if (*p)
        {
            tokens[tokCount++] = p;
            char *end = p + strlen(p) - 1;
            while (end >= p && isspace((unsigned char)*end))
            {
                *end = '\0';
                --end;
            }
        }

        if (tokCount == 0)
            continue;

        if (tokens[0] && strcasecmp(tokens[0], "s") == 0)
            handleCreateTokens(tokens, tokCount);
        else if (tokens[0] && strcasecmp(tokens[0], "upd") == 0)
            handleUpdateTokens(tokens, tokCount);
        else if (tokens[0] && strcasecmp(tokens[0], "d") == 0)
            handleDeleteTokens(tokens, tokCount);
    }
}