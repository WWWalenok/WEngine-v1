import json
import re
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import time
import os
from collections import defaultdict

# Настройки
LOG_FILE = "bin_windows_Release/prof.log"  # Укажите путь к вашему лог-файлу
UPDATE_INTERVAL = 2000  # Интервал обновления графика в мс
MAX_HISTORY_POINTS = 30  # Максимальное количество хранимых замеров
TOP_FUNCTIONS = 10      # Количество отображаемых функций

class ProfilerData:
    def __init__(self):
        self.timestamps = []
        self.data_history = []
        self.last_position = 0
        self.function_colors = {}
    
    def read_new_data(self):
        if not os.path.exists(LOG_FILE):
            return []
        
        new_entries = []
        try:
            with open(LOG_FILE, 'r', encoding='utf-8') as f:
                # Переместиться к последней обработанной позиции
                f.seek(self.last_position)
                
                # Читать новые строки
                for line in f:
                    if "PROFILER:" in line:
                        timestamp_match = re.search(r'\[(.*?)\]', line)
                        timestamp_s = timestamp_match.group(1) if timestamp_match else ""
                        timestamp_spl1 = timestamp_s.split('.')
                        timestamp_spl2 = timestamp_spl1[0].split(':')
                        timestamp = float(timestamp_spl2[2] + '.' + timestamp_spl1[1]) + (float(timestamp_spl2[1]) + float(timestamp_spl2[0]) * 60) * 60
                        # Извлечь JSON данные
                        json_start = line.find('statistic:[')
                        if json_start != -1:
                            json_str = line[json_start + 10:]
                            print(json_str)
                            try:
                                data = json.loads(json_str)
                                new_entries.append((timestamp, data))
                            except json.JSONDecodeError:
                                continue
                
                # Сохранить текущую позицию
                self.last_position = f.tell()
        
        except Exception as e:
            print(f"Ошибка чтения файла: {e}")
        
        return new_entries

    def update_data(self):
        new_entries = self.read_new_data()
        if not new_entries:
            return False
        
        for timestamp, entry in new_entries:
            self.timestamps.append(timestamp)
            self.data_history.append(entry)
            
            # Обновить цвета для новых функций
            for item in entry:
                if item['Name'] not in self.function_colors:
                    self.function_colors[item['Name']] = f'#{os.urandom(3).hex()}'
        
        # Сохранять только последние MAX_HISTORY_POINTS замеров
        if len(self.data_history) > MAX_HISTORY_POINTS:
            self.data_history = self.data_history[-MAX_HISTORY_POINTS:]
            self.timestamps = self.timestamps[-MAX_HISTORY_POINTS:]
        
        return True

# Инициализация данных
profiler_data = ProfilerData()

# Создание фигуры
ax_avg :plt.Axes
ax_total :plt.Axes
fig, (ax_avg, ax_total) = plt.subplots(2, 1, figsize=(12, 10))
fig.subplots_adjust(hspace=0.4)
fig.canvas.manager.set_window_title('Profiler Data Analyzer')

# Функция для обновления графиков
def update_graphs(frame):
    data_updated = profiler_data.update_data()
    if not data_updated or not profiler_data.data_history:
        return
    
    # Используем последние данные
    current_data = profiler_data.data_history[-1]
    
    # Сортировка функций по OwnAvg для отображения
    sorted_data = sorted(current_data, key=lambda x: x['OwnAvg'], reverse=True)
    
    # Очистка предыдущих графиков
    ax_avg.clear()
    ax_total.clear()
    
    # Подготовка данных для графиков
    function_names = [item['Name'] for item in sorted_data]
    own_avgs = [item['OwnAvg'] for item in sorted_data]
    totals = [item['Total_calls'] for item in sorted_data]
    colors = [profiler_data.function_colors[name] for name in function_names]
    
    # График среднего времени
    bar_container = ax_avg.bar(function_names, own_avgs, color=colors)
    ax_avg.tick_params(axis='x', rotation=0, labelsize=8, direction='in')
    ax_avg.grid(True, linestyle='--', alpha=0.6)
    
    # Добавление значений на столбцы
    for i, v in enumerate(own_avgs):
        ax_avg.text(i, v + 0.01 * max(own_avgs), f'{v:.4f}', 
                   ha='center', fontsize=8)
    
    # График общего времени
    bar_container = ax_total.bar(function_names, totals, color=colors)
    ax_total.tick_params(axis='x', rotation=0, labelsize=8)
    ax_total.grid(True, linestyle='--', alpha=0.6)
    
    # Добавление значений на столбцы
    for i, v in enumerate(totals):
        ax_total.text(i, v + 0.01 * max(totals), f'{v:.2f}', 
                     ha='center', fontsize=8)

# Запуск анимации
ani = FuncAnimation(
    fig,
    update_graphs,
    interval=UPDATE_INTERVAL,
    cache_frame_data=False
)

plt.tight_layout()
plt.show()