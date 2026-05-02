"""
ATGS - Real-Time Monitoring Dashboard
--------------------------------------
Reads sensor data from Arduino Mega via Serial
and displays a live Streamlit dashboard.

Run: streamlit run atgs_dashboard.py
"""

import streamlit as st
import serial
import re
import time
import pandas as pd
from datetime import datetime

# ─── Config ───────────────────────────────────────────────────
SERIAL_PORT = "COM3"      # Change to your port (Linux: /dev/ttyUSB0)
BAUD_RATE   = 9600

st.set_page_config(page_title="ATGS Dashboard", page_icon="🚜", layout="wide")
st.title("🚜 Autonomous Tractor Guidance System — Live Dashboard")
st.caption("Real-time monitoring: Navigation | Weed | Soil | Pest")

# ─── Demo Data (when Arduino not connected) ───────────────────
import random
def get_demo_data():
    return {
        "nav":       random.choice(["FORWARD", "CORRECT_LEFT", "CORRECT_RIGHT", "OBSTACLE"]),
        "gps":       "11.0351, 76.9320",
        "obstacle":  random.randint(20, 200),
        "weed":      random.choice(["CLEAR", "DETECTED"]),
        "soil_ph":   round(random.uniform(5.0, 8.0), 2),
        "moisture":  round(random.uniform(20, 80), 1),
        "temp":      round(random.uniform(25, 38), 1),
        "humidity":  round(random.uniform(50, 90), 1),
        "pest_risk": random.choice(["NO", "YES"]),
    }

# ─── Parse Serial ─────────────────────────────────────────────
def parse_serial_block(block):
    def extract(pattern, text, default="--"):
        m = re.search(pattern, text)
        return m.group(1) if m else default

    return {
        "nav":       extract(r"Nav\s*:\s*(\w+)", block),
        "gps":       extract(r"GPS\s*:\s*([\d., -]+)", block),
        "obstacle":  extract(r"Obstacle\s*:\s*(\d+)", block),
        "weed":      extract(r"Weed\s*:\s*(\w+)", block),
        "soil_ph":   extract(r"Soil pH\s*:\s*([\d.]+)", block),
        "moisture":  extract(r"Moisture\s*:\s*([\d.]+)", block),
        "temp":      extract(r"Temp\s*:\s*([\d.]+)", block),
        "humidity":  extract(r"Humidity\s*:\s*([\d.]+)", block),
        "pest_risk": extract(r"PestRisk\s*:\s*(\w+)", block),
    }

# ─── Color helpers ────────────────────────────────────────────
def nav_color(status):
    return {"FORWARD": "#1e8449", "OBSTACLE": "#c0392b",
            "CORRECT_LEFT": "#d68910", "CORRECT_RIGHT": "#d68910",
            "WEED_REMOVE": "#1a5276"}.get(status, "#888")

def ph_status(ph):
    try:
        ph = float(ph)
        if ph < 5.5:   return "ACIDIC ⚠️", "#c0392b"
        if ph > 7.5:   return "ALKALINE ⚠️", "#d68910"
        return "OPTIMAL ✅", "#1e8449"
    except: return "--", "#888"

# ─── Dashboard Loop ───────────────────────────────────────────
history = []
placeholder = st.empty()

for _ in range(200):
    try:
        ser   = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=3)
        lines = [ser.readline().decode("utf-8", errors="ignore") for _ in range(12)]
        block = "\n".join(lines)
        data  = parse_serial_block(block)
        ser.close()
    except Exception:
        data = get_demo_data()

    data["time"] = datetime.now().strftime("%H:%M:%S")
    history.append(data)
    if len(history) > 30: history.pop(0)
    df = pd.DataFrame(history)

    with placeholder.container():
        # ── Module Status Cards ──
        col1, col2, col3, col4 = st.columns(4)

        col1.markdown(f"""
        <div style='background:#eaf4fd;border-radius:10px;padding:14px;text-align:center'>
        <p style='margin:0;font-size:12px;color:#888'>MOD 1 — Navigation</p>
        <p style='margin:4px 0;font-size:20px;font-weight:bold;color:{nav_color(data["nav"])}'>{data["nav"]}</p>
        <p style='margin:0;font-size:11px;color:#555'>📍 {data["gps"]}</p>
        <p style='margin:0;font-size:11px;color:#555'>Obstacle: {data["obstacle"]} cm</p>
        </div>""", unsafe_allow_html=True)

        weed_color = "#c0392b" if data["weed"] == "DETECTED" else "#1e8449"
        col2.markdown(f"""
        <div style='background:#fef9e7;border-radius:10px;padding:14px;text-align:center'>
        <p style='margin:0;font-size:12px;color:#888'>MOD 2 — Weed Status</p>
        <p style='margin:4px 0;font-size:20px;font-weight:bold;color:{weed_color}'>{data["weed"]}</p>
        <p style='margin:0;font-size:11px;color:#555'>OV7670 + Servo Arm</p>
        </div>""", unsafe_allow_html=True)

        ph_label, ph_color = ph_status(data["soil_ph"])
        col3.markdown(f"""
        <div style='background:#e9f7ef;border-radius:10px;padding:14px;text-align:center'>
        <p style='margin:0;font-size:12px;color:#888'>MOD 3 — Soil</p>
        <p style='margin:4px 0;font-size:20px;font-weight:bold;color:{ph_color}'>pH {data["soil_ph"]}</p>
        <p style='margin:0;font-size:11px;color:#555'>{ph_label} | Moisture: {data["moisture"]}%</p>
        </div>""", unsafe_allow_html=True)

        pest_color = "#c0392b" if data["pest_risk"] == "YES" else "#1e8449"
        col4.markdown(f"""
        <div style='background:#fdecea;border-radius:10px;padding:14px;text-align:center'>
        <p style='margin:0;font-size:12px;color:#888'>MOD 4 — Pest Risk</p>
        <p style='margin:4px 0;font-size:20px;font-weight:bold;color:{pest_color}'>{data["pest_risk"]}</p>
        <p style='margin:0;font-size:11px;color:#555'>🌡️ {data["temp"]}°C 💧 {data["humidity"]}%</p>
        </div>""", unsafe_allow_html=True)

        # ── Soil pH Chart ──
        st.markdown("### 📊 Soil pH Trend")
        if len(df) > 1 and "soil_ph" in df.columns:
            chart_df = df[["time", "soil_ph"]].copy()
            chart_df["soil_ph"] = pd.to_numeric(chart_df["soil_ph"], errors="coerce")
            st.line_chart(chart_df.set_index("time"))

    time.sleep(2)
