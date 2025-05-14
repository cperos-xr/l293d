# RP2040-Based 8-Motor Bidirectional Driver (UART-Controlled)

This project is a compact, UART-controlled 8-motor driver board powered by an external **RP2040 Zero** module. It uses 4× L293D motor driver ICs and is designed for simple, scalable motor control — ideal for robotics, kinetic art, or automation.

---

## 🖼️ Visual Overview

### L293D Motor Driver Board (This Repo)
**Simulated 3D View**  
![3D Render](https://github.com/cperos-xr/l293d/raw/main/L293dBoard.png)

**PCB X-Ray View**  
![PCB Layer View](https://github.com/cperos-xr/l293d/raw/main/L293dBoard2.png)

### RP2040 Zero (Required External MCU)  
![RP2040 Zero](https://github.com/cperos-xr/l293d/raw/main/2040_zero.jpg)  
> Sold separately – see links below

---

## 🔧 Hardware Overview

- **Microcontroller**: RP2040 Zero (mounted externally)
- **Motor Drivers**: 4× L293D (each drives 2 motors bidirectionally)
- **Capacitors**:
  - 1× 820μF bulk capacitor on motor power rail
  - 4× 100μF (one per L293D) for local decoupling
- **Voltage Breakouts**: 3.3V, 5V, GND
- **Screw Terminals**:
  - 8× motor outputs
  - 2× power inputs (external voltage)
- **Status LED**: Blinking RGB LED on GPIO 16
- **GPIO Usage**:
  - GPIO 0–15: Motor PWM outputs
  - GPIO 28 (TX) / 29 (RX): UART communication
  - GPIO 26 / 27: Available for future I²C or GPIO expansion

> ⚠️ L293D drivers may run warm at 7.2V; consider passive cooling or heatsinks for sustained high-current applications.

---

## 💻 Firmware: UART-Controlled Motor Slave

- Receives 8 signed bytes (`int8_t`, range -128 to 127) via UART
- Each byte controls one motor’s direction and PWM duty
- RGB LED indicates heartbeat/status
- UART on GPIO 29 (RX), GPIO 28 (TX)
- Written in C with the Pico SDK

---

## 📤 Master Controller Code

| Platform      | Interface  | Status     |
|---------------|------------|------------|
| RP2040 (C)    | UART       | ✅ Complete |
| Arduino       | UART       | ✅ Complete |
| RP2040 (C)    | USB Serial | ⏳ In Progress |
| MicroPython   | UART/USB   | ⏳ Planned |
| CircuitPython | UART/USB   | ⏳ Planned |

Each master sends 8 bytes over UART or USB. Slave interprets them in order to drive each motor.

> The Pico SDK offers reliable UART pin mapping, unlike Arduino/MicroPython which had issues on RP2040 — thus slave code is C-only for now.

---

## 🧰 Files in This Repo

| File | Description |
|------|-------------|
| `uartSlave.c` | RP2040 C firmware for motor control over UART |
| `uartMaster.c` | RP2040 C example of a UART master |
| `uartMaster_arduino.ino` *(coming soon)* | Arduino sketch to drive 8 motors |

Compiled `.uf2` binaries will be included shortly for drag-and-drop flashing.

---

## 🛒 Buy an RP2040 Zero Module

You’ll need a compatible RP2040 Zero board. Here are some examples:

- [AliExpress – Option 1 ($3.32)](https://www.aliexpress.us/item/3256806899545831.html)
- [AliExpress – Option 2 ($3.37)](https://www.aliexpress.us/item/3256805942102792.html)
- [Amazon – Search “RP2040 Zero”](https://www.amazon.com/s?k=rp2040+zero)

---

## 🔩 Kits & DIY Options

Kits will be available soon via:

- [Tindie](https://www.tindie.com)
- [eBay](https://www.ebay.com)
- [Etsy](https://www.etsy.com)

You'll be able to purchase:

- **Kit with Components (Unsoldered)**  
  Includes the custom PCB and all required components (L293Ds, capacitors, screw terminals, etc.).  
  🔧 *You solder the parts yourself.*  
  Just provide your own **RP2040 Zero** (sold separately), flash it with the provided slave `.uf2` firmware, solder the kit components, and you're ready to plug it in and go.

- **Bare PCB Only**  
  For advanced builders who want full control over parts sourcing or layout variants.  
  *(Bring your own L293Ds, capacitors, terminals, etc.)*

> Note: The RP2040 Zero is **not included** in either option — purchase it separately (see links above).

---

## 🚀 Roadmap

| Feature                          | Status         |
|----------------------------------|----------------|
| UART Slave (C, RP2040 SDK)       | ✅ Complete     |
| UART Master (C and Arduino)      | ✅ Complete     |
| USB Communication (Slave/Master) | 🔄 In Progress  |
| MicroPython/CircuitPython        | 🔄 Planned      |
| Kit Sales                        | 🔜 Coming Soon  |
| Board Schematics                 | 🔜 In Progress  |

---

## 📎 License

MIT License — use it freely with attribution.

---

## 🙌 Contribute / Follow Along

- Fork the repo
- Open issues for bugs or feature requests
- PRs welcome

Join the roadmap — more firmware backends, automation, and compatibility layers are coming!
