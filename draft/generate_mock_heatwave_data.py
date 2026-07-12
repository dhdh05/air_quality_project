import os
import random
import math
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime, timedelta, timezone

# Set paths
base_dir = os.path.dirname(os.path.abspath(__file__))
data_dir = os.path.join(base_dir, "..", "data")
media_dir = os.path.join(base_dir, "..", "media")
os.makedirs(data_dir, exist_ok=True)
os.makedirs(media_dir, exist_ok=True)

# Generate hourly timestamps from May 15, 2026, to July 8, 2026
start_time = datetime(2026, 5, 15, 0, 0, 0, tzinfo=timezone.utc)
end_time = datetime(2026, 7, 8, 23, 0, 0, tzinfo=timezone.utc)

timestamps = []
current_time = start_time
while current_time <= end_time:
    timestamps.append(current_time)
    current_time += timedelta(hours=1)

total_records = len(timestamps)
print(f"Generating simulated dataset containing {total_records} hours of logs...")

# --- 1. Generate Indoor Dataset (Room) ---
print("Generating indoor logs...")
indoor_records = []
for idx, ts in enumerate(timestamps):
    hour = ts.hour
    
    # Check peak heatwave (June 1 - June 25)
    is_peak_heatwave = (6 <= ts.month <= 6) and (1 <= ts.day <= 25)
    
    # AC active during working hours (8 AM - 6 PM)
    if 8 <= hour <= 18:
        temp = round(25.0 + random.uniform(-0.5, 0.5), 1)
        hum = round(52.0 + random.uniform(-1.5, 1.5), 1)
    else:
        base_night_temp = 28.5 if is_peak_heatwave else 27.2
        temp = round(base_night_temp + 0.6 * math.sin(idx * 0.25) + random.uniform(-0.3, 0.3), 1)
        hum = round(60.0 - 2.0 * math.sin(idx * 0.25) + random.uniform(-1.0, 1.0), 1)
        
    # Pressure (hPa) - slight daily variation
    pressure = round(1009.5 + 1.2 * math.cos(idx * 0.1) + random.uniform(-0.2, 0.2), 1)
    
    # MQ135: Stable clean indoor levels
    mq135_ppm = round(160.0 + 20.0 * math.sin(idx * 0.05) + random.uniform(-6.0, 6.0), 1)
    mq135_v = round(0.85 + 0.06 * math.sin(idx * 0.05) + random.uniform(-0.02, 0.02), 2)
    
    # PM2.5/PM10: Low dust levels indoor
    pm2_5 = round(23.0 + 5.0 * math.sin(idx * 0.04) + random.uniform(-2.0, 2.0), 1)
    pm10 = round(pm2_5 * 1.2 + random.uniform(1.0, 3.0), 1)
    
    indoor_records.append({
        "id": idx + 10000,
        "created_at": ts.isoformat(),
        "pm2_5": pm2_5,
        "pm10": pm10,
        "temperature": temp,
        "humidity": hum,
        "pressure": pressure,
        "mq135_ppm": mq135_ppm,
        "mq135_v": mq135_v,
        "status": 0,
        "warning": False
    })

df_indoor = pd.DataFrame(indoor_records)
df_indoor['created_at'] = pd.to_datetime(df_indoor['created_at'])
df_indoor.to_csv(os.path.join(data_dir, "indoor_logs.csv"), index=False)
print("[OK] Saved: data/indoor_logs.csv")

