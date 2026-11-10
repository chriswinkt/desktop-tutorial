#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "RTClib.h"
#include <ESP32Servo.h>

// =====================================================
// WIFI SETTINGS
// =====================================================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
// India Standard Time (UTC + 5:30)
const long GMT_OFFSET_SEC = 19800;
const int DAYLIGHT_OFFSET_SEC = 0;

// =====================================================
// PIN CONNECTIONS
// =====================================================
// OLED I2C
#define OLED_SDA 21
#define OLED_SCL 22

// Servos
#define SERVO1_PIN 18
#define SERVO2_PIN 19

// LED Indicators (Active-HIGH)
#define LED1_PIN 25  // Compartment 1 Indicator
#define LED2_PIN 27  // Compartment 2 Indicator

// Buzzer: Active-HIGH (Positive -> GPIO 26, Negative -> GND)
#define BUZZER_PIN 26
#define BUZZER_ON  HIGH
#define BUZZER_OFF LOW

// =====================================================
// OLED CONFIGURATION
// =====================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =====================================================
// RTC & SERVOS
// =====================================================
RTC_DS3231 rtc;

Servo servo1;
Servo servo2;

int SERVO_CLOSED = 0;
int SERVO_OPEN = 90;

// =====================================================
// ALARM SETTINGS
// =====================================================
int alarmHour[2] = {8, 13};
int alarmMinute[2] = {0, 0};
bool alarmEnabled[2] = {true, true};

// =====================================================
// RUNTIME ALARM STATE
// =====================================================
bool alarmActive = false;
unsigned long alarmStartTime = 0;
const unsigned long ALARM_DURATION = 10000; // 10-second timeout

int activeAlarmIndex = -1;

int lastAlarmDay = -1;
int lastAlarmMonth = -1;
int lastAlarmYear = -1;
int lastAlarmIndex = -1;

// =====================================================
// WEB SERVER
// =====================================================
WebServer server(80);
bool displayEnabled = true;

// =====================================================
// FUNCTION: SHOW OLED
// =====================================================
void showOLED(String line1, String line2 = "", String line3 = "", String line4 = "") {
  if (!displayEnabled) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(line1);

  display.setCursor(0, 16);
  display.println(line2);

  display.setCursor(0, 32);
  display.println(line3);

  display.setCursor(0, 48);
  display.println(line4);

  display.display();
}

// =====================================================
// FUNCTION: STOP ALARM
// =====================================================
void stopAlarm() {
  // Turn Buzzer OFF
  digitalWrite(BUZZER_PIN, BUZZER_OFF);

  // Turn both LEDs OFF
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  if (activeAlarmIndex == 0) {
    servo1.write(SERVO_CLOSED);
    Serial.println("Servo 1 CLOSED | LED 1 OFF");
  } else if (activeAlarmIndex == 1) {
    servo2.write(SERVO_CLOSED);
    Serial.println("Servo 2 CLOSED | LED 2 OFF");
  }

  alarmActive = false;

  if (activeAlarmIndex >= 0) {
    DateTime now = rtc.now();
    lastAlarmDay = now.day();
    lastAlarmMonth = now.month();
    lastAlarmYear = now.year();
    lastAlarmIndex = activeAlarmIndex;
  }

  activeAlarmIndex = -1;

  showOLED("SMART MEDICINE BOX", "Alarm Stopped", "Compartment Closed", "System Ready");
}

// =====================================================
// FUNCTION: START ALARM
// =====================================================
void startAlarm(int alarmIndex) {
  alarmActive = true;
  activeAlarmIndex = alarmIndex;
  alarmStartTime = millis();

  // Buzzer ON
  digitalWrite(BUZZER_PIN, BUZZER_ON);

  if (alarmIndex == 0) {
    // Compartment 1 Action
    digitalWrite(LED1_PIN, HIGH);
    digitalWrite(LED2_PIN, LOW);

    servo1.write(SERVO_OPEN);
    servo2.write(SERVO_CLOSED);

    showOLED("MEDICINE ALERT!", "Compartment 1", "LED 1 ON / Servo Open", "Press ALARM OFF");
    Serial.println("ALERT: Compartment 1 -> LED 1 ON, Servo 1 OPEN");
  } else if (alarmIndex == 1) {
    // Compartment 2 Action
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, HIGH);

    servo1.write(SERVO_CLOSED);
    servo2.write(SERVO_OPEN);

    showOLED("MEDICINE ALERT!", "Compartment 2", "LED 2 ON / Servo Open", "Press ALARM OFF");
    Serial.println("ALERT: Compartment 2 -> LED 2 ON, Servo 2 OPEN");
  }
}

