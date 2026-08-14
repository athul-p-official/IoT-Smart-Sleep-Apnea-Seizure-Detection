IoT-Based Smart Sleep Apnea and Seizure Detection System

📌 Project Overview

The IoT-Based Smart Sleep Apnea and Seizure Detection System is an ESP32-based biomedical monitoring prototype designed for continuous monitoring of multiple physiological and motion parameters.

The system integrates an AD8232 ECG sensor, MAX30102 pulse oximeter, MPU6050 accelerometer/gyroscope, SSD1306 OLED display, buzzer alarm, Wi-Fi connectivity, web-based dashboard, and ThingSpeak cloud communication.

The primary objective is to develop a low-cost embedded platform capable of monitoring physiological parameters and identifying potential sleep-apnea events and abnormal movement patterns during sleep.

The system provides both local monitoring through an OLED display and remote monitoring through an ESP32-hosted web dashboard.

---

🎯 Objectives

- Monitor ECG signals using the AD8232 sensor.
- Monitor heart rate using ECG and PPG techniques.
- Measure blood oxygen saturation (SpO₂) using the MAX30102.
- Monitor body movement using the MPU6050.
- Analyze motion signals using Fast Fourier Transform (FFT).
- Develop a prototype sleep-apnea event detection algorithm.
- Develop a prototype abnormal-movement/seizure detection algorithm.
- Generate an audible alarm during detected events.
- Display vital parameters locally using an OLED display.
- Provide a Wi-Fi-based real-time monitoring dashboard.
- Store monitoring records in the browser.
- Upload selected parameters to ThingSpeak for IoT-based remote monitoring.

---

🧠 System Concept

The system combines physiological sensing and motion sensing into a single ESP32-based platform.

                 ┌─────────────────────────┐
                 │          ESP32           │
                 │   Main Processing Unit   │
                 └────────────┬────────────┘
                              │
        ┌─────────────────────┼─────────────────────┐
        │                     │                     │
        ▼                     ▼                     ▼
   ┌─────────┐          ┌──────────┐          ┌──────────┐
   │ AD8232  │          │ MAX30102 │          │ MPU6050  │
   │   ECG   │          │ PPG/SpO₂ │          │ Motion   │
   └────┬────┘          └────┬─────┘          └────┬─────┘
        │                    │                     │
        ▼                    ▼                     ▼
 ECG Signal            HR + SpO₂             Acceleration
 Filtering             Detection              Analysis
        │                    │                     │
        ▼                    │                ┌────┴─────┐
 Peak Detection             │                │          │
        │                    │                ▼          ▼
        │                    │              FFT      Jerk/
        │                    │             Analysis   Motion
        │                    │                │        Index
        │                    │                ▼          │
        └────────────────────┼───────────────┴──────────┘
                             │
                             ▼
                  ┌──────────────────────┐
                  │  Monitoring & Alarm  │
                  └──────────┬───────────┘
                             │
              ┌──────────────┼──────────────┐
              ▼              ▼              ▼
            OLED       Web Dashboard    ThingSpeak
                             │
                             ▼
                       Browser Storage

---

🔬 Hardware Components

Component| Quantity| Purpose
ESP32 Development Board| 1| Main controller and IoT connectivity
AD8232 ECG Module| 1| ECG signal acquisition
MAX30102| 1| PPG and SpO₂ monitoring
MPU6050| 1| Motion and acceleration sensing
SSD1306 OLED 128×64| 1| Local display
Buzzer| 1| Audible alarm
ECG electrodes| 3| ECG signal acquisition
Li-ion/LiPo battery| 1| Portable power source
Connecting wires| As required| Electrical connections

---

🔌 Pin Configuration

AD8232 → ESP32

AD8232 Pin| ESP32 Pin
3.3V| 3V3
GND| GND
OUTPUT| GPIO 34
LO−| GPIO 4
LO+| GPIO 5

MAX30102 → ESP32