# --- 2. Generate Outdoor Dataset (Parking Garage / Nhà gửi xe) ---
print("Generating outdoor (parking garage) logs...")
outdoor_records = []
for idx, ts in enumerate(timestamps):
    hour = ts.hour
    
    # 3-Phase Weather Profile
    if ts.month == 5:
        base_temp = 32.5
        amplitude = 3.5
    elif ts.month == 6 and ts.day <= 25: # Heatwave
        base_temp = 36.5
        amplitude = 5.5
    else: # Rainy phase
        base_temp = 31.0
        amplitude = 2.5
        
    time_factor = math.cos((hour - 14) * math.pi / 12)
    temp = round(base_temp + amplitude * time_factor + random.uniform(-0.5, 0.5), 1)
    
    base_hum = 68.0 if (ts.month == 7 or (ts.month == 6 and ts.day > 25)) else 56.0
    hum = round(base_hum - 12.0 * time_factor + random.uniform(-2.0, 2.0), 1)
    
    # Pressure (hPa)
    pressure = round(1008.2 + 1.5 * math.cos(idx * 0.1) + random.uniform(-0.3, 0.3), 1)
    
    # Vehicle rush hours in the garage: 7-9 AM and 5-7 PM
    is_rush_hour = (7 <= hour <= 9) or (17 <= hour <= 19)
    
    if is_rush_hour:
        mq135_ppm = round(960.0 + random.uniform(50.0, 310.0), 1)
        mq135_v = round(2.70 + random.uniform(0.10, 0.42), 2)
        pm2_5 = round(162.0 + random.uniform(15.0, 110.0), 1)
    else:
        mq135_ppm = round(460.0 + 90.0 * math.sin(idx * 0.08) + random.uniform(-20.0, 20.0), 1)
        mq135_v = round(1.60 + 0.22 * math.sin(idx * 0.08) + random.uniform(-0.04, 0.04), 2)
        pm2_5 = round(90.0 + 20.0 * math.sin(idx * 0.06) + random.uniform(-8.0, 8.0), 1)
        
    pm10 = round(pm2_5 * 1.3 + random.uniform(2.0, 8.0), 1)
    
    # Generate realistic Reference PM2.5/PM10 (PamAir)
    # Hygroscopic effect model: sensor overestimates dust under high humidity!
    # ref_pm25 = raw_pm25 / (1.0 + 0.005 * (humidity - 40)^1.5)
    hum_factor = 1.0 + 0.005 * max(0, hum - 35.0)**1.3
    reference_pm25 = round(pm2_5 / hum_factor + random.uniform(-1.5, 1.5), 1)
    reference_pm25 = max(5.0, reference_pm25)
    
    reference_pm10 = round(pm10 / (1.0 + 0.003 * max(0, hum - 35.0)**1.3) + random.uniform(-2.5, 2.5), 1)
    reference_pm10 = max(8.0, reference_pm10)
    
    warning = (temp > 40.0) or (mq135_ppm > 1000.0) or (pm2_5 > 150.0)
    
    outdoor_records.append({
        "id": idx + 20000,
        "created_at": ts.isoformat(),
        "pm2_5": pm2_5,
        "pm10": pm10,
        "temperature": temp,
        "humidity": hum,
        "pressure": pressure,
        "mq135_ppm": mq135_ppm,
        "mq135_v": mq135_v,
        "status": 0,
        "warning": warning,
        "reference_pm25": reference_pm25,
        "reference_pm10": reference_pm10
    })

df_outdoor = pd.DataFrame(outdoor_records)
df_outdoor['created_at'] = pd.to_datetime(df_outdoor['created_at'])
df_outdoor.to_csv(os.path.join(data_dir, "outdoor_logs.csv"), index=False)
print("[OK] Saved: data/outdoor_logs.csv")

# Define global plot styles
plt.rcParams['font.family'] = 'sans-serif'
plt.rcParams['font.sans-serif'] = ['DejaVu Sans', 'Arial', 'Helvetica']
plt.rcParams['axes.edgecolor'] = '#CCCCCC'
plt.rcParams['axes.linewidth'] = 0.8

# Helper function to plot individual metrics on long timelines
def draw_long_term_chart(df, col_name, title, ylabel, color, threshold=None, filepath=None):
    fig, ax = plt.subplots(figsize=(12, 5), dpi=150)
    
    # Plot line
    ax.plot(df['created_at'], df[col_name], color=color, linewidth=1.2, label=title)
    ax.fill_between(df['created_at'], df[col_name], color=color, alpha=0.06)
    
    # Optional threshold line and highlight
    if threshold is not None:
        ax.axhline(y=threshold, color='#D35400', linestyle='--', linewidth=1.5, label=f'Ngưỡng cảnh báo ({threshold})')
        # Highlight regions exceeding the warning threshold in very soft red
        for i in range(0, len(df) - 1, 2):
            if df[col_name].iloc[i] > threshold or df[col_name].iloc[i+1] > threshold:
                ax.axvspan(df['created_at'].iloc[i], df['created_at'].iloc[i+1], color='#FF0000', alpha=0.08)

    ax.set_xlabel('Thời gian (Ngày/Tháng)', labelpad=10, fontweight='bold', color='#333333')
    ax.set_ylabel(ylabel, fontweight='bold', color=color)
    ax.tick_params(axis='y', labelcolor=color)
    ax.grid(True, linestyle='--', alpha=0.4, color='#DDDDDD')
    
    # Format dates (Weekly ticks since timeline is ~55 days)
    ax.xaxis.set_major_locator(mdates.DayLocator(interval=7))
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%d/%m'))
    fig.autofmt_xdate()
    
    plt.title(title, fontsize=12, fontweight='bold', pad=12, color='#2C3E50')
    if threshold is not None:
        ax.legend(loc='upper right', frameon=True, facecolor='white', edgecolor='#EEEEEE')
        
    plt.tight_layout()
    plt.savefig(filepath, bbox_inches='tight')
    plt.close()
    print(f"[OK] Saved chart: {filepath}")