// =====================================================
// FUNCTION: CHECK ALARMS
// =====================================================
void checkAlarms() {
  DateTime now = rtc.now();
  int currentHour = now.hour();
  int currentMinute = now.minute();

  for (int i = 0; i < 2; i++) {
    if (!alarmEnabled[i]) continue;

    if (currentHour == alarmHour[i] && currentMinute == alarmMinute[i]) {
      bool alreadyTriggered = (
        lastAlarmDay == now.day() &&
        lastAlarmMonth == now.month() &&
        lastAlarmYear == now.year() &&
        lastAlarmIndex == i
      );

      if (!alreadyTriggered && !alarmActive) {
        startAlarm(i);
        lastAlarmDay = now.day();
        lastAlarmMonth = now.month();
        lastAlarmYear = now.year();
        lastAlarmIndex = i;
      }
    }
  }
}

// =====================================================
// FUNCTION: UPDATE ALARM TIMER
// =====================================================
void updateAlarm() {
  if (!alarmActive) return;

  if (millis() - alarmStartTime >= ALARM_DURATION) {
    Serial.println("Alarm timeout reached (10 seconds).");
    stopAlarm();
  }
}

// =====================================================
// FUNCTION: UPDATE NORMAL OLED
// =====================================================
void updateNormalOLED() {
  if (!displayEnabled || alarmActive) return;

  static unsigned long lastOLEDUpdate = 0;
  if (millis() - lastOLEDUpdate < 1000) return;
  lastOLEDUpdate = millis();

  DateTime now = rtc.now();

  char timeBuffer[10];
  sprintf(timeBuffer, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  char dateBuffer[12];
  sprintf(dateBuffer, "%02d/%02d/%04d", now.day(), now.month(), now.year());

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART MEDICINE BOX");

  display.setTextSize(2);
  display.setCursor(16, 18);
  display.println(timeBuffer);

  display.setTextSize(1);
  display.setCursor(30, 40);
  display.println(dateBuffer);

  display.setCursor(0, 55);
  display.println("System Ready");

  display.display();
}

// =====================================================
// FUNCTION: CREATE WEB DASHBOARD
// =====================================================
String createWebPage() {
  DateTime now = rtc.now();

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Smart Medicine Box Dashboard</title>
<style>
:root {
  --bg: #0f172a;
  --card: #1e293b;
  --card-border: #334155;
  --text-main: #f8fafc;
  --text-sub: #94a3b8;
  --primary: #38bdf8;
  --accent-green: #22c55e;
  --accent-red: #ef4444;
}
* { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; }
body { background-color: var(--bg); color: var(--text-main); padding: 24px 16px; display: flex; justify-content: center; }
.container { width: 100%; max-width: 500px; display: flex; flex-direction: column; gap: 16px; }
.card { background: var(--card); border: 1px solid var(--card-border); border-radius: 16px; padding: 18px; box-shadow: 0 4px 16px rgba(0,0,0,0.3); }
.header { text-align: center; }
.header h1 { font-size: 1.45rem; font-weight: 700; margin-bottom: 4px; }
.header p { color: var(--text-sub); font-size: 0.85rem; }
.clock-display { text-align: center; background: rgba(15,23,42,0.6); border-radius: 12px; padding: 16px; border: 1px solid rgba(255,255,255,0.05); }
.clock-time { font-size: 2.4rem; font-weight: 800; letter-spacing: 2px; color: var(--primary); }
.clock-date { font-size: 0.85rem; color: var(--text-sub); margin-top: 4px; }
.status-banner { display: flex; align-items: center; justify-content: space-between; padding: 14px 16px; border-radius: 12px; font-weight: 600; }
.status-normal { background: rgba(34,197,94,0.15); border: 1px solid var(--accent-green); color: var(--accent-green); }
.status-active { background: rgba(239,68,68,0.2); border: 1px solid var(--accent-red); color: #fca5a5; animation: pulse 1.5s infinite; }
@keyframes pulse { 0% { opacity: 0.8; } 50% { opacity: 1; transform: scale(1.01); } 100% { opacity: 0.8; } }
.alarm-row { display: flex; align-items: center; justify-content: space-between; padding: 12px 0; border-bottom: 1px solid var(--card-border); }
.alarm-row:last-child { border-bottom: none; }
.alarm-title { font-weight: 600; font-size: 0.95rem; }
input[type="time"] { background: #0f172a; border: 1px solid var(--card-border); color: var(--text-main); padding: 6px 10px; border-radius: 8px; font-size: 0.95rem; outline: none; }
.switch { position: relative; display: inline-block; width: 44px; height: 24px; margin-left: 10px; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #334155; transition: .2s; border-radius: 24px; }
.slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 3px; bottom: 3px; background-color: white; transition: .2s; border-radius: 50%; }
input:checked + .slider { background-color: var(--primary); }
input:checked + .slider:before { transform: translateX(20px); }
.btn { width: 100%; padding: 12px; border: none; border-radius: 10px; font-size: 0.95rem; font-weight: 600; cursor: pointer; display: inline-flex; align-items: center; justify-content: center; gap: 8px; transition: opacity 0.2s, transform 0.1s; }
.btn:active { transform: scale(0.98); }
.btn-save { background: var(--primary); color: #0f172a; margin-top: 14px; }
.btn-danger { background: var(--accent-red); color: white; }
.btn-outline { background: #334155; color: var(--text-main); }
.btn-outline:hover { background: #475569; }
.grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 10px; }
.section-title { font-size: 0.8rem; text-transform: uppercase; letter-spacing: 1px; color: var(--text-sub); margin-bottom: 8px; }
</style>
</head>
<body>
<div class="container">
<div class="header">
<h1>💊 MediBox Manager</h1>
<p>ESP32 Automated Dispenser Dashboard</p>
</div>
<div class="card clock-display">
<div id="liveClock" class="clock-time">--:--:--</div>
<div id="liveDate" class="clock-date">Fetching RTC...</div>
</div>
)rawliteral";

  if (alarmActive) {
    html += "<div class='card status-banner status-active'>";
    html += "<div>🚨 MEDICINE TIME: Compartment " + String(activeAlarmIndex + 1) + " (LED " + String(activeAlarmIndex + 1) + " ACTIVE)</div>";
    html += "<button class='btn btn-danger' style='width:auto; padding:6px 14px;' onclick=\"fetch('/alarm/off').then(()=>location.reload())\">ALARM OFF</button>";
    html += "</div>";
  } else {
    html += "<div class='status-banner status-normal'>";
    html += "<span>● System Ready</span>";
    html += "<span style='font-size:0.8rem; font-weight:normal;'>Awaiting Schedule</span>";
    html += "</div>";
  }

  html += R"rawliteral(
<div class="card">
<div class="section-title">Schedule Alarms</div>
<form action="/save" method="GET">
)rawliteral";

  for (int i = 0; i < 2; i++) {
    char timeStr[6];
    sprintf(timeStr, "%02d:%02d", alarmHour[i], alarmMinute[i]);

    html += "<div class='alarm-row'>";
    html += "<div class='alarm-title'>Compartment " + String(i + 1) + " (LED " + String(i + 1) + ")</div>";
    html += "<div style='display:flex; align-items:center;'>";
    html += "<input type='time' name='a" + String(i + 1) + "' value='" + String(timeStr) + "'>";
    html += "<label class='switch'>";
    html += "<input type='checkbox' name='e" + String(i + 1) + "' " + (alarmEnabled[i] ? "checked" : "") + ">";
    html += "<span class='slider'></span>";
    html += "</label>";
    html += "</div></div>";
  }

  html += R"rawliteral(
<button class="btn btn-save" type="submit">💾 Save Schedules</button>
</form>
</div>

<div class="card">
<div class="section-title">Hardware Diagnostics</div>
<div class="grid-2">
<button class="btn btn-outline" onclick="runAction('/servo/test', this)">🔧 Cycle Servos</button>
<button class="btn btn-outline" onclick="runAction('/buzzer/test', this)">🔊 Beep Buzzer</button>
</div>
<div class="grid-2">
<button class="btn btn-outline" onclick="runAction('/led/test', this)">💡 Test LEDs</button>
<button class="btn btn-outline" onclick="runAction('/display/on', this)">☀️ OLED On</button>
</div>
</div>
</div>

<script>
)rawliteral";

  html += "let currentTime = new Date(" +
          String(now.year()) + "," +
          String(now.month() - 1) + "," +
          String(now.day()) + "," +
          String(now.hour()) + "," +
          String(now.minute()) + "," +
          String(now.second()) + ");\n";

  html += R"rawliteral(
function updateClock() {
  currentTime.setSeconds(currentTime.getSeconds() + 1);
  const h = String(currentTime.getHours()).padStart(2, '0');
  const m = String(currentTime.getMinutes()).padStart(2, '0');
  const s = String(currentTime.getSeconds()).padStart(2, '0');
  document.getElementById('liveClock').textContent = `${h}:${m}:${s}`;
  const options = { weekday: 'short', month: 'short', day: 'numeric', year: 'numeric' };
  document.getElementById('liveDate').textContent = currentTime.toLocaleDateString(undefined, options);
}
setInterval(updateClock, 1000);
updateClock();

function runAction(endpoint, btn) {
  const original = btn.innerText;
  btn.innerText = "Running...";
  btn.disabled = true;
  fetch(endpoint)
    .then(() => {
      setTimeout(() => { btn.innerText = original; btn.disabled = false; }, 1800);
    })
    .catch(() => {
      btn.innerText = "Error";
      btn.disabled = false;
    });
}
</script>
</body>
</html>
)rawliteral";

  return html;
}

// =====================================================
// WEB ROUTE HANDLERS
// =====================================================
void handleRoot() {
  server.send(200, "text/html", createWebPage());
}

void handleSave() {
  for (int i = 0; i < 2; i++) {
    String timeParam = "a" + String(i + 1);
    if (server.hasArg(timeParam)) {
      String timeValue = server.arg(timeParam);
      int sep = timeValue.indexOf(':');
      if (sep > 0) {
        alarmHour[i] = timeValue.substring(0, sep).toInt();
        alarmMinute[i] = timeValue.substring(sep + 1).toInt();
      }
    }
    String enableParam = "e" + String(i + 1);
    alarmEnabled[i] = server.hasArg(enableParam);
  }

  lastAlarmDay = -1;
  lastAlarmMonth = -1;
  lastAlarmYear = -1;
  lastAlarmIndex = -1;

  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "Saved");
}

