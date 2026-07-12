import os
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LinearRegression
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_absolute_error, mean_squared_error, r2_score

# Set paths
base_dir = os.path.dirname(os.path.abspath(__file__))
csv_path = os.path.join(base_dir, "..", "data", "outdoor_logs.csv")
media_dir = os.path.join(base_dir, "..", "media")
os.makedirs(media_dir, exist_ok=True)

# 1. Load data
if not os.path.exists(csv_path):
    print(f"[ERROR] CSV not found at: {csv_path}")
    exit(1)

df = pd.read_csv(csv_path)

# Custom plot styles
plt.rcParams['font.family'] = 'sans-serif'
plt.rcParams['font.sans-serif'] = ['DejaVu Sans', 'Arial', 'Helvetica']
plt.rcParams['axes.edgecolor'] = '#CCCCCC'
plt.rcParams['axes.linewidth'] = 0.8

# ----------------- FIGURE 1: Scatter Raw vs Reference (Hình 4.13 & 4.14) -----------------
print("Generating Scatter Raw vs Reference plot...")
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6), dpi=150)

# PM2.5
ax1.scatter(df['pm2_5'], df['reference_pm25'], color='#3498DB', alpha=0.5, edgecolors='none', label='Dữ liệu đo thực tế')
min_val = min(df['pm2_5'].min(), df['reference_pm25'].min())
max_val = max(df['pm2_5'].max(), df['reference_pm25'].max())
ax1.plot([min_val, max_val], [min_val, max_val], color='#E74C3C', linestyle='--', linewidth=2, label='Đường lý tưởng y = x')
ax1.set_xlabel('Nồng độ PM2.5 cảm biến đo (µg/m³)', fontweight='bold', labelpad=10)
ax1.set_ylabel('Nồng độ PM2.5 tham chiếu PamAir (µg/m³)', fontweight='bold', labelpad=10)
ax1.set_title('So sánh dữ liệu PM2.5 thô cảm biến với PamAir', fontsize=11, fontweight='bold', color='#2C3E50', pad=10)
ax1.grid(True, linestyle='--', alpha=0.5)
ax1.legend(frameon=True, edgecolor='#EEEEEE')

# PM10
ax2.scatter(df['pm10'], df['reference_pm10'], color='#E67E22', alpha=0.5, edgecolors='none', label='Dữ liệu đo thực tế')
min_val_10 = min(df['pm10'].min(), df['reference_pm10'].min())
max_val_10 = max(df['pm10'].max(), df['reference_pm10'].max())
ax2.plot([min_val_10, max_val_10], [min_val_10, max_val_10], color='#E74C3C', linestyle='--', linewidth=2, label='Đường lý tưởng y = x')
ax2.set_xlabel('Nồng độ PM10 cảm biến đo (µg/m³)', fontweight='bold', labelpad=10)
ax2.set_ylabel('Nồng độ PM10 tham chiếu PamAir (µg/m³)', fontweight='bold', labelpad=10)
ax2.set_title('So sánh dữ liệu PM10 thô cảm biến với PamAir', fontsize=11, fontweight='bold', color='#2C3E50', pad=10)
ax2.grid(True, linestyle='--', alpha=0.5)
ax2.legend(frameon=True, edgecolor='#EEEEEE')

plt.tight_layout()
fig_scatter_path = os.path.join(media_dir, "figures_scatter_raw_vs_ref.png")
plt.savefig(fig_scatter_path, bbox_inches='tight')
plt.close()
print(f"[OK] Saved: {fig_scatter_path}")


# ----------------- FIGURE 2: Correlation Matrix Heatmap (Hình 4.15) -----------------
print("Generating Correlation Matrix Heatmap...")
corr_cols = ['pm2_5', 'pm10', 'temperature', 'humidity', 'pressure', 'reference_pm25', 'reference_pm10']
corr_labels = ['PM2.5 Thô', 'PM10 Thô', 'Nhiệt độ', 'Độ ẩm', 'Áp suất', 'PM2.5 Ref', 'PM10 Ref']
corr_matrix = df[corr_cols].corr().values

fig, ax = plt.subplots(figsize=(8, 7), dpi=150)
im = ax.imshow(corr_matrix, cmap='coolwarm', vmin=-1.0, vmax=1.0)

# Add colorbar
cbar = ax.figure.colorbar(im, ax=ax, fraction=0.046, pad=0.04)
cbar.ax.set_ylabel('Hệ số tương quan', rotation=-90, va="bottom", fontweight='bold')