MAX30102| ESP32
VIN/VCC| 3.3V
GND| GND
SDA| GPIO 21
SCL| GPIO 22

MPU6050 → ESP32

MPU6050| ESP32
VCC| 3.3V
GND| GND
SDA| GPIO 21
SCL| GPIO 22

SSD1306 OLED → ESP32

OLED| ESP32
VCC| 3.3V
GND| GND
SDA| GPIO 21
SCL| GPIO 22

Buzzer → ESP32

Buzzer| ESP32
Positive| GPIO 26
Negative| GND

---

🧰 Software and Tools

Development

- Arduino IDE
- C/C++
- ESP32 Arduino Core

Libraries

Wire
WiFi
WebServer
HTTPClient
Adafruit GFX
Adafruit SSD1306
MAX30105
heartRate
arduinoFFT

IoT

- ESP32 Wi-Fi
- ThingSpeak
- Embedded HTTP communication

Dashboard

- HTML
- CSS
- JavaScript
- ESP32 WebServer
- Browser LocalStorage

---

⚙️ Working Principle

1. ECG Monitoring

The AD8232 module acquires the electrical activity of the heart.

The analog ECG output is connected to GPIO 34, which is an ADC input of the ESP32.

The acquired signal is processed using:

Raw ECG
   ↓
Moving Average Filter
   ↓
Exponential Moving Average
   ↓
Peak Detection
   ↓
RR Interval
   ↓
Heart Rate

The heart rate is estimated from the interval between detected ECG peaks.

The system also monitors the AD8232 LO+ and LO− lead-off outputs to identify possible electrode disconnection.

---

❤️ ECG Processing

A moving-average filter is implemented using a 10-sample buffer.

#define FILTER_SIZE 10

The filtered signal is then passed through an exponential moving average:

float emaAlpha = 0.3;

A configurable threshold is used for prototype peak detection:

int peakThreshold = 2200;

The instantaneous heart rate is estimated using:

BPM = 60000 / RR interval (ms)

---

🩸 MAX30102 Monitoring

The MAX30102 provides red and infrared optical measurements.

The infrared signal is used for pulse detection.

The prototype checks the IR signal to determine whether a finger is present.

Finger Detection
       ↓
IR Signal
       ↓
Beat Detection
       ↓
Heart Rate

The red-to-infrared ratio is also used in the current prototype to estimate SpO₂.

«Note: The current SpO₂ calculation is a simplified prototype estimation and is not a clinically validated pulse-oximetry algorithm.»

---

🫁 Sleep Apnea Detection

The MPU6050 is used to monitor body movement associated with breathing.

The acceleration data is converted into an orientation-related signal.

A block of 64 samples is collected at approximately 10 Hz.

Sampling frequency = 10 Hz
Number of samples = 64
Window duration ≈ 6.4 seconds

FFT processing is then performed on the motion signal.

The system analyzes frequencies approximately between:

0.1 Hz – 1.0 Hz

to identify low-frequency movement associated with breathing.

If sufficient breathing-related motion is detected, the breathing timer is updated.

If no valid breathing-related movement is detected for the configured timeout period:

10 seconds

the prototype generates an apnea alarm event.

---

📊 AHI Calculation

The system estimates an Apnea-Hypopnea Index using:

AHI = Number of detected apnea events / Monitoring time in hours

For example:

Apnea events = 5
Monitoring time = 2 hours

AHI = 5 / 2
    = 2.5 events/hour

The AHI calculation in this project is intended for prototype evaluation and algorithm development, not clinical diagnosis.

---

⚡ Abnormal Movement / Seizure Detection

The MPU6050 continuously monitors acceleration along three axes:

X-axis
Y-axis
Z-axis

The difference between current and previous acceleration is used to estimate rapid movement:

Jerk X = |Current X - Previous X|
Jerk Y = |Current Y - Previous Y|
Jerk Z = |Current Z - Previous Z|

A prototype movement index is then calculated:

Movement Index =
(Jerk X + Jerk Y + Jerk Z) × 100

