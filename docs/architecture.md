
# Architecture

**Project:** Guv’nuh – Embedded Governor Development Platform (C/C++ Edition)
**Scope:** Dual-MCU SEIG controller with deterministic real-time control (STM32H753) and a network gateway (ESP32) publishing telemetry to a VPS running **Dioxus Fullstack + SurrealDB** (with MQTT broker retained).
**Phase-1 Focus:** Phase synchronization and load response in real time.

---

## 1. System Overview

The system regulates a 3-phase **Self-Excited Induction Generator (SEIG)** using a deterministic control loop on the STM32H753. An ESP32 serves as a **network gateway** for telemetry uplink and remote commands while remaining isolated from plant buses.

```
[VFD] <-- RS-485/Modbus --> [STM32H753] ==(Isolated UART DMA, COBS+CRC16)==> [ESP32] --TLS/MQTT--> [VPS]
         SPI/DRDY   ^               | A/B/Z
[ADS131M04] --------'               '-> [Encoder]

[VPS]: Dioxus Fullstack (API + UI) + SurrealDB + MQTT broker
```

**Key principles**

* **Single bus master:** STM32 owns RS-485 and sensors; ESP32 never touches plant buses.
* **Isolation boundary:** Galvanic isolation across the MCU link to decouple network and plant grounds.
* **Determinism first:** Control loop at **1 kHz**; DMA-first I/O; fixed-priority ISRs.

---

## 2. Hardware Partitioning

**Domains & isolation**

* **Plant domain:** SEIG, VFD, ADS131M04 front-end, INA240 shunts, AMT10 encoder.
* **Control domain:** STM32H753; owns SPI(ADS131), TIMx encoder, RS-485 (Modbus master).
* **Network domain:** ESP32 (Wi-Fi/Ethernet), TLS stack, OTA, local status UI.
* **Isolation:** Digital isolators + isolated DC/DC across STM32↔ESP32 UART; proper **AGND/DGND** partitioning; star-point and chassis bonding per enclosure design.

**Safety**

* STO/contactors for torque-off; brown-out detection; latched faults; independent watchdogs on both MCUs.

---

## 3. Firmware Architecture

### 3.1 STM32H753 (Control MCU)

* **Language/Stack:** C/C++ (+ FreeRTOS optional); LL/HAL for peripherals.
* **Responsibilities:** Sensor acquisition, PLL/speed estimation, SEIG excitation control, Modbus-RTU master, protection.

**Timing model**

* **Control ISR:** 1 kHz (triggered via timer or DRDY-paced schedule).
* **ADC path:** DRDY ISR → SPI DMA → decimate → RMS pipeline → control update.
* **Telemetry packer:** 10–50 Hz, low priority (never blocks control).
* **UART link:** DMA RX/TX, lock-free ring buffers, COBS framing, CRC-16.

**Suggested task layout (if using FreeRTOS)**

| Task        | Priority          | Period       | Notes                           |
| ----------- | ----------------- | ------------ | ------------------------------- |
| ControlLoop | High (ISR-driven) | 1 kHz        | PI/droop, excitation cmd        |
| SensorProc  | High              | 1 kHz / DRDY | Decimation, RMS, plausibility   |
| LinkTx/Rx   | Med               | as needed    | UART DMA service, retries       |
| Telemetry   | Low               | 20–100 ms    | Pack CBOR frames                |
| FaultMgr    | High              | async        | Latch trips, safe state         |
| Housekeep   | Low               | 1 s          | Stats, WDT kick (if not in ISR) |

### 3.2 ESP32 (Network MCU)

* **Language/Stack:** C/C++ (ESP-IDF, FreeRTOS).
* **Responsibilities:** UART bridge, MQTT/HTTPS client, SNTP, OTA, local UI.

**Suggested task layout**

