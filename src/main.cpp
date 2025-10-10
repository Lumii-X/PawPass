#include <Arduino.h>
#include "Core/SystemCoordinator.h"
#include "Bluetooth/BluetoothManager.h"
#include "RFID/RFIDManager.h"

SystemCoordinator coordinator;
BluetoothManager bt(coordinator);
RFIDManager rfid(coordinator);

void setup()
{
    Serial.begin(115200);
    coordinator.begin();
    bt.begin();
    rfid.begin();
#if DEBUG_MODE
    DBG_SL("PawPass init");
#endif
}

void loop()
{
    bt.loop();
    rfid.loop();
    coordinator.loop();
    delay(10);
}