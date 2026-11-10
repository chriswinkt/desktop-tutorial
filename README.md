# Smart Medicine Box (Dual Compartment)

An IoT-enabled, automated medicine reminder and dispensing system powered by an **ESP32** microcontroller. It automates medication management for two separate compartments using scheduled servo actuation, active buzzer alerts, dedicated LED indicators, an OLED status display, and an embedded local web dashboard for remote scheduling and diagnostics.

---

## 🌟 Key Features

- **Dual-Compartment Dispensing:** Individual servo-driven compartment control (`Compartment 1` and `Compartment 2`).
- **RTC & NTP Clock Synchronization:** DS3231 RTC maintains accurate timing across power cycles, auto-synced with NTP via Wi-Fi (IST / UTC +5:30).
- **Embedded Web Dashboard:** Responsive local web server running on port `80` to configure alarm schedules, toggle alarms, and perform hardware diagnostics.
- **Visual & Audio Reminders:** 
  - 0.96" SSD1306 OLED display for real-time status and dosing alerts.
  - Active buzzer alert with an automatic 10-second timeout fail-safe.
  - Dedicated per-compartment LED indicators.
- **Hardware Diagnostics:** Built-in web endpoints to cycle servos, pulse the buzzer, flash LEDs, and toggle the OLED display.

---

## 📌 Hardware Pinout & Wiring

| Component | Pin / Interface | ESP32 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **OLED Display (SSD1306)** | SDA | `GPIO 21` | I2C Data (Address `0x3C`) |
| **OLED Display (SSD1306)** | SCL | `GPIO 22` | I2C Clock |
| **Servo Motor 1** | PWM Signal | `GPIO 18` | Controls Compartment 1 |
| **Servo Motor 2** | PWM Signal | `GPIO 19` | Controls Compartment 2 |
| **LED Indicator 1** | Anode (+) | `GPIO 25` | Active-HIGH (Compartment 1) |
| **LED Indicator 2** | Anode (+) | `GPIO 27` | Active-HIGH (Compartment 2) |
| **Active Buzzer** | Positive (+) | `GPIO 26` | Active-HIGH |
| **DS3231 RTC Module** | SDA / SCL | `GPIO 21` / `GPIO 22` | Shared I2C Bus |

---

## 🗂️ Project Structure

```text
smart-medicine-box/
├── Smart_Medicine_Box_2_Separate_Compartment/
│   └── Smart_Medicine_Box_2_Separate_Compartment.ino
└── README.md
```
## ⚙️ Installation & Setup

### 1. Arduino IDE Prerequisites

- Install the **ESP32 Board Package** via Arduino Boards Manager (`esp32` by Espressif Systems).
- Install the following libraries via the Arduino Library Manager (**Sketch > Include Library > Manage Libraries...**):
  - `Adafruit GFX Library`
  - `Adafruit SSD1306`
  - `RTClib` (by Adafruit)
  - `ESP32Servo` (by Kevin Harrington)

### 2. Configuration

Before uploading, configure your local network credentials in the sketch:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```
### 3. Uploading Code

1. Connect the ESP32 board to your PC via USB.
2. Select your board under **Tools > Board > ESP32 Arduino** (e.g., *ESP32 Dev Module*).
3. Select the active COM port under **Tools > Port**.
4. Click **Upload**.

---

## 🌐 Web Dashboard & Endpoints

Once connected, open the Serial Monitor at **115200 baud** to view the assigned local IP address (e.g., `http://192.168.1.50`).

### Dashboard Interface

Navigate to the board's IP address in any browser on the same network to access:

- **Live Clock & Status:** View real-time RTC clock and dispensing status.
- **Alarm Scheduling:** Set hours, minutes, and toggle enable/disable for Compartments 1 and 2.
- **Hardware Diagnostics:**
  - `Cycle Servos` -> Triggers `/servo/test`
  - `Beep Buzzer` -> Triggers `/buzzer/test`
  - `Test LEDs` -> Triggers `/led/test`
  - `OLED On / Off` -> Triggers `/display/on` or `/display/off`
- **Dismiss Alert:** Silences buzzer, turns off indicator LEDs, and closes the active compartment servo.
