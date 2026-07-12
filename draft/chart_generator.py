import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

# Set paths relative to script location
base_dir = os.path.dirname(os.path.abspath(__file__))
csv_path = os.path.join(base_dir, "..", "data", "air_quality_logs_rows.csv")
media_dir = os.path.join(base_dir, "..", "media")
os.makedirs(media_dir, exist_ok=True)

# 1. Read and parse CSV data
if not os.path.exists(csv_path):
    print(f"[ERROR] CSV file not found at: {csv_path}")
    exit(1)

df = pd.read_csv(csv_path)
df['created_at'] = pd.to_datetime(df['created_at'], format='mixed')
# Sort chronologically (earliest to latest)
df = df.sort_values('created_at').reset_index(drop=True)

# Drop rows with critical null fields for temperature & humidity plotting
df_clean = df.dropna(subset=['temperature', 'humidity']).copy()

# Drop rows with critical null fields and filter out tiny near-zero values (< 1.0) for MQ135/GP2Y plotting
df_clean_aq = df.dropna(subset=['mq135_ppm', 'dust_density']).copy()
df_clean_aq = df_clean_aq[df_clean_aq['mq135_ppm'] > 1.0]

# Define custom styling for professional look
plt.rcParams['font.family'] = 'sans-serif'
plt.rcParams['font.sans-serif'] = ['DejaVu Sans', 'Arial', 'Helvetica']
plt.rcParams['axes.edgecolor'] = '#CCCCCC'
plt.rcParams['axes.linewidth'] = 0.8

# ----------------- CHART 1: Temperature & Humidity -----------------
print("Generating Temperature & Humidity trend chart...")
fig, ax1 = plt.subplots(figsize=(12, 6), dpi=150)

# Temperature (Left Axis)
color_temp = '#E74C3C'  # Crimson Red
ax1.set_xlabel('Thời gian (Ngày/Tháng Giờ:Phút)', labelpad=12, fontweight='bold', color='#333333')
ax1.set_ylabel('Nhiệt độ (°C)', color=color_temp, fontweight='bold')
line1 = ax1.plot(df_clean['created_at'], df_clean['temperature'], color=color_temp, linewidth=2, label='Nhiệt độ (°C)')
ax1.tick_params(axis='y', labelcolor=color_temp)
ax1.grid(True, linestyle='--', alpha=0.5, color='#DDDDDD')

# Fill under temperature line
ax1.fill_between(df_clean['created_at'], df_clean['temperature'], color=color_temp, alpha=0.08)

# Humidity (Right Axis)
ax2 = ax1.twinx()
color_hum = '#3498DB'  # Sky Blue
ax2.set_ylabel('Độ ẩm (%)', color=color_hum, fontweight='bold')
line2 = ax2.plot(df_clean['created_at'], df_clean['humidity'], color=color_hum, linewidth=2, linestyle='-', label='Độ ẩm (%)')
ax2.tick_params(axis='y', labelcolor=color_hum)

# Fill under humidity line
ax2.fill_between(df_clean['created_at'], df_clean['humidity'], color=color_hum, alpha=0.05)

# Formatting time axis
ax1.xaxis.set_major_formatter(mdates.DateFormatter('%d/%m %H:%M'))
fig.autofmt_xdate()

# Title and Layout
plt.title('Biểu đồ xu hướng Nhiệt độ & Độ ẩm', fontsize=14, fontweight='bold', pad=16, color='#2C3E50')
lines = line1 + line2
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc='upper left', frameon=True, facecolor='white', edgecolor='#EEEEEE')

plt.tight_layout()
temp_hum_chart_path = os.path.join(media_dir, "temp_humidity_chart.png")
plt.savefig(temp_hum_chart_path, bbox_inches='tight')
plt.close()
print(f"[OK] Saved temperature & humidity chart to: {temp_hum_chart_path}")

# ----------------- CHART 2: Air Quality (MQ135 PPM & Dust Density) -----------------
print("Generating Air Quality trend chart...")
fig, ax1 = plt.subplots(figsize=(12, 6), dpi=150)

# MQ135 PPM (Left Axis)
color_mq135 = '#8E44AD'  # Amethyst Purple
ax1.set_xlabel('Thời gian (Ngày/Tháng Giờ:Phút)', labelpad=12, fontweight='bold', color='#333333')
ax1.set_ylabel('Nồng độ khí MQ135 (PPM)', color=color_mq135, fontweight='bold')
line1 = ax1.plot(df_clean_aq['created_at'], df_clean_aq['mq135_ppm'], color=color_mq135, linewidth=2, label='Khí Gas MQ-135 (PPM)')
ax1.tick_params(axis='y', labelcolor=color_mq135)
ax1.grid(True, linestyle='--', alpha=0.5, color='#DDDDDD')

# Add threshold line for MQ135 (1000 PPM)
thresh1 = ax1.axhline(y=1000, color='#D35400', linestyle=':', linewidth=1.5, label='Ngưỡng cảnh báo MQ135 (1000 PPM)')

# GP2Y Dust Density (Right Axis)
ax2 = ax1.twinx()
color_dust = '#F39C12'  # Orange/Yellow
ax2.set_ylabel('Nồng độ bụi mịn GP2Y (µg/m³)', color=color_dust, fontweight='bold')
line2 = ax2.plot(df_clean_aq['created_at'], df_clean_aq['dust_density'], color=color_dust, linewidth=2, label='Bụi mịn GP2Y10 (µg/m³)')
ax2.tick_params(axis='y', labelcolor=color_dust)

# Add threshold line for Dust (150 ug/m3)
thresh2 = ax2.axhline(y=150, color='#C0392B', linestyle='--', linewidth=1.5, label='Ngưỡng nguy hại Bụi (150 µg/m³)')

# Formatting time axis
ax1.xaxis.set_major_formatter(mdates.DateFormatter('%d/%m %H:%M'))
fig.autofmt_xdate()

# Title and Layout
plt.title('Biểu đồ xu hướng Chất lượng không khí & Bụi mịn', fontsize=14, fontweight='bold', pad=16, color='#2C3E50')
lines = line1 + [thresh1] + line2 + [thresh2]
labels = [l.get_label() for l in lines]
ax1.legend(lines, labels, loc='upper left', frameon=True, facecolor='white', edgecolor='#EEEEEE')

plt.tight_layout()
air_quality_chart_path = os.path.join(media_dir, "air_quality_chart.png")
plt.savefig(air_quality_chart_path, bbox_inches='tight')
plt.close()
print(f"[OK] Saved air quality chart to: {air_quality_chart_path}")

