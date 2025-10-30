# Guv’nuh – Embedded Governor Development Platform (C/C++ Edition)

A laboratory-scale governor that transforms a 0.5 HP induction machine into a **programmable micro-generator**, serving as a complete test-bed for turbine-control firmware, load-response algorithms, and networked telemetry.

---

## Project Purpose

> Deliver a robust, real-time **governor controller** demonstrating the embedded firmware, control theory, and safety-engineering competencies required for professional embedded and power-electronics development.

This version focuses on **C/C++**, deterministic control, and FreeRTOS-based task management across dual MCUs (STM32 + ESP32).
A remote visualization stack hosted on a **VPS** provides live telemetry, replay, and demo access.

---

## Functional Scope (Phase-1)

| ID   | Requirement                              | Target Value / Notes              |
| ---- | ---------------------------------------- | --------------------------------- |
| F-01 | Real-time phase synchronization          | ±1 Hz tolerance @ 50 Hz nominal   |
| F-02 | Load response under transient excitation | < 100 ms stabilization            |
| F-03 | Encoder-PLL speed estimation             | ±0.5 % accuracy, 0–1800 RPM range |
| F-04 | Modbus-RTU link to VFD                   | 115 200 baud, master-mode         |
| F-05 | Isolated MCU bridge (COBS + CRC16)       | 1–3 Mbps, full-duplex DMA         |
| F-06 | MQTT/HTTPS uplink                        | TLS 1.3, 10–50 Hz telemetry       |

Phase-2 will migrate from Modbus RTU to **EtherCAT**, enabling deterministic distributed control and multi-node synchronization.

---

## Repository Structure

```
├── stm32/                  ↳ Real-time control firmware (C/C++ + FreeRTOS)
│   ├── drivers/            ↳ ADS131M04, encoder, RS-485
│   ├── middleware/link/    ↳ UART DMA · COBS · CRC16
│   ├── control/            ↳ PLL · SEIG excitation · fault logic
│   ├── app/                ↳ Config · telemetry · command handler
│   └── rtos/               ↳ Task setup, priorities, hooks
│
├── esp32/                  ↳ Networking & UI gateway (C/C++ + ESP-IDF)
│   ├── link/               ↳ UART bridge identical framing
│   ├── net/                ↳ MQTT · HTTPS · SNTP
│   ├── ota/                ↳ Self-update + STM32 relay
│   ├── ui/                 ↳ Minimal local web status
│   └── rtos/               ↳ FreeRTOS task layout
│
├── docs/                   ↳ Architecture · control flow · standards
├── tests/                  ↳ HIL and long-run link tests
├── tools/                  ↳ Telemetry schema · CRC/COBS generators
├── build/                  ↳ Out-of-tree artifacts
└── ci/                     ↳ Future automated build/test scripts
```

A separate repository, **`guvnuh-digest`**, will handle ingestion, visualization, and long-term storage of telemetry data.

---

## Technology Stack

| Layer           | Implementation / Hardware                      | Rationale                                                             |
| --------------- | ---------------------------------------------- | --------------------------------------------------------------------- |
| **Control MCU** | STM32H753 (NUCLEO-H753ZI) + C/C++ + FreeRTOS   | Deterministic real-time control                                       |
| **Gateway MCU** | ESP32 (Wi-Fi/Ethernet) + ESP-IDF               | Secure MQTT/HTTPS uplink                                              |
| **Fieldbus**    | Modbus-RTU → EtherCAT (Phase-2)                | Legacy + modern compatibility                                         |
| **Sensing**     | ADS131M04 (ΣΔ ADC) + Encoder (AMT10) + INA240s | High-accuracy power measurement                                       |
| **Comms Link**  | UART DMA · COBS · CRC16 (0xA001)               | Binary framing · no delimiters                                        |
| **Data Layer**  | MQTT + TLS + CBOR telemetry                    | Compact · loss-tolerant                                               |
| **Backend**     | Dioxus Fullstack + SurrealDB                   | Unified Rust-based web stack handling API, storage, and visualization |
| **RTOS Choice** | FreeRTOS (primary) · Zephyr (candidate)        | Demonstrates embedded RTOS skills                                     |

---

## Phase-1 Milestones

| Deliverable                  | Competence Demonstrated         |
| ---------------------------- | ------------------------------- |
| UART-DMA loopback both MCUs  | Low-level peripheral bring-up   |
| COBS + CRC link validated    | Deterministic packet transport  |
| SPI DMA ADS131 stream        | High-rate ADC pipeline          |
| Encoder + PLL speed tracking | Signal processing + control     |
| Modbus master → VFD control  | Industrial protocol integration |
| Load response demo           | Real-time loop tuning           |
| MQTT telemetry uplink        | Network gateway implementation  |

---

## Build & Flash

```bash
# STM32 (FreeRTOS build)
cmake -B build/stm32 -S stm32 -DCMAKE_BUILD_TYPE=Release
cmake --build build/stm32 -j
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
  -c "program build/stm32/firmware.elf verify reset exit"

# ESP32 (ESP-IDF)
idf.py set-target esp32
idf.py build flash monitor
```

---

## Visualization & Web Interface

All telemetry is published to a **VPS** via MQTT for digestion and display by a separate repository:

* **`guvnuh-digest`** → Ingests, normalizes, and stores data
* **`guvnuh-web`** → Presents dashboards, demo playback, and system status

This separation ensures the embedded side remains deterministic while still providing rich insights into control performance and system behavior.

---

## Standards & Practices

* **IEC 61508 SIL-2** – Safety life-cycle principles
* **IEC 60204-1** – Control-power segregation
* **ISA-50 / Modbus-RTU** – Industrial protocol reference
* **IEEE 519** – Power quality and harmonic analysis
* **MISRA-C 2012** – Static analysis and safe C subset

---

© 2025 Gauss Industries · MIT License