# Show all ticks and label them
ax.set_xticks(np.arange(len(corr_cols)))
ax.set_yticks(np.arange(len(corr_cols)))
ax.set_xticklabels(corr_labels, rotation=45, ha="right", rotation_mode="anchor", fontweight='bold')
ax.set_yticklabels(corr_labels, fontweight='bold')

# Loop over data dimensions and create text annotations
for i in range(len(corr_cols)):
    for j in range(len(corr_cols)):
        text = ax.text(j, i, f"{corr_matrix[i, j]:.2f}",
                       ha="center", va="center", 
                       color="white" if abs(corr_matrix[i, j]) > 0.55 else "black",
                       fontweight='bold')

ax.set_title("Ma trận tương quan giữa dữ liệu cảm biến và tham chiếu", fontsize=12, fontweight='bold', color='#2C3E50', pad=15)
plt.tight_layout()
fig_corr_path = os.path.join(media_dir, "figures_correlation_matrix.png")
plt.savefig(fig_corr_path, bbox_inches='tight')
plt.close()
print(f"[OK] Saved: {fig_corr_path}")


# ----------------- TRAIN CALIBRATION MODELS FOR EVALUATION -----------------
# Prepare features and targets
features = ['pm2_5', 'temperature', 'humidity', 'pressure']
X = df[features]
y = df['reference_pm25']

# Train/Test split (80/20)
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Model 1: Linear Regression
lr_model = LinearRegression()
lr_model.fit(X_train, y_train)
y_pred_lr = lr_model.predict(X_test)

# Model 2: Random Forest
rf_model = RandomForestRegressor(n_estimators=100, random_state=42)
rf_model.fit(X_train, y_train)
y_pred_rf = rf_model.predict(X_test)

# Evaluate errors
mae_raw = mean_absolute_error(y_test, X_test['pm2_5'])
rmse_raw = np.sqrt(mean_squared_error(y_test, X_test['pm2_5']))
r2_raw = r2_score(y_test, X_test['pm2_5'])

mae_lr = mean_absolute_error(y_test, y_pred_lr)
rmse_lr = np.sqrt(mean_squared_error(y_test, y_pred_lr))
r2_lr = r2_score(y_test, y_pred_lr)

mae_rf = mean_absolute_error(y_test, y_pred_rf)
rmse_rf = np.sqrt(mean_squared_error(y_test, y_pred_rf))
r2_rf = r2_score(y_test, y_pred_rf)

print(f"\nModel Performance Evaluation (Test Set):")
print(f"Raw Sensor      -> MAE: {mae_raw:.2f}, RMSE: {rmse_raw:.2f}, R2: {r2_raw:.2f}")
print(f"Linear Reg      -> MAE: {mae_lr:.2f}, RMSE: {rmse_lr:.2f}, R2: {r2_lr:.2f}")
print(f"Random Forest   -> MAE: {mae_rf:.2f}, RMSE: {rmse_rf:.2f}, R2: {r2_rf:.2f}")


# ----------------- FIGURE 3: Predictions Scatter (Hình 4.16) -----------------
print("\nGenerating Model Predictions Comparison plot...")
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6), dpi=150)

# Linear Regression vs Ref
ax1.scatter(X_test['pm2_5'], y_test, color='#CCCCCC', alpha=0.6, label='Cảm biến thô')
ax1.scatter(X_test['pm2_5'], y_pred_lr, color='#3498DB', alpha=0.8, marker='x', label='Dự đoán Hồi quy tuyến tính')
ax1.plot([y_test.min(), y_test.max()], [y_test.min(), y_test.max()], color='#E74C3C', linestyle='--', linewidth=1.5, label='Đường lý tưởng')
ax1.set_xlabel('Nồng độ PM2.5 thực tế đo (µg/m³)', fontweight='bold')
ax1.set_ylabel('Giá trị dự đoán / tham chiếu (µg/m³)', fontweight='bold')
ax1.set_title(f'Hồi quy tuyến tính (MAE: {mae_lr:.2f}, R²: {r2_lr:.2f})', fontsize=11, fontweight='bold', color='#2C3E50')
ax1.grid(True, linestyle='--', alpha=0.5)
ax1.legend(frameon=True, edgecolor='#EEEEEE')