If the index exceeds the configured threshold, an abnormal-movement alarm condition is generated.

«Important: This is an experimental movement-based seizure-detection prototype. It is not a clinically validated seizure detector and should not be used for medical diagnosis.»

---

🔔 Alarm System

A buzzer connected to GPIO 26 provides an audible warning.

The alarm is activated when either of the prototype event conditions becomes active:

Apnea Event
     OR
Abnormal Movement Event
     ↓
Buzzer Alarm

The buzzer is controlled using a periodic ON/OFF pattern to create an alarm signal.

---

📺 OLED Display

The system uses a 128×64 SSD1306 OLED.

The display is intended to show parameters such as:

ECG Heart Rate
PPG Heart Rate
SpO₂
Apnea Index
Movement Index
System Status

The OLED provides local monitoring without requiring a smartphone or computer.

---

🌐 IoT Web Dashboard

The ESP32 runs an HTTP web server.

The main dashboard provides real-time monitoring of:

- ECG heart rate
- PPG heart rate
- SpO₂
- AHI
- Movement/seizure index
- Apnea alarm
- Abnormal-movement alarm
- System status

The browser periodically requests updated sensor data from:

/data

The dashboard automatically updates approximately once per second.

---

☁️ ThingSpeak Integration

When the ESP32 is connected to an external Wi-Fi router, selected parameters can be uploaded to ThingSpeak.

The prototype can transmit:

Field 1 → ECG Heart Rate
Field 2 → SpO₂
Field 3 → AHI
Field 4 → Movement/Seizure Index

The cloud update interval is approximately:

20 seconds

---

💾 Browser Data Storage

The web dashboard uses browser "localStorage" to save monitoring records.

The stored information includes:

Date
Time
ECG HR
PPG HR
SpO₂
AHI
Movement Index

The dashboard also provides a CSV export option.

---

🖥️ Dashboard Features

The dashboard contains:

- ❤️ Heart-rate card
- 🩸 SpO₂ card
- 😴 Apnea index card
- ⚡ Abnormal movement index card
- 🟢 System status
- 💾 Browser data storage
- CSV download
- History clearing
- Live device reset

---

📁 Project Structure

IoT-Smart-Sleep-Apnea-Seizure-Detection/
│
├── README.md
│
├── src/
│   └── biomedical_monitoring.ino
│
├── hardware/
│   ├── block-diagram.png
│   ├── circuit-diagram.png
│   └── pin-configuration.md
│
├── documentation/
│   ├── internship-report.pdf
│   └── presentation.pdf
│
├── images/
│   ├── prototype.jpg
│   ├── ecg-ad8232.jpg
│   ├── max30102.jpg
│   ├── mpu6050.jpg
│   ├── oled.jpg
│   └── dashboard.jpg
│
└── results/
    ├── sample-data.csv
    └── results.md

---

🚀 Installation and Setup

Step 1 — Install Arduino IDE

Install Arduino IDE and configure the ESP32 development board.

Step 2 — Install Required Libraries

Install the required libraries through the Arduino IDE Library Manager.

Required libraries include:

Adafruit GFX Library
Adafruit SSD1306
SparkFun MAX3010x Sensor Library
arduinoFFT

The ESP32 Wi-Fi and WebServer libraries are provided through the ESP32 Arduino core.

Step 3 — Connect the Hardware

Follow the pin configuration provided in:

hardware/pin-configuration.md

Step 4 — Configure Wi-Fi

Inside the source code, replace the placeholders with your own credentials during local testing:

const char* router_ssid = "YOUR_ROUTER_SSID";
const char* router_password = "YOUR_ROUTER_PASSWORD";

Do not commit real credentials to GitHub.

Step 5 — Configure ThingSpeak

Add your ThingSpeak API key locally.

Never upload a real API key to a public repository.

Step 6 — Upload the Firmware

Select the appropriate ESP32 board and COM port in Arduino IDE and upload the firmware.

Step 7 — Connect to the ESP32 Access Point

