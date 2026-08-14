#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <math.h>
#include <arduinoFFT.h>

// ==========================================
// 1. HARDWARE & PIN SETTINGS
// ==========================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// AD8232 ECG Pins
#define SENSOR_PIN 34
#define LO_MINUS_PIN 4
#define LO_PLUS_PIN 5

// ALARM BUZZER
#define BUZZER_PIN 26

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
MAX30105 particleSensor;

// ==========================================
// 2. WI-FI & CLOUD SETTINGS
// ==========================================
const char* ap_ssid = "ESP32-Test";
const char* ap_password = "12345678";

// Optional: Your home router credentials
const char* router_ssid = "YOUR_ROUTER_SSID";
const char* router_password = "YOUR_ROUTER_PASSWORD";

WebServer server(80);
const char* cloudServer = "http://api.thingspeak.com/update";
String apiKey = "YOUR_THINGSPEAK_WRITE_API_KEY"; 

bool isCloudEnabled = false; 

// ==========================================
// 3. INTEGRATED VITALS VARIABLES (Volatile)
// ==========================================
const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;

volatile int ppgHeartRate = 0;   
volatile int ecgHeartRate = 0;   
volatile float spo2 = 0;        
volatile float currentAHI = 0.0;
volatile int seizureIndex = 0;   
volatile bool isApneaActive = false;
volatile bool isSeizureActive = false;

// --- ECG Filter 1: 50Hz moving average notch ---
#define FILTER_SIZE 10
int readings[FILTER_SIZE];
int readIndex = 0;
long ecgTotal = 0;
int ecgAverage = 0;

// --- ECG Filter 2: EMA ---
float emaAlpha = 0.3;
volatile float emaSmoothed = 0;
int peakThreshold = 2200;

unsigned long lastEcgBeatTime = 0;
int bpmArray[5] = {0}; 
int bpmIndex = 0;

// IMU & FFT Apnea/Movement Variables
const int MPU_ADDR = 0x68;
#define SAMPLES 64             
#define SAMPLING_FREQ 10.0     
unsigned int sampling_period_us;
unsigned long lastSampleTime;

double vReal[SAMPLES];
double vImag[SAMPLES];
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQ);

int sampleIndex = 0;
unsigned long lastValidBreathTime = 0;
int totalApneaEvents = 0;
unsigned long sessionStartTime = 0;

const double FFT_MAGNITUDE_THRESHOLD = 5.0; 
const int APNEA_TIMEOUT_MS = 10000;         
float display_ax, display_ay, display_az;

unsigned long prevEcgMillis = 0;
const long ecgInterval = 2; 

unsigned long prevPpgMillis = 0;
const long ppgInterval = 40; 

unsigned long lastOLEDUpdate = 0;

