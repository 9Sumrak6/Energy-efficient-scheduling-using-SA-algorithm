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
    }
}

abs_opt = defaultdict()
opt = defaultdict()

# Регулярные выражения для парсинга файлов
energy_pattern = re.compile(r'Energy=([+-]?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)')
time_pattern = re.compile(r'Time=([\d\.]+)(µs)?')
iter_pattern = re.compile(r'Iterations=(\d+)')

# Обходим структуру папок
for method in ['mpi/10/', 'fork_mpi/10/']:
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

# Теперь данные структурированы и доступны для анализа
# Пример доступа к данным:
print("MPI_FORK algo:")
print("boltz:", data['fork_mpi/10/']['boltz'])
print("cauchy:", data['fork_mpi/10/']['cauchy'])
print("common:", data['fork_mpi/10/']['common'])
print("MPI algo:")
print("boltz:", data['mpi/10/']['boltz'])
print("cauchy:", data['mpi/10/']['cauchy'])
print("common:", data['mpi/10/']['common'])
# print(data['mpi/10/']['boltz']['10_2'])  # Все запуски для n=10, m=2 в MPI/Boltzman
# print(data['fork_mpi/10/']['cauchy']['1000_100'])  # Все запуски для n=1000, m=100 в Fork-MPI/Cauchy
# print(data['fork_mpi/10/']['boltz']['300_20']['energy'])
# Можно преобразовать в pandas DataFrame для удобного анализа
import matplotlib.pyplot as plt
import numpy as np

def plot_metrics(data, title):
    fig, axes = plt.subplots(3, 1, figsize=(20, 16))
    fig.suptitle(title, fontsize=20, y=0.995)

    models = ['boltz', 'cauchy', 'common']
    colors = ['#4e79a7', '#f28e2b', '#59a14f']
    metrics = ['energy', 'time', 'iterations']
    metric_labels = ['Energy', 'Time (s)', 'Iterations']
    colors = ['yellow', 'red', 'blue']
    # Соберём все task-ключи, где есть хотя бы по одному значению
    all_keys = set()
    for model in models:
        all_keys.update(data[model].keys())
    all_keys = sorted(all_keys, key=lambda s: [int(i) for i in s.split('_')])

    # allowed_m = {3, 10, 20, 80, 160}
    allowed_n = {100, 200, 300, 400, 500}
    all_keys = [key for key in all_keys if int(key.split('_')[0]) in allowed_n]

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
            
            # Добавим подписи (там, где есть значения)
            # for xi, v in enumerate(arr):
            #     if not np.isnan(v):
            #         ax.text(x[xi] + i*bar_width, v + 0.03*np.nanmax(arr), f'{v:.2f}', 
            #                 ha='center', va='bottom', fontsize=8)

        ax.set_xticks(x + bar_width)
        ax.set_xticklabels(all_keys, rotation=45)
        ax.set_xlabel('Task (n_m)', fontsize=12)
        ax.set_ylabel(label, fontsize=12)
        ax.legend(framealpha=0.94)
        ax.grid(axis='y', linestyle='--', alpha=0.7)

    plt.tight_layout(rect=[0, 0, 1, 0.97])
    plt.savefig('cmp.png', dpi=1000, bbox_inches='tight')
    # plt.show()

# plot_metrics(data['fork_mpi/10/'], 'title')