The prototype creates an access point:

SSID: ESP32-Test
Password: 12345678

After connecting to the ESP32 network, open the IP address shown on the OLED in a browser.

---

🧪 Experimental Results

The prototype demonstrates integration of:

- ECG signal acquisition
- Optical pulse sensing
- SpO₂ estimation
- Motion sensing
- FFT-based motion analysis
- Apnea-event estimation
- Abnormal movement detection
- Local OLED monitoring
- Wi-Fi dashboard
- Cloud communication
- Alarm generation

Example monitoring parameters:

Parameter| Prototype Output
ECG HR| BPM
PPG HR| BPM
SpO₂| %
AHI| events/hour
Movement Index| numerical index
Apnea Alarm| ON/OFF
Abnormal Movement Alarm| ON/OFF

Actual experimental values should be added to the "results/" folder after testing.

---

⚠️ Current Prototype Limitations

This project is an engineering and research prototype.

ECG

The current ECG heart-rate algorithm uses threshold-based peak detection and requires further filtering and calibration for robust operation.

PPG

The MAX30102 heart-rate processing requires further validation under different finger positions, motion conditions, and ambient-light conditions.

SpO₂

The current implementation uses a simplified red/IR ratio equation. It is not equivalent to a medically certified pulse-oximeter algorithm.

Sleep Apnea

The apnea algorithm estimates breathing-related activity using MPU6050 motion data. Body movement, sensor orientation, loose mounting, and other factors can cause false detections.

Seizure Detection

The abnormal-movement algorithm is based on acceleration/jerk thresholds. It does not perform clinical EEG-based seizure diagnosis.

Mock Data

During dashboard development, ECG and PPG heart-rate values may be generated using a mock-data routine for testing the user interface. These values must not be interpreted as actual physiological measurements.

---

🔮 Future Improvements

Possible future improvements include:

- Advanced ECG filtering
- Adaptive ECG R-peak detection
- Improved PPG signal processing
- Validated SpO₂ estimation
- Respiratory-rate extraction
- Improved apnea classification
- Machine-learning-based apnea detection
- Machine-learning-based abnormal movement classification
- Sensor fusion
- Real-time ECG waveform display
- Mobile application
- Secure cloud communication
- Long-term sleep-session analysis
- Battery optimization
- Compact wearable PCB
- Medical-grade sensor validation
- Clinical dataset validation
- False-positive and false-negative analysis

---

🧑‍💻 Technologies Used

ESP32
Arduino C/C++
AD8232
MAX30102
MPU6050
SSD1306 OLED
Wi-Fi
HTTP
ThingSpeak
HTML
CSS
JavaScript
FFT
Signal Processing
IoT
Embedded Systems
Biomedical Instrumentation

---

🎓 Internship Project

This project was developed as part of a biomedical/embedded systems internship project involving the design and implementation of an IoT-enabled physiological monitoring prototype.

The project provided practical exposure to:

- Biomedical sensors
- ECG signal acquisition
- PPG sensing
- Pulse oximetry
- Motion sensing
- Digital signal processing
- FFT analysis
- ESP32 embedded programming
- IoT communication
- Web-based monitoring
- Cloud data logging
- Alarm systems
- Hardware-software integration

---

👨‍💻 Author

Athul P

B.Tech Electronics and Communication Engineering

TKM College of Engineering, Kollam, Kerala, India

---

📜 Disclaimer

This project is intended for educational, research, and prototype development purposes only.

It is not a medical device and has not been clinically validated or approved for diagnosis, treatment, or monitoring of medical conditions.

The apnea and abnormal-movement detection algorithms are experimental implementations. Sensor readings and detected events should not be used as a substitute for professional medical evaluation.

---

📄 License

This project is intended for educational and research use.

See the "LICENSE" file for details.

---

⭐ Acknowledgements

Thanks to the mentors, faculty members, internship coordinators, and institutions involved in the development and testing of this biomedical monitoring prototype.
