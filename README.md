<div align="center">

# MedTech Glove (ESP32)

**IoT wearable glove streaming BPM, SpO&#8322; and single-lead ECG to a live browser dashboard**

![Domain](https://img.shields.io/badge/Domain-Biomedical_Signal_Processing-00F3FF?style=for-the-badge) ![Platform](https://img.shields.io/badge/Platform-ESP32_+_WiFi-9D00FF?style=for-the-badge) ![Interface](https://img.shields.io/badge/Interface-Web_Dashboard-0066FF?style=for-the-badge)

![ESP32](https://img.shields.io/badge/ESP32-0D1117?style=flat-square&logo=espressif&logoColor=white) ![Arduino](https://img.shields.io/badge/Arduino-0D1117?style=flat-square&logo=arduino&logoColor=white) ![C++](https://img.shields.io/badge/C++-0D1117?style=flat-square&logo=cplusplus&logoColor=white) ![MAX30102](https://img.shields.io/badge/MAX30102-0D1117?style=flat-square) ![AD8232](https://img.shields.io/badge/AD8232-0D1117?style=flat-square) ![HTTP_JSON](https://img.shields.io/badge/HTTP_JSON-0D1117?style=flat-square)

</div>

---

## Overview

A wearable glove that measures three vitals at once — heart rate and blood oxygen from a fingertip PPG
sensor, plus a single-lead ECG trace — and serves them over WiFi to any browser on the local network.
The device is its own web server: no app to install, no cloud account, no external dependency.

This is a rebuild of the original project firmware after the source files were lost to a hard disk
failure. The sensor choices below are the standard modules for this class of build.

## Domain &amp; Techniques

| Layer | Implementation |
| :--- | :--- |
| **PPG Processing** | MAX30102 red/IR channels fill a 100-sample rolling window; the Maxim SpO&#8322; algorithm re-estimates heart rate and oxygen saturation every 25 new samples |
| **ECG Acquisition** | AD8232 output sampled at 250 Hz into a 2-second ring buffer for continuous waveform streaming |
| **Validity Gating** | Finger-presence detection on the PPG path and lead-off detection on the ECG path mark readings invalid rather than emitting garbage |
| **Telemetry** | On-device HTTP server exposes a live dashboard plus `/vitals` and `/ecg` JSON endpoints for external consumers |

## Pipeline

```
MAX30102 (red + IR)  ---> 100-sample window ---> Maxim SpO2 algorithm ---> BPM, SpO2
                                                        |
AD8232 (single lead) ---> 250 Hz ring buffer -----------+---> ESP32 HTTP server
                                                                 |
                                            /  (dashboard)  /vitals  /ecg  (JSON)
```

## Hardware

| Part | Purpose |
| --- | --- |
| ESP32 dev board | Microcontroller + WiFi |
| MAX30102 | PPG sensor for heart rate and SpO2 (fingertip) |
| AD8232 | ECG front end (3 electrodes) |
| Li-ion cell + charger/boost | Wearable power |

## Wiring

| Signal | ESP32 pin |
| --- | --- |
| MAX30102 SDA | GPIO 21 |
| MAX30102 SCL | GPIO 22 |
| AD8232 OUTPUT | GPIO 36 (ADC1) |
| AD8232 LO+ | GPIO 32 |
| AD8232 LO- | GPIO 33 |

Power both sensors from 3.3V.

## API

| Endpoint | Returns |
| --- | --- |
| `/` | Live dashboard (BPM, SpO2, ECG trace) |
| `/vitals` | JSON: `bpm`, `spo2`, validity flags, finger/lead status |
| `/ecg` | JSON: latest ECG samples |

## Repository Layout

| Path | Purpose |
| :--- | :--- |
| `MedTechGlove.ino` | Main firmware — sensor polling, algorithm dispatch, web server |
| `config.example.h` | WiFi credentials and tuning constants — copy to `config.h` |

## Project Status

**Implemented:** PPG heart rate and SpO&#8322; estimation, 250 Hz ECG capture, validity gating, live
WiFi dashboard with JSON endpoints.

**Roadmap:** on-device arrhythmia flagging from the ECG buffer, and persistent logging to SPIFFS so a
session survives a power cycle.

> Research prototype. Not a certified medical device.

---

<div align="center">
  <sub>
    Part of the <b>AI + Robotics</b> engineering portfolio of
    <a href="https://github.com/divyansh-sachdev">Divyansh Sachdev</a><br>
    90+ national &amp; international competition wins &middot; IIT / NIT / IIIT podiums
  </sub>
</div>