| Task       | Priority | Period    | Notes                     |
| ---------- | -------- | --------- | ------------------------- |
| UartBridge | Med      | as needed | COBS/CRC, backpressure    |
| MqttClient | Med      | 10–100 ms | Publish/subscribe, QoS    |
| TimeSync   | Low      | 60–600 s  | SNTP disciplined clock    |
| OtaAgent   | Low      | async     | Self update + STM32 relay |
| UiServer   | Low      | async     | Minimal status HTTP page  |

---

## 4. Communication Protocols

### 4.1 STM32 ↔ ESP32 (Isolated UART)

* **Baud:** 1–3 Mbps.
* **Framing:** **COBS** (marker-free), 1 frame = `{payload | CRC16 (0xA001)}`.
* **Integrity:** CRC-16 (Modbus poly 0xA001, little-endian).
* **Flow control:** DMA ring buffers + software backpressure; sequence numbers for loss detection.
* **Payloads:** Fixed C structs or **CBOR** blobs (endianness and alignment documented).

**Telemetry frame (example, 50 Hz)**
`telem50_t` (packed):

```
uint32_t seq, t_us;
float v_rms[3], i_rms[3];
float p_total_w, freq_hz, speed_rpm;
uint16_t vfd_status, faults;
```

**Commands (ESP32 → STM32)**

* `SET_VFD_FREQ(float hz)`
* `ENABLE`, `DISABLE`
* `SET_MODE(enum)`
* `CLEAR_FAULTS`
* Future: set droop gains, excitation limits, test modes.

### 4.2 Fieldbus (STM32 ↔ VFD)

* **Protocol:** Modbus-RTU (master).
* **Timing:** 3.5 char idle between frames; 115200 baud; CRC-16 (0xA001).
* **Map:** Minimal: frequency setpoint, run/stop, status words, fault codes.

**Phase-2:** Migrate to **EtherCAT** (CiA-402 style control) on separate hardware path.

### 4.3 ESP32 ↔ VPS

* **Primary:** MQTT over TLS 1.2/1.3 (QoS 1 for critical summaries, QoS 0 for high-rate samples).
* **Topics (proposed)**

  * `guvnuh/<unit_id>/telem/50hz` (CBOR)
  * `guvnuh/<unit_id>/event/fault` (JSON/CBOR)
  * `guvnuh/<unit_id>/cmd/inbox` (JSON command envelope)
* **Secondary:** HTTPS for configs, logs, and OTA triggers.
* **Timebase:** SNTP; ESP32 stamps `t_us`, VPS can echo server time for round-trip skew estimates.

---

## 5. Control Theory & Algorithms

**Objectives**

* Phase synchronization to VFD reference/frequency (`freq_hz`).
* Load response: maintain speed/voltage during step loads via excitation control.

**Loop structure (Phase-1)**

1. **Sensing:** ADS131M04 → RMS(V/I), estimate power/phase.
2. **Speed:** Encoder A/B/Z → PLL → `speed_rpm`, `freq_hz`.
3. **Control:** PI + droop scheduling; clamps and rate limiters.
4. **Actuation:** SEIG excitation control (via VFD reference or dedicated driver), VFD frequency setpoint.

**Design notes**

* **Sample rates:** ADC pipeline ≥ 8–16 kS/s → decimate → 1 kHz control.
* **Filters:** IIR or sliding RMS; phase-aligned measurement windows to reduce bias.
* **Protection:** Overcurrent, brown-out, overspeed → latched fault → torque-off path.

---

## 6. Data Model & Backend (Dioxus Fullstack + SurrealDB)

**Ingestion**

* MQTT subscriber within Dioxus backend ingests telemetry frames and persists to **SurrealDB**.
* Command endpoints (HTTPS/WS) enqueue messages to `guvnuh/<unit_id>/cmd/inbox`.

**Storage (SurrealDB) – example tables**

* `device{ id, hw_rev, fw_rev, last_seen, status }`
* `sample{ device_id, t_us, v_rms[3], i_rms[3], p_total_w, freq_hz, speed_rpm, faults }`
* `event{ device_id, ts, kind, payload }`
* `cmd{ device_id, ts, user, cmd, args, result }`

**UI (Dioxus)**

