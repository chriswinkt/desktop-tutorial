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
