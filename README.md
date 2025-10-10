```markdown
<!-- Banner -->
![PawPass Banner](assets/banner.png)

# 🐾 PawPass  
**RFID-Based Smart Access System for Pets**  
Developed by **Lumi**

---

## 🧠 Overview
**PawPass** is an embedded access-control system that automates door movement using **UHF RFID detection**, **Bluetooth configuration**, and a **servo-controlled locking mechanism**, coordinated by a centralized system core.

The design follows strict **low-level optimization principles**, ensuring **deterministic timing**, **no dynamic memory allocation**, and **high reliability** on resource-limited AVR boards (Arduino Uno / Nano).

---

## ⚙️ System Architecture
![System Diagram](assets/system-architecture.png)

### Core Components
- **RFID Reader (YRM1003)** – Handles multi-tag reads via serial using the `ReadMulti` command.  
- **SystemCoordinator** – Manages time intervals, device states, and RFID enable windows.  
- **ServoController** – Executes door movement with PWM timing and configurable delay logic.  
- **Bluetooth Module (HC-05)** – Optional control channel for runtime interval configuration.  
- **EEPROM/PROGMEM Storage** – Stores default tags and configuration constants in flash memory.

---

## 🧩 Core Workflow
![Flow Diagram](assets/system-flow.png)

1. `SystemCoordinator` polls time and evaluates active intervals.  
2. If within an enabled time window, the RFID reader activates and scans.  
3. A valid tag triggers the servo open routine.  
4. Loss of tag detection starts a 3-second countdown before automatic closure.  
5. All subsystems log state transitions when `DEBUG_MODE` is enabled.

---

## 🧱 Memory & Performance Design

- **No heap allocation:** All buffers, tag arrays, and structs are statically defined.  
- **Tag list in flash (`PROGMEM`)** with RAM-mirrored array for runtime extension.  
- **Compact parsing** of EPC frames (12 bytes) via pointer iteration.  
- **Interval evaluation** uses integer arithmetic only (minutes since midnight).  
- **Flash string macros (`F()`)** to reduce SRAM load on debug prints.  
- **Serial operations** isolated from timing loops to prevent blocking.

---

## 🧭 Data Flow
![Data Diagram](assets/dataflow.png)

| Source | Process | Output |
|---------|----------|--------|
| RFID → UART | Frame parse (12B EPC) | Tag match |
| Coordinator | Time interval check | RFID enable / disable |
| Bluetooth | Interval or tag command | Update runtime structures |
| ServoCtrl | Door action | PWM control signal |

> Replace this section with your own diagram if you prefer visual layout.

---

## 🔬 Debugging Interface

Debugging is done through `DBG_*` macros:  
- `DBG_S()` / `DBG_SL()` – static strings (stored in flash)  
- `DBG_V()` / `DBG_VL()` – print runtime values  
- `DBG_HEX()` / `DBG_HEXL()` – print byte sequences in hex  

Logs use lightweight serial prints with controlled verbosity.  
Example output sequence:
```

[RFID] Tag detected: 300833B2DDD9014000000000
[SERVO] OPEN → delay(3000) → CLOSE
[SYS] Interval #2 active (06:00a–10:00p)

```

---

## 🧩 Key Modules (src/Core)

| Module | Responsibility |
|---------|----------------|
| `SystemCoordinator.cpp` | Manages timing, active intervals, RFID enable states |
| `RFIDManager.cpp` | Handles command sequencing and EPC parsing |
| `ServoController.cpp` | Servo PWM logic and door movement delay |
| `BluetoothHandler.cpp` | Parses commands for interval creation/update/deletion |
| `Config.h` | Hardware mapping, constants, and compile-time flags |

---

## 💡 Engineering Notes

- Designed for **deterministic loop timing** at 1-second base cycle.  
- Fully **interrupt-free logic** — no ISRs to maintain predictable state handling.  
- Memory footprint < 2 KB SRAM (Arduino Uno).  
- Easily portable to **ESP32 / STM32** by abstracting serial and timing layers.  
- Build system: **PlatformIO**, modular include hierarchy under `/src/Core/`.  

---

## 🧠 Design Philosophy

> *“Efficiency is not about making it faster — it’s about making it precise.”*  
> — Lumi

PawPass was built to show that **high-level design principles** — modularity, stateless data flow, and memory determinism — can exist even within a small 8-bit microcontroller.  
The goal: **stable embedded logic that feels like engineered software**, not hobby code.

---

## 📜 License
**MIT License** — open use, modification, and distribution with credit to **Lumi**.

---

<!-- Image placeholders -->
![Prototype](assets/prototype.jpg)
![Bluetooth App UI](assets/app.png)
```

---
