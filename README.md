# 🏭 Smart Industrial Sensor Node (ESP32 | FreeRTOS)

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.x-red.svg)
![FreeRTOS](https://img.shields.io/badge/OS-FreeRTOS-blue.svg)
![Language](https://img.shields.io/badge/Language-C-00599C.svg)
![Protocol](https://img.shields.io/badge/Protocol-MQTT_v5.0-660066.svg)
![Platform](https://img.shields.io/badge/Cloud-ThingsBoard-00547B.svg)

An industrial-grade, fault-tolerant edge computing node built on the ESP32 platform. Designed for predictive maintenance and real-time monitoring in harsh environments, this system securely collects, processes, and transmits telemetry data (Vibration, Temperature, Current) to the Cloud.

This project emphasizes **System Reliability**, **Real-time processing (RTOS)**, and **Data Resiliency (Offline Caching)**.

![esp](images/esp.png)

---

## 🌟 Key Engineering Features (Highlights)

- **Fault-Tolerant Offline Caching (SPIFFS):** Implemented a robust local storage mechanism. If Wi-Fi or MQTT disconnects, JSON payloads are safely cached in the ESP32's flash memory. Upon reconnection, a background task sequentially uploads the backlogged data to the Cloud.
- **Adaptive Logging Strategy:** Optimizes flash memory lifespan and storage capacity. The logging frequency dynamically shifts from `500ms` (Online Mode) to `2000ms` (Offline Mode). Critical threshold violations (Alarms) bypass this delay for immediate recording.
- **Decoupled RTOS Architecture:** Utilizes FreeRTOS to separate high-priority deterministic tasks (100ms sensor polling) from low-priority blocking tasks (Network I/O). Tasks communicate safely via FreeRTOS Queues.
- **Digital Signal Processing (DSP):** Implements a **Peak-Hold Algorithm** for the accelerometer to capture transient shock anomalies between MQTT transmission windows, alongside a **Moving Average Filter** for stable ADC current readings.
- **Commercial-Grade Wi-Fi Provisioning:** Eliminates hardcoded credentials. Uses ESP32 SoftAP Provisioning (`wifi_provisioning` component), allowing field technicians to configure Wi-Fi via a mobile app securely.
- **Hardware Watchdog & Auto-Recovery:** Gracefully handles I2C bus lockups or sensor disconnections without crashing the main system.

---

## 🛠️ Hardware Architecture

The edge node integrates the following components:
* **Microcontroller:** ESP32 DevKit V1
* **Vibration Sensor:** MPU6050 (I2C) - *Measures total acceleration vector (g).*
* **Thermal/Environment:** BMP280 (I2C) - *Industrial temperature monitoring.*
* **Current Sensor:** ACS712 (Analog/ADC) - *Monitors load consumption.*
* **Actuator:** Active Buzzer (GPIO) - *Triggers local alarms on threshold violations.*

---

## 🧩 Software Architecture & Task Management

The firmware is developed using the **Espressif IoT Development Framework (ESP-IDF)** in standard C. 

| Task Name | Priority | Cycle | Description |
| :--- | :---: | :---: | :--- |
| `sensor_task` | High (4) | 100ms | Polls MPU6050, BMP280, ACS712. Applies filtering and pushes structs to FreeRTOS Queue. |
| `logger_task` | Normal (3) | 500ms | Consumes queue, applies Peak-Hold, evaluates Alarms, and dispatches JSON to Comms. |
| `wifi_prov_task` | System | Event | Handles SoftAP server, DHCP, and PoP security for initial setup. |
| `offline_upload`| Low (2) | Event | Triggered purely on MQTT reconnect. Reads SPIFFS chunks and publishes backlogs. |

---

## 🚀 Getting Started

### Prerequisites
* ESP-IDF v5.x installed and configured.
* A ThingsBoard Cloud account (or local instance).

### Installation & Flashing
**1. Clone the repository:**
```bash
   git clone https://github.com/SweetieCloud/Smart-Industrial-Sensor-Node.git    
   cd smart_sensor_node
```

**2. Configure the ThingsBoard Access Token in `components/comms/comms.c`:**
```c
esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = "mqtt://thingsboard.cloud",
    .credentials.username = "YOUR_THINGSBOARD_ACCESS_TOKEN",
};

```


**3. Build, flash, and monitor:**
```bash
idf.py build flash monitor

```



### Provisioning the Device (First Boot)

1. Upon first boot, the ESP32 will host a Wi-Fi network named `PROV_SMART_NODE`.
2. Download the **ESP SoftAP Prov** app (iOS/Android).
3. Connect to the device using the Proof of Possession (PoP) PIN: `12345678`.
4. Provide your local 2.4GHz Wi-Fi credentials. The device will save them to NVS and reboot automatically.

---

## 📊 Cloud Dashboard (ThingsBoard)

Data is published to the default ThingsBoard telemetry topic: `v1/devices/me/telemetry` using the following JSON schema:

```json
{
  "current": 1.25,
  "vibration": 0.105,
  "temperature": 34.52
}

```

*(Optional: Add your screenshots here to make the repo visually appealing)*

> **Real-time Monitoring Dashboard:**
> 
> 
> 
> 
> 
> `<img src="link_anh_dashboard_cua_ban.png" width="800">`
> 
> 
> 
> 
> 
> **Industrial Analog Gauges & Alarms:**
> 
> 
> 
> 
> 
> `<img src="link_anh_dong_ho_cua_ban.png" width="400">`

---

## 📝 License

This project is open-source and available under the MIT License.