* Live charts (down-sampled feeds), run replays, fault timelines.
* Device inventory & health; configuration panels (read-only in Phase-1 except whitelisted commands).

---

## 7. RTOS & Scheduling

**FreeRTOS (primary)**

* Fixed priorities: ISRs > Control > Comms > Telemetry > UI/Housekeeping.
* Avoid priority inversion (bounded blocking; prefer lock-free where possible).
* DMA everywhere feasible (SPI, UART, possibly timers) to reduce ISR cost.

**Zephyr (candidate)**

* Future consolidation path; consider for EtherCAT stacks and unified logging/tracing.

---

## 8. Fault Handling & Safe States

**Fault classes**

* **A (immediate):** Overspeed, overcurrent, UVLO on control rail → torque-off, excitation disable.
* **B (recoverable):** Link loss, MQTT offline, sensor out-of-range → degraded mode, no torque-off.
* **C (advisory):** Timestamp skew, packet loss, non-critical sensor mismatch.

**Mechanisms**

* Latching faults with explicit `CLEAR_FAULTS`.
* Redundant sensing (shunt + CT optional), plausibility checks.
* Independent WDTs on both MCUs; brown-out reset configured.

**Safe state definition**

* Contactors open (where applicable), excitation = 0, VFD stop; command path locked until faults cleared.

---

## 9. Testing & Verification

**Bring-up**

* GPIO blinks, UART-DMA loopback on both MCUs.
* COBS/CRC soak at target baud with randomized payloads.

**HIL harness**

* Recorded/parametric sensor frames → feed STM32 at real rates.
* Fault injection: bus noise, frame loss, VPS down, clock skew.

**Acceptance (Phase-1)**

* Phase lock within ±1 Hz at 50 Hz nominal.
* Load step (TBD %) settles < 100 ms; no protective trips in normal envelope.
* Link BER < 1e-9 over 24-hour soak; zero control overruns.

---

## 10. Migration Plan (Phase-2: EtherCAT)

* Introduce EtherCAT master (or slave, depending on VFD/IO choice) on STM32 or companion NIC.
* Map control words/status (CiA-402-like) and migrate timing base to DC-synchronized domain.
* Keep ESP32 gateway unchanged (topic schema stable), or add ECAT health telemetry.

---

## 11. Interfaces & Maps (Placeholders)

**Modbus (VFD) minimal map (example)**

* `0x2000` Frequency setpoint (0.01 Hz units)
* `0x2001` Run/stop control word
* `0x2100` Status word, faults, actual freq

**STM32 GPIO/Periph (to be finalized)**

* `SPIx` = ADS131M04 (CS/CLK/MISO/MOSI, DRDY IRQ)
* `TIMx` = Encoder mode (A/B/Z)
* `USARTx` = RS-485 (DE/RE control), `USARTy` = Isolated link to ESP32

---

## 12. Security & Resilience

* TLS on MQTT/HTTPS; device credentials provisioned per unit.
* Command whitelist on VPS; audit log for all remote actions.
* Rate limiting/backpressure from ESP32 to avoid UDP-like storms; on-disk queue optional (ESP32 PSRAM/flash considerations).

---

## 13. Documentation Map

* `README.md` – goals, stack, milestones, build steps.
* `docs/architecture.md` – (this file) full system rationale.
* `docs/controls/` – PLL, RMS filters, droop design notes and math.
* `docs/interfaces/` – UART frame formats, MQTT topics, Modbus map.
* `docs/safety/` – fault tree, test procedures, trip levels.
* `docs/bench/` – wiring, EMI, enclosure, grounding.

---

## 14. Open Questions / TODO

* Finalize UART baud target and buffer sizes under max telemetry rate.
* Decide FreeRTOS vs bare-metal for STM32 Phase-1; (ESP32 remains FreeRTOS/ESP-IDF).
* Confirm MQTT topic naming and QoS per stream.
* Define exact EtherCAT hardware path and stack (Phase-2).
* Specify STO/contactors wiring and hardware interlock timing.

---

**End of document**
