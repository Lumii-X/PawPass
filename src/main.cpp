#include <Arduino.h>
#include "rfid.h"

static RFIDManager rfid;

void setup()
{
  rfid.begin(115200);
}

void loop()
{
  rfid.update();
}