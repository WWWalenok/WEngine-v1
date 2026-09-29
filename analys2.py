import json
import re
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import os
from collections import defaultdict
from datetime import datetime

# Настройки
LOG_FILE = "bin_windows_Debug/prof.log"  # Путь к лог-файлу
UPDATE_INTERVAL = 2000          # Интервал обновления графиков (мс)
TOP_FUNCTIONS = 10              # Сколько функций отображать (по последнему OwnAvg)
DEBUG_PRINT = False             # Печатать ли сырые JSON-строки

class ProfilerData:
    def __init__(self):
        self.timestamps = []                     # Время (timestamp в секундах)
        self.function_series = defaultdict(lambda: {'OwnAvg': [], 'Total_calls': []})
        self.last_data = []                      # Последний замер (список словарей)
        self.last_position = 0                   # Позиция в файле
        self.function_colors = {}                # Цвета для функций
        self._all_function_names = set()         # Все когда-либо встречавшиеся имена
        self._last_time_seconds = None           # Последнее распарсенное время (секунды от полуночи)
        self._time_offset = 0.0                  # Смещение для обработки перехода через полночь

    def _parse_time(self, time_str):
        """Преобразует строку времени 'HH:MM:SS.ffffff' в секунды от полуночи."""
        try:
            dt = datetime.strptime(time_str, "%H:%M:%S.%f")
        except ValueError:
            # Если нет микросекунд
            dt = datetime.strptime(time_str, "%H:%M:%S")
        # Секунды от полуночи
        seconds = dt.hour * 3600 + dt.minute * 60 + dt.second + dt.microsecond / 1_000_000
        return seconds

    def read_new_data(self):
        """Читает новые строки из лог-файла и возвращает список (timestamp, entry)."""
        if not os.path.exists(LOG_FILE):
            return []

        new_entries = []
        try:
            with open(LOG_FILE, 'r', encoding='utf-8') as f:
                # Если файл был обрезан или мы впервые читаем, начинаем с начала
                f.seek(0, os.SEEK_END)
                file_size = f.tell()
                if self.last_position > file_size:
                    self.last_position = 0

                f.seek(self.last_position)

                for line in f:
                    if "PROFILER:" in line:
                        timestamp_match = re.search(r'\[(.*?)\]', line)
                        if timestamp_match:
                            time_str = timestamp_match.group(1)  # "22:09:26.956657"
                            try:
                                current_seconds = self._parse_time(time_str)
                            except ValueError:
                                # Если время не удалось распарсить, пропускаем строку
                                continue

                            # Обработка перехода через полночь
                            if self._last_time_seconds is not None and current_seconds < self._last_time_seconds:
                                self._time_offset += 24 * 3600  # добавляем сутки
                            self._last_time_seconds = current_seconds
                            timestamp = self._time_offset + current_seconds
                        else:
                            timestamp = time.time()  # fallback (на всякий случай)

                        json_start = line.find('statistic:[')
                        if json_start != -1:
                            json_str = line[json_start + 10:]
                            if DEBUG_PRINT:
                                print(json_str)
                            try:
                                data = json.loads(json_str)
                                new_entries.append((timestamp, data))
                            except json.JSONDecodeError:
                                continue

                self.last_position = f.tell()
        except Exception as e:
            print(f"Ошибка чтения файла: {e}")

        return new_entries

    def update_data(self):
        """Обновляет внутренние структуры данных новыми записями."""
        new_entries = self.read_new_data()
        if not new_entries:
            return False

        for timestamp, entry in new_entries:
            self.timestamps.append(timestamp)
            self.last_data = entry

            current_names = {item['Name'] for item in entry}
            self._all_function_names.update(current_names)

            for name in current_names:
                if name not in self.function_colors:
                    self.function_colors[name] = f'#{os.urandom(3).hex()}'

            # Обновляем ряды для всех известных функций
            for name in self._all_function_names:
                if name in current_names:
                    func_data = next(item for item in entry if item['Name'] == name)
                    self.function_series[name]['OwnAvg'].append(func_data['OwnAvg'])
                    self.function_series[name]['Total_calls'].append(func_data['Total_calls'])
                else:
                    self.function_series[name]['OwnAvg'].append(None)
                    self.function_series[name]['Total_calls'].append(None)

        return True

# Инициализация данных
profiler_data = ProfilerData()

# Создание фигуры с двумя подграфиками
fig, (ax_avg, ax_total) = plt.subplots(2, 1, figsize=(12, 10))
fig.subplots_adjust(hspace=0.4)
fig.canvas.manager.set_window_title('Profiler Time Series Analyzer')

def update_graphs(frame):
    """Функция обновления графиков, вызываемая анимацией."""
    data_updated = profiler_data.update_data()
    if not data_updated or not profiler_data.last_data:
        return

    # Выбираем топ функций по последнему OwnAvg
    sorted_last = sorted(profiler_data.last_data, key=lambda x: x['OwnAvg'], reverse=True)
    top_functions = [item['Name'] for item in sorted_last[:TOP_FUNCTIONS]]

    # Очищаем оси
    ax_avg.clear()
    ax_total.clear()

    # Рисуем линии для каждой выбранной функции
    for name in top_functions:
        color = profiler_data.function_colors.get(name, '#000000')
        series = profiler_data.function_series[name]

        ax_avg.plot(profiler_data.timestamps, series['OwnAvg'],
                    label=name, color=color, marker='.', markersize=4, linewidth=1.5)
        ax_total.plot(profiler_data.timestamps, series['Total_calls'],
                      label=name, color=color, marker='.', markersize=4, linewidth=1.5)

    # Настройка верхнего графика
    ax_avg.set_ylabel('OwnAvg (ms)', fontsize=10)
    ax_avg.set_title('Среднее время выполнения функций (по времени)', fontsize=12)
    ax_avg.grid(True, linestyle='--', alpha=0.6)
    ax_avg.legend(loc='upper left', fontsize=8, ncol=2)

    # Настройка нижнего графика
    ax_total.set_ylabel('Total calls', fontsize=10)
    ax_total.set_xlabel('Время (сек от старта)', fontsize=10)
    ax_total.set_title('Общее количество вызовов функций (по времени)', fontsize=12)
    ax_total.grid(True, linestyle='--', alpha=0.6)
    ax_total.legend(loc='upper left', fontsize=8, ncol=2)

    # Поворачиваем подписи оси X для удобства
    for ax in (ax_avg, ax_total):
        plt.setp(ax.get_xticklabels(), rotation=45, ha='right', fontsize=8)

# Запуск анимации
ani = FuncAnimation(
    fig,
    update_graphs,
    interval=UPDATE_INTERVAL,
    cache_frame_data=False
)

plt.tight_layout()
plt.show()