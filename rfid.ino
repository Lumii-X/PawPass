#include <Servo.h>

// --- Comando para lectura múltiple RFID --- BB 00 27 00 03 22 FF FF 4A 7E 
unsigned char ReadMulti[10] = {
  0XBB, 0X00, 0X27, 0X00, 0X03, 0X22, 0XFF, 0XFF, 0X4A, 0X7E
};

// --- Pines ---
const int BuzzerPin = 6;
const int BlueLedPin = 5;
const int ServoPin = 9;

/*--- RFID Tag esperado ---
Principal (ACTUAL): E2 00 47 09 3E B0 64 26 B8 4A 01 13
Secundario: E2 00 47 10 80 30 60 26 99 06 01 0B 
*/ 
const unsigned char ExpectedEPC[12] = {
  0xE2, 0x00, 0x47, 0x09, 0x3E, 0xB0, 0x64, 0x26, 0xB8, 0x4A, 0x01, 0x13
};

// --- Variables globales ---
unsigned int TimeSec = 0, TimeMin = 0;
unsigned int DataAdd = 0, Incomedate = 0;
unsigned int ParState = 0, CodeState = 0;
unsigned char EpcBuffer[12];
unsigned int EpcIndex = 0;

bool TagDetected = false;
bool DoorOpen = false;
unsigned long LastTagTime = 0;
const unsigned long DoorDelay = 3000;

Servo DoorServo;

// --- Setup inicial ---
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(BlueLedPin, OUTPUT);

  digitalWrite(BuzzerPin, 0);
  digitalWrite(BlueLedPin, 0);

  DoorServo.attach(ServoPin);
  CloseDoor();

  Serial.begin(115200);
  Serial.println("Sistema RFID iniciado.");

  SendReadCommand();
}

// --- Loop principal ---
void loop() {
  HandleTimedCommand();
  ProcessSerialInput();
  CheckTagTimeout();
  UpdateBlueLed();
}

// --- Envía el comando de lectura RFID ---
void SendReadCommand() {
  Serial.write(ReadMulti, 10);
}

// --- Reenvío de comando cada cierto tiempo ---
void HandleTimedCommand() {
  TimeSec++;
  if (TimeSec >= 50000) {
    TimeMin++;
    TimeSec = 0;

    if (TimeMin >= 20) {
      TimeMin = 0;
      digitalWrite(LED_BUILTIN, 1);
      SendReadCommand();
      digitalWrite(LED_BUILTIN, 0);
    }
  }
}

// --- Procesa entrada del lector RFID ---
void ProcessSerialInput() {
  if (Serial.available() > 0) {
    Incomedate = Serial.read();

    if ((Incomedate == 0x02) && (ParState == 0)) {
      ParState = 1;
    } else if ((ParState == 1) && (Incomedate == 0x22) && (CodeState == 0)) {
      CodeState = 1;
      DataAdd = 3;
      EpcIndex = 0;
    } else if (CodeState == 1) {
      DataAdd++;

      if ((DataAdd >= 9) && (DataAdd <= 20)) {
        if (EpcIndex < 12) {
          EpcBuffer[EpcIndex++] = Incomedate;
        }
      } else if (DataAdd >= 21) {
        HandleTagDetection();
        ResetStates();
      }
    } else {
      ResetStates();
    }
  }
}

// --- Maneja detección del tag ---
void HandleTagDetection() {
  if (IsExpectedTag()) {
    if (!TagDetected) {
      Serial.println("Tag detectado. Abriendo puerta.");
      ActivateBuzzer();
      OpenDoor();
    }
    TagDetected = true;
    LastTagTime = millis();
  }
}

// --- Verifica si el EPC coincide ---
bool IsExpectedTag() {
  for (int i = 0; i < 12; i++) {
    if (EpcBuffer[i] != ExpectedEPC[i]) return false;
  }
  return true;
}

// --- Verifica si el tag desapareció y cierra ---
void CheckTagTimeout() {
  if (TagDetected && (millis() - LastTagTime > DoorDelay)) {
    Serial.println("Tag no detectado. Cerrando puerta.");
    CloseDoor();
    TagDetected = false;
  }
}

// --- Prende o apaga el LED azul en tiempo real ---
void UpdateBlueLed() {
  digitalWrite(BlueLedPin, TagDetected ? 1 : 0);
}

// --- Abre la puerta con recorrido amplio y veloz ---
void OpenDoor() {
  DoorServo.writeMicroseconds(2500);
  DoorOpen = true;
}

// --- Cierra la puerta con recorrido amplio y veloz ---
void CloseDoor() {
  DoorServo.writeMicroseconds(500);  
  DoorOpen = false;
}

// --- Buzzer corto ---
void ActivateBuzzer() {
  digitalWrite(BuzzerPin, 1);
  delay(250);
  digitalWrite(BuzzerPin, 0);
}

// --- Reinicia estados de lectura ---
void ResetStates() {
  DataAdd = 0;
  ParState = 0;
  CodeState = 0;
  EpcIndex = 0;
}