void handleAlarmOff() {
  if (alarmActive) {
    stopAlarm();
  }
  server.send(200, "text/plain", "OK");
}

void handleServoTest() {
  server.send(200, "text/plain", "OK");
  servo1.write(SERVO_OPEN);
  servo2.write(SERVO_OPEN);
  showOLED("SERVO TEST", "Opening both...");
  delay(1500);
  servo1.write(SERVO_CLOSED);
  servo2.write(SERVO_CLOSED);
  showOLED("SERVO TEST", "Both Closed", "Ready");
}

void handleBuzzerTest() {
  server.send(200, "text/plain", "OK");
  digitalWrite(BUZZER_PIN, BUZZER_ON);
  showOLED("DIAGNOSTIC TEST", "Buzzer ON");
  delay(1000);
  digitalWrite(BUZZER_PIN, BUZZER_OFF);
  showOLED("DIAGNOSTIC TEST", "Buzzer OFF", "Ready");
}

void handleLedTest() {
  server.send(200, "text/plain", "OK");
  digitalWrite(LED1_PIN, HIGH);
  digitalWrite(LED2_PIN, HIGH);
  showOLED("LED TEST", "Both LEDs ON");
  delay(1500);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  showOLED("LED TEST", "Both LEDs OFF", "Ready");
}