# --- 3. Generate Individual Charts for Indoor ---
print("\nPlotting Indoor (Room) long-term individual charts...")
draw_long_term_chart(df_indoor, "temperature", "Biểu đồ Nhiệt độ trong phòng", "Nhiệt độ (°C)", "#E74C3C", None, os.path.join(media_dir, "indoor_temp.png"))
draw_long_term_chart(df_indoor, "humidity", "Biểu đồ Độ ẩm trong phòng", "Độ ẩm (%)", "#3498DB", None, os.path.join(media_dir, "indoor_humidity.png"))
draw_long_term_chart(df_indoor, "pm2_5", "Biểu đồ Nồng độ bụi mịn PM2.5 trong phòng", "Nồng độ bụi PM2.5 (µg/m³)", "#D35400", None, os.path.join(media_dir, "indoor_pm25.png"))
draw_long_term_chart(df_indoor, "pm10", "Biểu đồ Nồng độ bụi PM10 trong phòng", "Nồng độ bụi PM10 (µg/m³)", "#E67E22", None, os.path.join(media_dir, "indoor_pm10.png"))
draw_long_term_chart(df_indoor, "mq135_ppm", "Biểu đồ Nồng độ khí MQ-135 trong phòng", "Nồng độ khí (PPM)", "#8E44AD", None, os.path.join(media_dir, "indoor_mq135.png"))
draw_long_term_chart(df_indoor, "pressure", "Biểu đồ Áp suất khí quyển trong phòng", "Áp suất (hPa)", "#16A085", None, os.path.join(media_dir, "indoor_pressure.png"))

# --- 4. Generate Individual Charts for Outdoor ---
print("\nPlotting Outdoor (Parking Garage) long-term individual charts...")
draw_long_term_chart(df_outdoor, "temperature", "Biểu đồ Nhiệt độ nhà xe ngoài trời", "Nhiệt độ (°C)", "#E74C3C", 40.0, os.path.join(media_dir, "outdoor_temp.png"))
draw_long_term_chart(df_outdoor, "humidity", "Biểu đồ Độ ẩm nhà xe ngoài trời", "Độ ẩm (%)", "#3498DB", None, os.path.join(media_dir, "outdoor_humidity.png"))
draw_long_term_chart(df_outdoor, "pm2_5", "Biểu đồ Nồng độ bụi mịn PM2.5 nhà xe ngoài trời", "Nồng độ bụi PM2.5 (µg/m³)", "#D35400", 150.0, os.path.join(media_dir, "outdoor_pm25.png"))
draw_long_term_chart(df_outdoor, "pm10", "Biểu đồ Nồng độ bụi PM10 nhà xe ngoài trời", "Nồng độ bụi PM10 (µg/m³)", "#E67E22", 150.0, os.path.join(media_dir, "outdoor_pm10.png"))
draw_long_term_chart(df_outdoor, "mq135_ppm", "Biểu đồ Nồng độ khí thải MQ-135 nhà xe", "Nồng độ khí (PPM)", "#8E44AD", 1000.0, os.path.join(media_dir, "outdoor_mq135.png"))
draw_long_term_chart(df_outdoor, "pressure", "Biểu đồ Áp suất khí quyển ngoài trời nhà xe", "Áp suất (hPa)", "#16A085", None, os.path.join(media_dir, "outdoor_pressure.png"))

print("\n=======================================================")
print("  DATA & BASIC INDIVIDUAL CHARTS RERENDERED SUCCESSFULLY!")
print("=======================================================")
