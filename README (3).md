# Autonomous Tractor Guidance System (ATGS)

An integrated tractor-mounted precision agriculture platform combining **autonomous navigation**, **weed removal**, **soil pH monitoring**, and **pest detection** using IoT sensors and Arduino Mega 2560.

> Domain: Automobile & ATV | Category: Agri-Technology  
> Chennai Institute of Technology

---

## System Architecture

![System Diagram](images/system_diagram.png)

---

## 4 Modules

| Module | Function | Accuracy |
|---|---|---|
| MOD 1 — Navigation | GPS + IMU + IR + Ultrasonic | < 2.5 cm error |
| MOD 2 — Weed Removal | OV7670 Camera + Servo Arm | 95%+ accuracy |
| MOD 3 — Soil Monitoring | pH + Moisture sensors | ±0.05 accuracy |
| MOD 4 — Pest Detection | Camera + DHT22 | 92%+ accuracy |

---

## Components Used

| Component | Purpose |
|---|---|
| Arduino Mega 2560 | Central controller |
| NEO-6M GPS Module | Accurate field navigation |
| MPU-6050 IMU | Orientation and drift correction |
| IR Line Sensors | Row tracking |
| HC-SR04 Ultrasonic | Obstacle detection |
| L298N Motor Driver | Left/Right motor control |
| MG996R Servo x3 | Steering + Weed removal arm |
| OV7670 Camera | Weed and pest visual detection |
| SEN0161 Soil pH Sensor | Real-time pH measurement |
| Soil Moisture Sensor | Irrigation alerts |
| DHT22 | Temperature + Humidity for pest risk |

![Components](images/components.png)

---

## Circuit Diagram

![Circuit](images/circuit_diagram.png)

---

## Impact vs Traditional Farming

| Parameter | Without ATGS | With ATGS | Improvement |
|---|---|---|---|
| Steering Accuracy | >30 cm error | <2.5 cm error | ~92% better |
| Weeding Cost | Rs. 8,000/acre | Rs. 3,200/acre | 60% reduction |
| Soil pH Testing | Never/Rarely | Every field pass | Continuous |
| Pesticide Usage | 100% blanket | 50–60% targeted | 40–50% saved |
| Overall Yield | Baseline | +20–30% est | Multi-module |

---

## Project Structure

```
atgs/
├── atgs_main.ino          # Arduino Mega main code (all 4 modules)
├── atgs_dashboard.py      # Streamlit live monitoring dashboard
├── requirements.txt       # Python dependencies
└── images/                # Diagrams and component photos
```

---

## Setup Instructions

### Arduino
1. Install libraries in Arduino IDE:
   - `TinyGPS++`
   - `Adafruit MPU6050`
   - `Adafruit Unified Sensor`
   - `DHT sensor library`
   - `Servo` (built-in)
2. Open `atgs_main.ino` → Upload to Arduino Mega 2560

### Python Dashboard
```bash
pip install -r requirements.txt
streamlit run atgs_dashboard.py
```

---

## Author

**Divya M**  
B.E. Electronics & Communication (Advanced Communication Technology)  
Chennai Institute of Technology  
[GitHub](https://github.com/Divya0725)
