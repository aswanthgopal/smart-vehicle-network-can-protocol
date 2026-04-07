# Smart Vehicle Network Using CAN Protocol

This project demonstrates a small-scale automotive communication system using the CAN protocol — the same protocol used in modern vehicles to connect ECUs. Three Arduino Uno boards share a CAN bus: two slave nodes acquire sensor data and transmit it, while the master node acts as a dashboard, displaying information and generating alerts through an LCD, LEDs, and a buzzer.

---

## System Architecture

```
                      CAN Bus (CANH / CANL)
       ┌────────────────────────────────────────────┐
       │                                            │
 ┌─────┴──────┐   ┌──────────────┐   ┌─────────────┴────┐
 │   Master   │   │  Slave Node 1│   │  Slave Node 2    │
 │            │   │              │   │                  │
 │ Arduino +  │   │ Arduino +    │   │ Arduino +        │
 │ MCP2515    │   │ MCP2515      │   │ MCP2515          │
 │            │   │              │   │                  │
 │ LCD (I2C)  │   │ HC-SR04      │   │ Potentiometer    │
 │ LEDs       │   │ LM35         │   │ IR Sensor        │
 │ Buzzer     │   │ Push Button  │   │                  │
 └────────────┘   └──────────────┘   └──────────────────┘
                  IDs: 0x001–0x003    IDs: 0x005–0x006
```

---

## CAN Message Priority Table

| Priority | ID      | Source  | Function             | Data            |
| -------- | ------- | ------- | -------------------- | --------------- |
| 1        | `0x001` | Slave 1 | Airbag trigger       | `1` = triggered |
| 2        | `0x002` | Slave 1 | Obstacle detection   | Distance (cm)   |
| 3        | `0x003` | Slave 1 | Engine temperature   | Temp (°C)       |
| 4        | `0x005` | Slave 2 | Fuel level           | % (0–100)       |
| 5        | `0x006` | Slave 2 | Anti-theft detection | `1` = intrusion |

Lower CAN ID corresponds to higher priority during arbitration.

---

## Hardware Prototype

![Assembled Prototype](images/setup_photo.jpg)

---

## Demonstration

Full system demonstration:
https://drive.google.com/file/d/1ssG3E50oZ8ezO1lP6_U3xkvl5wll6AJh/view?usp=drive_link

LCD output demonstration:
https://drive.google.com/file/d/14Nsaj2f2EVQumHv7oDtQgC4Jerk5dEgV/view?usp=drive_link

---

## Components

| Component                                     | Qty         |
| --------------------------------------------- | ----------- |
| Arduino Uno                                   | 3           |
| MCP2515 CAN module (with TJA1050 transceiver) | 3           |
| 120 Ω termination resistors                   | 2           |
| HC-SR04 ultrasonic sensor                     | 1           |
| LM35 temperature sensor                       | 1           |
| Push button                                   | 1           |
| Potentiometer                                 | 1           |
| IR sensor module                              | 1           |
| 16×2 LCD with I²C backpack                    | 1           |
| Piezo buzzer                                  | 1           |
| Red / Green / Yellow LEDs                     | 1 each      |
| 220 Ω resistors                               | 3           |
| Breadboards and jumper wires                  | As required |

---

## Wiring

### MCP2515 → Arduino (same for all nodes)

| MCP2515   | Arduino | Note          |
| --------- | ------- | ------------- |
| VCC       | 5V      |               |
| GND       | GND     |               |
| SCK       | D13     | Hardware SPI  |
| SI (MOSI) | D11     | Hardware SPI  |
| SO (MISO) | D12     | Hardware SPI  |
| CS        | D10     | Configurable  |
| INT       | D2      | Interrupt pin |

### Slave Node 1

| Component   | Pin             | Arduino           |
| ----------- | --------------- | ----------------- |
| HC-SR04     | Trig            | D3                |
| HC-SR04     | Echo            | D4                |
| LM35        | Signal (middle) | A1                |
| LM35        | VCC / GND       | 5V / GND          |
| Push button | Leg A           | D6 (INPUT_PULLUP) |
| Push button | Leg B           | GND               |

### Slave Node 2

| Component     | Pin   | Arduino |
| ------------- | ----- | ------- |
| Potentiometer | Wiper | A0      |
| Potentiometer | Ends  | 5V, GND |
| IR sensor     | OUT   | D9      |

### Master Node

| Component  | Connection          |
| ---------- | ------------------- |
| LCD (I²C)  | SDA → A4, SCL → A5  |
| Buzzer     | `+` → D5, `−` → GND |
| Red LED    | D6 → 220 Ω → GND    |
| Green LED  | D7 → 220 Ω → GND    |
| Yellow LED | D8 → 220 Ω → GND    |

> CANH and CANL are connected in parallel across all nodes, with 120 Ω termination resistors at both ends of the bus.

---

## Circuit Diagrams

| Slave Node 1 | Slave Node 2 | Master Node |
|--------------|--------------|-------------|
| ![](images/slave1_circuit.png) | ![](images/slave2_circuit.png) | ![](images/master_circuit.png) |

## Software Design

### Interrupt-driven reception

The master node uses a hardware interrupt (D2) to detect incoming CAN messages. All pending frames are processed without loss.

### Non-blocking timing

Timing operations use `millis()` instead of `delay()`, allowing continuous CAN communication.

```cpp
void updateBuzzer() {
    if (buzzerActive && (millis() - buzzerStartTime >= buzzerDuration)) {
        noTone(BUZZER);
        buzzerActive = false;
    }
}
```

### Edge detection

Event-based sensors transmit data only on state transitions, avoiding repeated messages.

### Delta filtering

Sensor values are transmitted only when changes exceed defined thresholds, reducing bus load.

---

## Libraries Required

| Library             | Author             |
| ------------------- | ------------------ |
| `mcp_can`           | coryjfowler        |
| `LiquidCrystal_I2C` | Frank de Brabander |

---

## Getting Started

1. Upload `slave_node_1.ino` to the first Arduino
2. Upload `slave_node_2.ino` to the second Arduino
3. Upload `master_node.ino` to the third Arduino
4. Connect CAN bus and ensure proper wiring
5. Power the system

---

## Troubleshooting (Practical Issues Encountered)

During development and testing, the following issues were encountered and resolved:

| Issue                            | Observation                                     | Resolution                                                            |
| -------------------------------- | ----------------------------------------------- | --------------------------------------------------------------------- |
| CAN initialization failure       | MCP2515 failed to initialize consistently       | Verified SPI wiring and correct oscillator configuration (`MCP_8MHZ`) |
| LCD not displaying data properly | Distance and alerts were not visible on the LCD | Identified timing mismatch and restored appropriate update timing     |
| LCD flickering rapidly           | Display refreshed too frequently                | Reduced update rate and updated only when values changed              |
| Incorrect I²C address            | LCD remained blank despite correct wiring       | Used correct address (`0x27` / `0x3F`)                                |
| Sensor inaccuracies              | Invalid or unstable readings observed           | Corrected wiring and reference configuration                          |
| Repeated alerts                  | Same event transmitted continuously             | Implemented edge detection                                            |

---

## Authors

Aswanth Gopal, Arjun V
Department of Electronics and Communication Engineering
National Institute of Technology Calicut
