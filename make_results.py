import os
import re
from collections import defaultdict

# Путь к корневой папке
root_path = 'Output/'
opt_path = 'Input/opt/'

# Структуры для хранения данных
data = {
    'mpi/10/': {
        'boltz': defaultdict(),
        'cauchy': defaultdict(),
        'common': defaultdict()
    },
    'fork_mpi/10/': {
        'boltz': defaultdict(),
        'cauchy': defaultdict(),
        'common': defaultdict()
    },
    'consecutive/': {
        'boltz': defaultdict(),
        'cauchy': defaultdict(),
        'common': defaultdict()
    }
}

abs_opt = defaultdict()
opt = defaultdict()

# Регулярные выражения для парсинга файлов
energy_pattern = re.compile(r'Energy=([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)')
time_pattern = re.compile(r'Time=([\d\.]+)(µs)?')
iter_pattern = re.compile(r'Iterations=(\d+)')

# Обходим структуру папок
for method in ['mpi/10/', 'fork_mpi/10/', 'consecutive/']:
    for model in ['boltz/', 'cauchy/', 'common/']:
        current_path = os.path.join(root_path, method, model)
        
        # Проверяем существование пути
        if not os.path.exists(current_path):
            continue
            
        # Перебираем все файлы в папке
        for filename in os.listdir(current_path):
            if filename.endswith('.txt'):
                # Извлекаем n и m из имени файла
                try:
                    n, m = map(int, filename[:-4].split('_'))
                except:
                    continue  # Пропускаем файлы с неправильным форматом имени
                
                # Читаем содержимое файла
                with open(os.path.join(current_path, filename), 'r') as f:
                    content = f.read()

                with open(os.path.join(opt_path, filename), 'r') as f:
                    abs_opt[filename] = float(f.readline())
                    opt[filename] = float(f.readline())

                # Парсим данные
                energy = float(energy_pattern.search(content).group(1)) / opt[filename]
                
                time_match = time_pattern.search(content)
                time = float(time_match.group(1))
                if time_match.group(2):  # Если есть 'µs'
                    time /= 1000000  # Переводим в секунды
                
                iterations = int(iter_pattern.search(content).group(1))
                
                # Сохраняем данные
                key = f"{n}_{m}"
                data[method][model[:-1]][key] = {
                    'energy': energy,
                    'time': time,
                    'iterations': iterations,
                    'filename': filename
                }

import matplotlib.pyplot as plt
import numpy as np

def plot_metrics(data, allowed_n, filename, method):
    fig, axes = plt.subplots(3, 1, figsize=(20, 16))
    fig.suptitle(f"Сomparison of criteria  ({method})", fontsize=25, y=0.995)

    models = ['boltz', 'cauchy', 'common']
    metrics = ['energy', 'time', 'iterations']
    metric_labels = ['E_res / E_opt', 'Time (s)', 'Iterations']
    colors = ['red', 'green', "blue"]
    # Соберём все task-ключи, где есть хотя бы по одному значению
    all_keys = set()
    for model in models:
        all_keys.update(data[model].keys())

    all_keys = sorted([key for key in all_keys if int(key.split('_')[0]) in allowed_n])

    x = np.arange(len(all_keys))
    bar_width = 0.22

    for ax_idx, (metric, label) in enumerate(zip(metrics, metric_labels)):
        ax = axes[ax_idx]

        # Построим значения для каждого model
        plot_data = []
        for model in models:
            model_values = []
            for k in all_keys:
                # Безопасная проверка и np.nan, если нет значения
                v = data[model].get(k, {}).get(metric, np.nan)
                model_values.append(v)
            plot_data.append(model_values)
        
        # Рисуем столбцы — bar всегда должен быть одного размера с x
        for i, model in enumerate(models):
            arr = np.array(plot_data[i], dtype=float)
            # Для красоты, если NaN — покажет дырки
            ax.bar(x + i*bar_width, arr, width=bar_width,
                   color=colors[i], label=model.capitalize(), edgecolor=colors[i], alpha=0.87)

        ax.set_xticks(x + bar_width)
        ax.set_xticklabels(all_keys, fontsize=14, rotation=45)
        ax.set_xlabel('Task (n_m)', fontsize=16)
        ax.set_ylabel(label, fontsize=16)
        ax.legend(framealpha=0.94, fontsize=12)
        ax.set_title(metric, fontsize=15)
        ax.grid(axis='y', linestyle='--', alpha=0.7)

    plt.tight_layout(rect=[0, 0, 1, 0.97])
    plt.savefig(filename, dpi=1500, bbox_inches='tight')

plot_metrics(data['fork_mpi/10/'], {10, 20, 30, 40, 50}, "Graphics/small/fork.png", "fork")
plot_metrics(data['fork_mpi/10/'], {100, 200, 300, 400, 500}, "Graphics/middle/fork.png", "fork")
plot_metrics(data['fork_mpi/10/'], {1000, 2000, 3000, 4000}, "Graphics/hard/fork.png", "fork")

plot_metrics(data['mpi/10/'], {10, 20, 30, 40, 50}, "Graphics/small/mpi.png", "mpi")
plot_metrics(data['mpi/10/'], {100, 200, 300, 400, 500}, "Graphics/middle/mpi.png", "mpi")
plot_metrics(data['mpi/10/'], {1000, 2000, 3000, 4000}, "Graphics/hard/mpi.png", "mpi")

plot_metrics(data['consecutive/'], {10, 20, 30, 40, 50}, "Graphics/small/cons.png", "consecutive")
plot_metrics(data['consecutive/'], {100, 200, 300, 400, 500}, "Graphics/middle/cons.png", "consecutive")
plot_metrics(data['consecutive/'], {1000, 2000, 3000, 4000}, "Graphics/hard/cons.png", "consecutive")
