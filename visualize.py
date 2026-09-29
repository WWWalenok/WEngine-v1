import sys
import numpy as np
import matplotlib.pyplot as plt

def load_bin(filename, dtype=np.float32):
    return np.fromfile(filename, dtype=dtype)

# Проверяем аргументы командной строки
if len(sys.argv) < 2:
    print("Usage: python visualize.py <iteration>")
    print("Example: python visualize.py 0100")
    sys.exit(1)

iteration = sys.argv[1]   # например "0100"

# Загрузка размеров сетки
wh = np.fromfile("output/dims.bin", dtype=np.int32)
W, H = wh[0], wh[1]

# Загрузка данных для указанной итерации
height = load_bin(f"output/height_{iteration}.bin").reshape(H, W)
water  = load_bin(f"output/water_{iteration}.bin").reshape(H, W)

# Загрузка статичных полей (если нужны)
try:
    plate = load_bin("output/plate.bin", dtype=np.uint8).reshape(H, W)
    tectonic_up = load_bin("output/tectonic_up.bin").reshape(H, W)
    wind_u = load_bin("output/wind_u.bin").reshape(H, W)
    wind_v = load_bin("output/wind_v.bin").reshape(H, W)
    moisture = load_bin("output/moisture.bin").reshape(H, W)
    static_available = True
except FileNotFoundError:
    static_available = False

# Создаём фигуру с двумя основными панелями (высота и вода)
fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
fig.suptitle(f"Iteration {iteration}")

# Высота – серый градиент с указанием min/max
hmin, hmax = height.min(), height.max()
im1 = ax1.imshow(height, origin='lower', cmap='gray', vmin=hmin, vmax=hmax)
ax1.set_title(f'Height (min={hmin:.1f}, max={hmax:.1f})')
cbar1 = plt.colorbar(im1, ax=ax1, label='Height (m)')

# Вода – синий градиент с фиксированным диапазоном или динамическим
wmin, wmax = 0, water.max()  # минимум 0 для воды
im2 = ax2.imshow(water, origin='lower', cmap='Blues', vmin=wmin, vmax=wmax)
ax2.set_title(f'Water depth (max={wmax:.2f})')
cbar2 = plt.colorbar(im2, ax=ax2, label='Water depth (m)')

# Если доступны статические поля, можно добавить ещё одну фигуру для ветра/влажности
if static_available:
    fig2, axs2 = plt.subplots(2, 2, figsize=(10, 8))
    fig2.suptitle(f"Static fields (iteration {iteration})")

    # Плиты
    axs2[0,0].imshow(plate, origin='lower', cmap='tab10', interpolation='nearest')
    axs2[0,0].set_title('Plates')

    # Тектоника
    im_tect = axs2[0,1].imshow(tectonic_up, origin='lower', cmap='RdBu_r')
    axs2[0,1].set_title('Tectonic uplift')
    plt.colorbar(im_tect, ax=axs2[0,1])

    # Ветер magnitude
    wind_mag = np.sqrt(wind_u**2 + wind_v**2)
    im_wind = axs2[1,0].imshow(wind_mag, origin='lower', cmap='viridis')
    axs2[1,0].set_title('Wind speed')
    plt.colorbar(im_wind, ax=axs2[1,0])

    # Влажность
    im_moist = axs2[1,1].imshow(moisture, origin='lower', cmap='Blues', vmin=0, vmax=1)
    axs2[1,1].set_title('Moisture')
    plt.colorbar(im_moist, ax=axs2[1,1])

    fig2.tight_layout()
    fig2.savefig(f"output/static_{iteration}.png", dpi=150)

plt.tight_layout()
plt.savefig(f"output/terrain_{iteration}.png", dpi=150)
plt.show()