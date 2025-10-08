#ifndef PAWPASS_RFID_H
#define PAWPASS_RFID_H

#include <Arduino.h>
#include <Servo.h>
#include "config.h"

class RFIDManager
{
public:
  RFIDManager() = default;
  void begin(unsigned long baud = 115200);
  void update(); // call from loop()

#ifdef RFID_SIMULATION
  void simulateTag(const uint8_t *tag);
#endif

private:
  enum class ParseState : uint8_t
  {
    WAIT_START = 0,
    GOT_START = 1,
    GOT_CODE = 2
  };
  enum class GateState : uint8_t
  {
    CLOSED = 0,
    OPEN = 1
  };
  enum class SystemState : uint8_t
  {
    IDLE = 0,
    READING = 1,
    DOOR_OPEN = 2,
    DOOR_CLOSING = 3
  };

  SystemState systemState = SystemState::READING;

  unsigned long lastByteMs = 0;
  ParseState state = ParseState::WAIT_START;
  uint8_t dataAdd = 0;
  uint8_t epcIndex = 0;
  uint8_t epcBuf[EPC_LENGTH];

  unsigned long lastSendMs = 0;
  unsigned long lastTagTime = 0;
  bool tagDetected = false;

  GateState gateState = GateState::CLOSED;
  Servo doorServo;

  void sendReadCommand();
  void processByte(uint8_t b) __attribute__((hot));
  void handleTagDetection();
  bool isAuthorizedTag() const;
  inline void openDoor();
  inline void closeDoor();
  void checkTagTimeout();
  inline void resetParser();
};

#endif // PAWPASS_RFID_H