// ==========================================
// 4. WEBPAGE HTML (PROGMEM)
// ==========================================
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Biomedical Dashboard</title>
<style>
  *{margin:0;padding:0;box-sizing:border-box;font-family:'Segoe UI',sans-serif;}
  body{background:linear-gradient(135deg,#4f46e5,#06b6d4);min-height:100vh;padding:20px;}
  .header{background:rgba(255,255,255,0.15);backdrop-filter:blur(10px);color:white;padding:20px;border-radius:20px;text-align:center;margin-bottom:20px;box-shadow:0 4px 15px rgba(0,0,0,0.2);}
  .grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;max-width:1000px;margin:auto;}
  .card{border-radius:20px;padding:25px;color:white;box-shadow:0 6px 15px rgba(0,0,0,0.25);transition:0.3s;}
  .card:hover{transform:translateY(-5px);}
  .hr{background:linear-gradient(135deg,#ff6b6b,#ff8787);}
  .spo2{background:linear-gradient(135deg,#38bdf8,#0ea5e9);}
  .apnea{background:linear-gradient(135deg,#f59e0b,#fbbf24);}
  .seizure{background:linear-gradient(135deg,#8b5cf6,#a78bfa);}
  .status{background:linear-gradient(135deg,#10b981,#34d399);}
  .storage{background:linear-gradient(135deg,#64748b,#94a3b8);}
  .title{font-size:18px;}
  .value{font-size:40px;font-weight:bold;margin-top:10px;}
  button{padding:10px 15px;border:none;border-radius:10px;font-weight:bold;cursor:pointer;color:#333;background:white;transition:0.2s;}
  button:hover{opacity:0.9;}
  .btn-danger{background:#ef4444;color:white;}
  .btn-group{display:flex;gap:10px;margin-top:15px;flex-wrap:wrap;}
  @media(max-width:700px){.grid{grid-template-columns:1fr;}}
</style>
</head>
<body>
<div class="header">
  <h1>Biomedical Monitoring System</h1>
  <p>Integrated Sleep Apnea & Seizure Diagnostics</p>
</div>
<div class="grid">
  <div class="card hr"><div class="title">❤️ Heart Rate (ECG / PPG)</div><div class="value" id="hr">-- / -- BPM</div></div>
  <div class="card spo2"><div class="title">🩸 Blood Oxygen</div><div class="value" id="spo2">-- %</div></div>
  <div class="card apnea"><div class="title">😴 Apnea Index (AHI)</div><div class="value" id="ai">--</div></div>
  <div class="card seizure"><div class="title">⚡ Seizure Metric</div><div class="value" id="si">--</div></div>
  <div class="card status"><div class="title">🟢 Current Status</div><div class="value" id="status">NORMAL</div></div>
  
  <div class="card storage">
    <h3>💾 Browser Storage</h3>
    <p style="margin-top:10px;">Saved Records: <span id="recordCount" style="font-weight:bold;">0</span></p>
    <div class="btn-group">
      <button onclick="downloadCSV()">⬇ Download CSV</button>
      <button onclick="clearData()">🗑️ Clear Browser History</button>
      <button onclick="resetDevice()" class="btn-danger">🔄 Reset Live Sensors</button>
    </div>
  </div>
</div>

<script>
let sessionHistory = JSON.parse(localStorage.getItem('vitalsHistory')) || [];
document.getElementById('recordCount').innerText = sessionHistory.length;

function updateData() {
  fetch('/data')
  .then(response => response.json())
  .then(data => {
    document.getElementById('hr').innerHTML = data.ecg_hr + " / " + data.ppg_hr + " BPM";
    document.getElementById('spo2').innerHTML = data.spo2 + " %";
    document.getElementById('ai').innerHTML = data.ai;
    document.getElementById('si').innerHTML = data.si;
    
    let statusBox = document.getElementById('status');
    let cardBox = document.querySelector('.status');
    
    if (data.apnea_alarm == 1) {
       statusBox.innerHTML = "APNEA ALARM!";
       cardBox.style.background = "linear-gradient(135deg, #ef4444, #dc2626)";
    } else if (data.seizure_alarm == 1) {
       statusBox.innerHTML = "SEIZURE WARNING!";
       cardBox.style.background = "linear-gradient(135deg, #7c3aed, #6d28d9)";
    } else {
       statusBox.innerHTML = "SYSTEM NORMAL";
       cardBox.style.background = "linear-gradient(135deg, #10b981, #34d399)";
    }

    if(data.ppg_hr > 0 || data.ecg_hr > 0 || data.spo2 > 0) {
        let now = new Date();
        let currentDate = now.toLocaleDateString(); 
        let currentTime = now.toLocaleTimeString(); 

        let record = { date: currentDate, time: currentTime, ecg_hr: data.ecg_hr, ppg_hr: data.ppg_hr, spo2: data.spo2, ai: data.ai, seizure: data.si };
        sessionHistory.push(record);
        if(sessionHistory.length > 5000) sessionHistory.shift(); 
        localStorage.setItem('vitalsHistory', JSON.stringify(sessionHistory));
        document.getElementById('recordCount').innerText = sessionHistory.length;
    }
  });
}

function downloadCSV() {
    if(sessionHistory.length === 0) { 
        alert("No records to download! Make sure sensors are reading > 0."); 
        return; 
    }
    
    let csvContent = "Date,Time,ECG_HR,PPG_HR,SpO2,AHI,JerkValue\n";
    
    sessionHistory.forEach(row => { 
        let safeDate = row.date.replace(/,/g, ' ');
        let safeTime = row.time.replace(/,/g, ' ');
        csvContent += `${safeDate},${safeTime},${row.ecg_hr},${row.ppg_hr},${row.spo2},${row.ai},${row.seizure}\n`; 
    });
    
    let blob = new Blob([csvContent], { type: 'text/csv;charset=utf-8;' });
    let url = URL.createObjectURL(blob);
    
    let link = document.createElement("a");
    link.setAttribute("href", url);
    
    let todayFileDate = new Date().toLocaleDateString().replace(/\//g, '-');
    link.setAttribute("download", `Biomedical_Session_${todayFileDate}.csv`);
    
    document.body.appendChild(link);
    link.click();
    
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
}

function clearData() {
    if(confirm("Are you sure you want to permanently delete all saved records?")) {
        sessionHistory = []; 
        localStorage.removeItem('vitalsHistory'); 
        document.getElementById('recordCount').innerText = "0"; 
    }
}

function resetDevice() {
    if(confirm("Are you sure you want to reset the ESP32 live metrics (AHI, Alarms, and Timers)?")) {
        fetch('/reset')
        .then(response => {
            if(response.ok) {
                document.getElementById('status').innerHTML = "SYSTEM NORMAL";
                document.querySelector('.status').style.background = "linear-gradient(135deg, #10b981, #34d399)";
                document.getElementById('ai').innerHTML = "0.0";
                document.getElementById('si').innerHTML = "0";
                alert("Device metrics have been reset successfully!");
            }
        })
        .catch(error => alert("Failed to reset device. Is it still connected?"));
    }
}

setInterval(updateData, 1000);
</script>
</body>
</html>
)rawliteral";

// ==========================================
// 5. WEB SERVER ENDPOINTS 
// ==========================================
void handleRoot() {
  server.sendHeader("Connection", "close"); 
  server.send(200, "text/html", index_html);
}

void sendData() {
  server.sendHeader("Connection", "close");
  String json = "{";
  json += "\"ecg_hr\":" + String(ecgHeartRate) + ",";
  json += "\"ppg_hr\":" + String(ppgHeartRate) + ",";
  json += "\"spo2\":" + String((int)spo2) + ",";
  json += "\"ai\":" + String(currentAHI, 1) + ",";
  json += "\"si\":" + String(seizureIndex) + ","; 
  json += "\"apnea_alarm\":" + String(isApneaActive ? 1 : 0) + ",";
  json += "\"seizure_alarm\":" + String(isSeizureActive ? 1 : 0);
  json += "}";
  server.send(200, "application/json", json);
}

void handleReset() {
  server.sendHeader("Connection", "close");
  
  // Reset all session metrics
  totalApneaEvents = 0;
  currentAHI = 0.0;
  seizureIndex = 0;
  isApneaActive = false;
  isSeizureActive = false;
  
  // Reset timers to prevent immediate false alarms
  sessionStartTime = millis(); 
  lastValidBreathTime = millis();
  
  server.send(200, "text/plain", "Metrics Reset OK");
}

// ==========================================
// 6. CORE 0 WORKER TASKS
// ==========================================
void webServerTask(void *pvParameters) {
  for (;;) {
    server.handleClient(); 
    vTaskDelay(5 / portTICK_PERIOD_MS); 
  }
}

void cloudTask(void *pvParameters) {
  for (;;) {
    if (isCloudEnabled && WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      String serverPath = String(cloudServer) + "?api_key=" + apiKey + 
                          "&field1=" + String(ecgHeartRate) + 
                          "&field2=" + String(spo2) + 
                          "&field3=" + String(currentAHI, 1) + 
                          "&field4=" + String(seizureIndex);
      
      http.begin(serverPath.c_str());
      http.GET(); 
      http.end();
    }
    vTaskDelay(20000 / portTICK_PERIOD_MS); 
  }
}

// ==========================================
// 7. SETUP 
// ==========================================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); 

  pinMode(SENSOR_PIN, INPUT);
  pinMode(LO_MINUS_PIN, INPUT);
  pinMode(LO_PLUS_PIN, INPUT);

  for (int i = 0; i < FILTER_SIZE; i++) readings[i] = 0;

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { Serial.println("OLED Fail"); }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("BOOTING SYSTEM...");
  display.display();

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("Part ID mismatch. Forcing setup anyway...");
    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("MAX BYPASS");
    display.display();
    delay(1000); 
  }
  
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A); 
  particleSensor.setPulseAmplitudeIR(0x0A); 

  Wire.beginTransmission(MPU_ADDR);
  if (Wire.endTransmission() == 0) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); Wire.write(0);
    Wire.endTransmission(true);
  }

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);
  delay(300); 

  Serial.println("Connecting to Router...");
  WiFi.begin(router_ssid, router_password);
  
  int connectionAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && connectionAttempts < 20) {
    delay(500);
    Serial.print(".");
    connectionAttempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    isCloudEnabled = true;
    Serial.println("\nCloud Connected successfully!");
  } else {
    WiFi.disconnect(true); 
    WiFi.mode(WIFI_AP); 
    Serial.println("\nRouter not found. Hotspot Locked into pure AP mode.");
  }
  
  server.on("/", handleRoot);
  server.on("/data", sendData);
  server.on("/reset", handleReset);
  server.begin();

  display.clearDisplay();
  display.println("WIFI HOTSPOT READY");
  display.print("SSID: "); display.println(ap_ssid);
  display.print("IP:   "); display.println(WiFi.softAPIP());
  display.display();
  delay(3000); 

  xTaskCreatePinnedToCore(webServerTask, "WebServerTask", 4096, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(cloudTask, "CloudTask", 4096, NULL, 1, NULL, 0);

  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));
  lastSampleTime = micros();
  lastValidBreathTime = millis();
  sessionStartTime = millis(); 
}

// ==========================================
// 8. CORE 1 LOOP (LIVE SENSORS)
// ==========================================
void loop() {
  unsigned long currentMillis = millis();

  // --- WINDOW 1: LIVE ECG PIPELINE (500Hz) ---
  if (currentMillis - prevEcgMillis >= ecgInterval) {
    prevEcgMillis = currentMillis;

    if (digitalRead(LO_MINUS_PIN) == 1 || digitalRead(LO_PLUS_PIN) == 1) {
      emaSmoothed = 0;
      // ecgHeartRate = 0; // Commented out so mock data isn't wiped
    } else {
      int rawValue = analogRead(SENSOR_PIN);

      ecgTotal = ecgTotal - readings[readIndex];      
      readings[readIndex] = rawValue;            
      ecgTotal = ecgTotal + readings[readIndex];      
      readIndex = (readIndex + 1) % FILTER_SIZE;
      ecgAverage = ecgTotal / FILTER_SIZE;            

      if (emaSmoothed == 0) emaSmoothed = ecgAverage; 
      emaSmoothed = (emaAlpha * ecgAverage) + ((1.0 - emaAlpha) * emaSmoothed);

      if (emaSmoothed > peakThreshold && (currentMillis - lastEcgBeatTime) > 300) {
        unsigned long rrInterval = currentMillis - lastEcgBeatTime;
        lastEcgBeatTime = currentMillis; 

        int instantBPM = 60000 / rrInterval;
        if (instantBPM > 40 && instantBPM < 160) {
          bpmArray[bpmIndex] = instantBPM;
          bpmIndex = (bpmIndex + 1) % 5;
          int bpmTotal = 0;
          for (int i = 0; i < 5; i++) bpmTotal += bpmArray[i];
          // ecgHeartRate = bpmTotal / 5; // Commented out to allow mock data
        }
      }
    }
  }

  // --- WINDOW 2: LIVE OPTICAL OXIMETER (40ms) ---
  if (currentMillis - prevPpgMillis >= ppgInterval) {
    prevPpgMillis = currentMillis;
    
    long irValue = particleSensor.getIR();
    long redValue = particleSensor.getRed();

    if (irValue > 7000) {
      if (checkForBeat(irValue)) {
        long delta = millis() - lastBeat;
        lastBeat = millis();
        beatsPerMinute = 60.0 / (delta / 1000.0);
        if (beatsPerMinute > 40 && beatsPerMinute < 180) {
          rates[rateSpot++] = (byte)beatsPerMinute;
          rateSpot %= RATE_SIZE;
          int tempAvg = 0;
          for (byte i = 0; i < RATE_SIZE; i++) tempAvg += rates[i];
          // ppgHeartRate = tempAvg / RATE_SIZE; // Commented out to allow mock data
        }
      }
      
      if (redValue > 0 && irValue > 0) {
        float ratio = (float)redValue / (float)irValue;
        float tempSpO2 = 110.0 - (25.0 * ratio);
        if (tempSpO2 > 100) tempSpO2 = 100;
        if (tempSpO2 < 80) tempSpO2 = 80;
        spo2 = tempSpO2; // <-- UNCOMMENTED: Now pulls real data from the sensor
      }
    } else {
      // ppgHeartRate = 0; // Commented out so mock data isn't wiped
      spo2 = 0; 
    }
  }

  // --- WINDOW 3: MOTION DETECTOR / FFT ACCELEROMETER (LIVE) ---
  if (micros() - lastSampleTime >= sampling_period_us) {
    lastSampleTime += sampling_period_us;
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);  
    Wire.endTransmission(false);
    
    if(Wire.requestFrom(MPU_ADDR, 6, true) == 6) {
      int16_t AcX = Wire.read()<<8 | Wire.read();    
      int16_t AcY = Wire.read()<<8 | Wire.read();  
      int16_t AcZ = Wire.read()<<8 | Wire.read();  

      static float prevAx = 0, prevAy = 0, prevAz = 0;
      display_ax = AcX / 16384.0; display_ay = AcY / 16384.0; display_az = AcZ / 16384.0;

      float jerkX = abs(display_ax - prevAx);
      float jerkY = abs(display_ay - prevAy);
      float jerkZ = abs(display_az - prevAz);
      seizureIndex = (int)((jerkX + jerkY + jerkZ) * 100);
      
      isSeizureActive = (seizureIndex > 80);
      prevAx = display_ax; prevAy = display_ay; prevAz = display_az;

      if (seizureIndex > 15) { 
        lastValidBreathTime = millis();
        isApneaActive = false;
      }

      vReal[sampleIndex] = atan2(display_ay, sqrt(display_ax * display_ax + display_az * display_az)) * 180.0 / PI;
      vImag[sampleIndex] = 0.0;
      sampleIndex++;

      if (sampleIndex == SAMPLES) {
        double mean = 0;
        for(int i = 0; i < SAMPLES; i++) mean += vReal[i];
        mean /= SAMPLES;
        for(int i = 0; i < SAMPLES; i++) vReal[i] -= mean;

        FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
        FFT.compute(FFTDirection::Forward); 
        FFT.complexToMagnitude(); 

        double maxMagnitude = 0;
        for(int i = 1; i < (SAMPLES / 2); i++) {
          double freq = (i * SAMPLING_FREQ) / SAMPLES;
          if (freq >= 0.1 && freq <= 1.0) { 
            if (vReal[i] > maxMagnitude) maxMagnitude = vReal[i];
          }
        }
        
        if (maxMagnitude >= FFT_MAGNITUDE_THRESHOLD) {
          lastValidBreathTime = millis();
          isApneaActive = false; 
        } 
        sampleIndex = 0;
      }
    }
  }

  if ((millis() - lastValidBreathTime > APNEA_TIMEOUT_MS) && !isApneaActive) {
    isApneaActive = true; totalApneaEvents++; 
  }
  float hoursElapsed = (millis() - sessionStartTime) / 3600000.0; 
  if (hoursElapsed > 0.083) currentAHI = totalApneaEvents / hoursElapsed;

  if (isApneaActive || isSeizureActive) {
    if ((millis() / 250) % 2 == 0) digitalWrite(BUZZER_PIN, HIGH);
    else digitalWrite(BUZZER_PIN, LOW);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // =========================================================
  // 🧪 MOCK DATA GENERATOR FOR DASHBOARD TESTING
  // =========================================================
  static unsigned long lastMockTime = 0;
  if (millis() - lastMockTime > 2000) { // Updates every 2 seconds
    ecgHeartRate = random(60, 91); // Random value between 60 and 90
    ppgHeartRate = random(60, 91); // Random value between 60 and 90
    // SpO2 simulation removed; pulls real data now
    lastMockTime = millis();
  }

  // --- WINDOW 4: OLED DISPLAY (2Hz) ---
  if (millis() - lastOLEDUpdate > 500) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("E-HR:"); display.print(ecgHeartRate);
    display.print(" P-HR:"); display.println(ppgHeartRate);
    display.setCursor(0, 16);
    display.print("SpO2:"); display.print(spo2, 0); disp