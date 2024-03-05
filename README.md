# MedTech Glove (ESP32)

IoT wearable glove that monitors body vitals remotely: heart rate (BPM), blood oxygen (SpO2) and a single-lead ECG, streamed over WiFi to a live browser dashboard.

This is a rebuild of the original project firmware after the source files were lost to a hard disk failure. The sensor choices below are the standard modules for this kind of build.

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

## How it works

- **BPM + SpO2**: red and IR samples from the MAX30102 fill a 100-sample window; the Maxim algorithm estimates heart rate and SpO2, re-running every 25 new samples. Readings are marked invalid when no finger is detected.
- **ECG**: the AD8232 output is sampled at 250 Hz into a 2 second ring buffer. The lead-off pins flag when electrodes aren't making contact.
- **Remote monitoring**: the ESP32 runs a web server on the local network.
  - `/` live dashboard (BPM, SpO2, ECG trace)
  - `/vitals` JSON: `bpm`, `spo2`, validity flags, finger/lead status
  - `/ecg` JSON: latest ECG samples

## Setup

1. Install the **SparkFun MAX3010x Pulse and Proximity Sensor Library** in the Arduino IDE.
2. Copy `config.example.h` to `config.h` and set your WiFi name and password.
3. Select your ESP32 board, upload, and open the Serial Monitor at 115200 baud to get the dashboard IP.

## Disclaimer

Prototype for research and competition use. Not a certified medical device and not for diagnosis.
