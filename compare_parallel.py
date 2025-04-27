def avg_metric(data, metric, keys=None):
    vals = []
    for model in data:   # boltz, cauchy, common
        for k,v in data[model].items():
            if keys is not None and k not in keys:
                continue
            vals.append(v[metric])
    return sum(vals)/len(vals), len(vals)

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
                    opt = float(f.readline())

                # Парсим данные
                energy = float(energy_pattern.search(content).group(1)) / opt
                
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

fork = data['fork_mpi/10/']
mpi = data['mpi/10/']

e_fork, n_fork = avg_metric(fork, 'energy')
e_mpi, n_mpi = avg_metric(mpi, 'energy')

print('MPI_FORK: среднее energy', e_fork, 'по', n_fork, 'экспериментам')
print('MPI:     среднее energy', e_mpi, 'по', n_mpi, 'экспериментам')