void handleDisplayOn() {
  displayEnabled = true;
  showOLED("SMART MEDICINE BOX", "OLED Enabled", "System Ready");
  server.send(200, "text/plain", "OK");
}

void handleDisplayOff() {
  displayEnabled = false;
  display.clearDisplay();
  display.display();
  server.send(200, "text/plain", "OK");
}

// =====================================================
// WIFI & NTP TIME SYNC
// =====================================================
void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected! IP: ");
    Serial.println(WiFi.localIP());

    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");
    delay(2000);

    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
      DateTime ntpTime(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
      );
      rtc.adjust(ntpTime);
      Serial.println("RTC synced with NTP.");
    }
  } else {
    Serial.println("WiFi not reachable. Running on RTC time.");
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // Initialize LEDs
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  // Initialize Buzzer (GPIO 26 -> POSITIVE, Active-HIGH)
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, BUZZER_OFF);

  // Initialize I2C Bus & OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    displayEnabled = true;
    showOLED("SMART MEDICINE BOX", "Starting...", "Please Wait");
  } else {
    displayEnabled = false;
    Serial.println("OLED NOT FOUND!");
  }

  // Initialize DS3231 RTC
  if (!rtc.begin()) {
    Serial.println("RTC NOT FOUND!");
    if (displayEnabled) showOLED("ERROR!", "RTC NOT FOUND");
  } else if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Allocate PWM Timers for Servos
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);

  servo1.attach(SERVO1_PIN, 500, 2400);
  servo2.attach(SERVO2_PIN, 500, 2400);
  servo1.write(SERVO_CLOSED);
  servo2.write(SERVO_CLOSED);

  // Connect WiFi and Sync RTC
  connectWiFi();

  // Register Web Endpoints
  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/alarm/off", handleAlarmOff);
  server.on("/servo/test", handleServoTest);
  server.on("/buzzer/test", handleBuzzerTest);
  server.on("/led/test", handleLedTest);
  server.on("/display/on", handleDisplayOn);
  server.on("/display/off", handleDisplayOff);

  server.begin();
  Serial.println("Web server started.");

  showOLED("SMART MEDICINE BOX", "System Ready", "2 Compartments", "Servos Closed");
}

// =====================================================
// MAIN LOOP
// =====================================================
void loop() {
  server.handleClient();
  checkAlarms();
  updateAlarm();
  updateNormalOLED();
  delay(10);
}