# Random Forest vs Ref
ax2.scatter(X_test['pm2_5'], y_test, color='#CCCCCC', alpha=0.6, label='Cảm biến thô')
ax2.scatter(X_test['pm2_5'], y_pred_rf, color='#8E44AD', alpha=0.8, marker='o', label='Dự đoán Random Forest')
ax2.plot([y_test.min(), y_test.max()], [y_test.min(), y_test.max()], color='#E74C3C', linestyle='--', linewidth=1.5, label='Đường lý tưởng')
ax2.set_xlabel('Nồng độ PM2.5 thực tế đo (µg/m³)', fontweight='bold')
ax2.set_ylabel('Giá trị dự đoán / tham chiếu (µg/m³)', fontweight='bold')
ax2.set_title(f'Random Forest Regressor (MAE: {mae_rf:.2f}, R²: {r2_rf:.2f})', fontsize=11, fontweight='bold', color='#2C3E50')
ax2.grid(True, linestyle='--', alpha=0.5)
ax2.legend(frameon=True, edgecolor='#EEEEEE')

plt.tight_layout()
fig_pred_path = os.path.join(media_dir, "figures_predictions_comparison.png")
plt.savefig(fig_pred_path, bbox_inches='tight')
plt.close()
print(f"[OK] Saved: {fig_pred_path}")


# ----------------- FIGURE 4: Error Comparison Bar Chart (Hình 4.18) -----------------
print("Generating Error Comparison Bar Chart...")
models = ['Cảm biến thô', 'Hồi quy tuyến tính', 'Random Forest']
maes = [mae_raw, mae_lr, mae_rf]
rmses = [rmse_raw, rmse_lr, rmse_rf]

x_indices = np.arange(len(models))
width = 0.35

fig, ax = plt.subplots(figsize=(9, 5.5), dpi=150)
rects1 = ax.bar(x_indices - width/2, maes, width, label='Sai số MAE', color='#E67E22', alpha=0.85)
rects2 = ax.bar(x_indices + width/2, rmses, width, label='Sai số RMSE', color='#2980B9', alpha=0.85)

ax.set_ylabel('Giá trị sai số (µg/m³)', fontweight='bold')
ax.set_title('So sánh sai số trước và sau khi áp dụng mô hình hiệu chuẩn', fontsize=12, fontweight='bold', color='#2C3E50', pad=15)
ax.set_xticks(x_indices)
ax.set_xticklabels(models, fontweight='bold')
ax.legend(frameon=True, edgecolor='#EEEEEE')
ax.grid(True, axis='y', linestyle='--', alpha=0.5)

# Add values above bars
def autolabel(rects):
    for rect in rects:
        height = rect.get_height()
        ax.annotate(f'{height:.2f}',
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom', fontsize=9, fontweight='bold')

autolabel(rects1)
autolabel(rects2)

plt.tight_layout()
fig_err_path = os.path.join(media_dir, "figures_error_comparison.png")
plt.savefig(fig_err_path, bbox_inches='tight')
plt.close()
print(f"[OK] Saved: {fig_err_path}")


# ----------------- FIGURE 5: Feature Importance Bar Chart (Hình 4.19) -----------------
print("Generating Feature Importance Bar Chart...")
importances = rf_model.feature_importances_
indices = np.argsort(importances)
feature_labels = ['Nhiệt độ (T)', 'Độ ẩm (RH)', 'Áp suất (P)', 'PM2.5 Thô (PM2.5)']
sorted_labels = [feature_labels[i] for i in indices]

fig, ax = plt.subplots(figsize=(9, 4.5), dpi=150)
ax.barh(np.arange(len(importances)), importances[indices], color='#16A085', alpha=0.85, height=0.6)
ax.set_yticks(np.arange(len(importances)))
ax.set_yticklabels(sorted_labels, fontweight='bold')
ax.set_xlabel('Mức độ quan trọng tương đối', fontweight='bold', labelpad=10)
ax.set_title('Mức độ quan trọng của các đặc trưng đầu vào (Random Forest)', fontsize=12, fontweight='bold', color='#2C3E50', pad=15)
ax.grid(True, axis='x', linestyle='--', alpha=0.5)

# Annotate values
for i, v in enumerate(importances[indices]):
    ax.text(v + 0.01, i, f"{v*100:.1f}%", va='center', fontweight='bold', color='#333333')

plt.tight_layout()
fig_feat_path = os.path.join(media_dir, "figures_feature_importance.png")
plt.savefig(fig_feat_path, bbox_inches='tight')
plt.close()
print(f"[OK] Saved: {fig_feat_path}")

print("\n=======================================================")
print("  ADVANCED CALIBRATION & ANALYSIS CHARTS GENERATED!   ")
print("=======================================================")
