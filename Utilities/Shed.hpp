#pragma once

#include "Graph.hpp"

bool is_equal(double x, double y) {
    return fabs(x - y) < std::numeric_limits<double>::epsilon();
}

class Shed {
    string inp_path;
    int n_procs;

    double alpha = 0.0;
    double max_time;
    double energy;

    int min_job_id = -1;

    vector<double> extra_time;
    vector<vector<int>> rel;
    vector<Proc> procs;
    // map<int, int> proc_last_time;
public:
    Shed(Graph &graph, const string& path = "Input/sys/default.txt");

    Shed(const Shed& other) = default;

    bool is_correct(Graph &graph, map<int, set<int>> &follows);
    
    bool set_random_freq(Graph &graph, int proc_id = -1);
    
    bool move_random_job(Graph &graph, int proc_id = -1);
    void push_back(Graph &graph, int job_id, int proc_id);
    // bool move_random_job(Graph &graph);
    bool switch_jobs(Graph &graph, map<int, set<int>> &follows);

    double calc_energy();

    double get_energy();
    int get_proc_num();
    int get_job_num(Graph &graph);

    vector<int> get_topo(Graph &graph);
    void build_time(Graph &graph);
    void switch_lvl(int j1, int j2, Graph &graph);

    void build(Graph &graph);

    std::string serialize() const;
    void deserialize(const string& data);

    void print(Graph &graph);
};

Shed::Shed(Graph &graph, const string& path) {
    Reader reader;

    inp_path = path;

    pair<int, vector<Proc>> res = reader.readSimpleSys(path, max_time);
    n_procs = res.first;
    procs = res.second;
    extra_time = vector<double>(n_procs);
    rel = vector<vector<int>>(n_procs);

    for (size_t i = 0; i < n_procs; ++i)
        alpha = max(alpha, procs[i].max_freq / procs[i].min_freq);

    alpha = (alpha + 1) * (alpha + 1);

    for (int i = 0; i < n_procs; i++) {
        // proc_last_time[i] = 0;
        extra_time[i] = 0.0;
    }
    int i = 0;
    for (auto j : graph.get_topo()) {
        this->push_back(graph, j, i);
        i = (i + 1) % n_procs;
    }

    energy = 0.0;
}

// Shed::Shed(const Shed& other) {
//     this->n_procs = other.n_procs;
//     this->alpha = other.alpha;
//     this->max_time = other.max_time;
//     this->energy = other.energy;
//     this->min_job_id = 0;
//     this->rel = other.rel;
//     this->procs = other.procs;
//     // this->extra_time = other.extra_time;
//     for (int i = 0; i < n_procs; i++) {
//         // proc_last_time[i] = 0;
//         this->extra_time[i] = 0.0;
//     }
// }


bool Shed::is_correct(Graph &graph, map<int, set<int>> &fols) {
    for (size_t i = 0; i < n_procs; ++i) {
        double prev_time = 0.0, prev_dur = 0.0;
        for (auto &job_id : rel[i]) {
            Job j = graph.get_job(job_id);

            if (prev_time + prev_dur > j.arrive_time) {
                cout << "1) Proc=" << i << " :: " << "Job=" << job_id << endl;
                return false;
            }

            prev_time = j.arrive_time;
            prev_dur = j.cur_time;
        }
    }

    for (auto &[id, fols] : fols) {
        Job j1 = graph.get_job(id);
        ull end_time = j1.arrive_time + j1.cur_time;

        for (auto fol : fols) {
            Job j2 = graph.get_job(fol);
            if (end_time > j2.arrive_time) {
                cout << "2) Job1=" << id << " :: " << "Job2=" << fol << endl;
                return false;
            }
        }
    }


    return true;
}

bool Shed::set_random_freq(Graph &graph, int proc_id) {
    while (proc_id < 0 || rel[proc_id].size() == 0)
        proc_id = rand() % n_procs;

    if (procs[proc_id].cur_freq + procs[proc_id].step > procs[proc_id].max_freq) {
        if (procs[proc_id].cur_freq - procs[proc_id].step < procs[proc_id].min_freq)
            return false;

        procs[proc_id].cur_freq -= procs[proc_id].step;
        procs[proc_id].volt = (procs[proc_id].beg_volt * procs[proc_id].cur_freq) / procs[proc_id].max_freq;
        
        for (auto i : rel[proc_id])
            graph.set_freq(i, procs[proc_id].cur_freq, true);

        return true;
    } else if (procs[proc_id].cur_freq - procs[proc_id].step < procs[proc_id].min_freq) {
        if (procs[proc_id].cur_freq + procs[proc_id].step > procs[proc_id].max_freq)
            return false;

        procs[proc_id].cur_freq += procs[proc_id].step;
        procs[proc_id].volt = (procs[proc_id].beg_volt * procs[proc_id].cur_freq) / procs[proc_id].max_freq;
        
        for (auto i : rel[proc_id])
            graph.set_freq(i, procs[proc_id].cur_freq, true);

        return true;
    }

    int mov = rand() % 2;
    if (mov == 1)
        procs[proc_id].cur_freq += procs[proc_id].step;
    if (mov == 0)
        procs[proc_id].cur_freq -= procs[proc_id].step;

    procs[proc_id].volt = (procs[proc_id].beg_volt * procs[proc_id].cur_freq) / procs[proc_id].max_freq;

    for (auto i : rel[proc_id])
        graph.set_freq(i, procs[proc_id].cur_freq, true);

    return true;
}

bool Shed::move_random_job(Graph &graph, int proc_id) {
    if (n_procs == 1)
        return true;

    // this->print(graph);

    int job_id = rand() % graph.get_job_num();
    Job job = graph.get_job(job_id);

    int proc = proc_id;
    int cur_proc = job.proc_id;

    while (proc < 0 || proc == cur_proc)
        proc = rand() % n_procs;

    // cout << "Move " << job_id << " from " << cur_proc << " to " << proc << endl;

    int lpos = -1;
    for (int k = rel[proc].size() - 1; k >= 0; --k) {
        if (job.lvl < graph.get_job_lvl(rel[proc][k])) {
            graph.set_job_proc_lvl(rel[proc][k], -1, true);
            continue;
        }
        lpos = k;
        break;
    }

    if (lpos == -1 || lpos < rel[proc].size() - 1)
        rel[proc].insert(rel[proc].begin() + lpos + 1, job_id);
    else
        rel[proc].push_back(job_id);

    if (cur_proc >= 0) {
        for (int k = rel[cur_proc].size() - 1; k >= 0; --k) {
            if (job_id != rel[cur_proc][k]) {
                graph.set_job_proc_lvl(rel[cur_proc][k], -2, true);
                continue;
            }
            // cout << "*****************" << endl;
            rel[cur_proc].erase(rel[cur_proc].begin() + k);
            break;
        }
    }

    graph.set_job(job_id, proc, true);
    graph.set_freq(job_id, procs[proc].cur_freq, true);
    graph.set_job_proc_lvl(job_id, lpos + 1, true);

    // this->print(graph);
    return true;
}

/*bool Shed::move_random_job(Graph &graph, int job_id, int proc_id) {
    if (n_procs == 1)
        return true;

    Job job = graph.get_job(job_id);
    int proc = proc_id;

    do {
        proc = rand() % n_procs;
    } while (proc == cur_proc);

    bool fl = true;
    for (int k = rel[proc].size() - 1; k >= 0; --k) {
        int i = rel[proc][k];
        if (job.lvl > graph.get_job(i).lvl) {
            fl = false;

            if (cur_proc >= 0) {
                for (int j = rel[cur_proc].size() - 1; j >= 0; --j) {
                    if (graph.get_job(rel[cur_proc][j]).id == job.id) {
                        rel[cur_proc].erase(rel[cur_proc].begin() + j);
                        break;
                    }

                    graph.set_job_proc_lvl(rel[cur_proc][j], -2, true);
                }
            }
            
            rel[proc].insert(rel[proc].begin() + k + 1, job.id);
            graph.set_job_proc_lvl(job_id, k + 1, true);

            break;
        }

        graph.set_job_proc_lvl(i, -1, true);
    }

    if (fl) {
        if (cur_proc >= 0) {
            for (int j = rel[cur_proc].size() - 1; j >= 0; --j) {
                if (graph.get_job(rel[cur_proc][j]).id == job.id) {
                    rel[cur_proc].erase(rel[cur_proc].begin() + j);
                    break;
                }

                graph.set_job_proc_lvl(rel[cur_proc][j], -2, true);
            }
        }

        graph.set_job_proc_lvl(job_id, 0, true);
        rel[proc].insert(rel[proc].begin(), job.id);
    }

    graph.set_job(job_id, proc);
    graph.set_freq(job_id, procs[proc].cur_freq);

    return true;
}*/

void Shed::push_back(Graph &graph, int job_id, int proc_id) {
    rel[proc_id].push_back(job_id);

    graph.set_job(job_id, proc_id);
    graph.set_job_proc_lvl(job_id, rel[proc_id].size() - 1);
    graph.set_freq(job_id, procs[proc_id].cur_freq);    
}

bool Shed::switch_jobs(Graph &graph, map<int, set<int>> &flws) {
    // Локальная переменная для хранения топологии графа
    vector<int> topo = graph.get_topo();

    for (int i = 0; i < n_procs; ++i) {
        for (int j = 0; j < int(rel[i].size()) - 1; ++j) { // Избегаем лишнего break, цикл заканчивается по условию j+1
            int j1 = rel[i][j];
            int j2 = rel[i][j + 1];

            // Сохраняем работы и их данные
            Job job1 = graph.get_job(j1);
            Job job2 = graph.get_job(j2);

            // Если работа не выполняется (arrive_time == 0), пропускаем её
            if (j == 0 && job1.arrive_time == 0) continue;

            // Проверка, можно ли выполнить switch на основе времени
            if (job1.arrive_time + job1.cur_time < job2.arrive_time) continue;

            // Если работы на одинаковых уровнях и могут быть расположены рядом, пропускаем
            if (j > 0 && graph.get_job(j-1).arrive_time + graph.get_job(j-1).cur_time == job1.arrive_time) continue;

            bool conflict_found = false;

            // cout << "HERE1" << endl;
            // Проверка возможных конфликтов на уровне
            for (int i = job1.lvl; i <= job2.lvl; ++i) {
                bool fl1 = (i != job1.lvl && flws[j1].find(topo[i]) != flws[j1].end());
                bool fl2 = (i != job2.lvl && flws[topo[i]].find(j2) != flws[topo[i]].end());
                
                if (fl1 || fl2) {
                    conflict_found = true;
                    break;
                }
            }
            // cout << "HERE2" << endl;

            // Если есть конфликт, продолжаем с следующими работами
            if (conflict_found) continue;

            // Выполняем switch
            // cout << "HERE3" << endl;
            graph.switch_lvl(j1, j2);
            std::swap(rel[i][j], rel[i][j + 1]);

            // min_job_id = min(rel[i][j], rel[i][j + 1]);

            return true; // Возвращаем true, так как обмен состоялся
        }
    }

    return false; // Возвращаем false, если обмен не был выполнен
}


void Shed::switch_lvl(int j1, int j2, Graph &graph) {
    // Переключаем уровни в графе
    graph.switch_lvl(j1, j2);
    
    // Получаем информацию о работах
    Job job1 = graph.get_job(j1), job2 = graph.get_job(j2);
    int i = job1.proc_id;
    int i1 = job2.proc_id;

    // Если работы принадлежат разным процессорам или хотя бы одна не имеет процессора, выходим
    if (i != i1 || i < 0 || i1 < 0)
        return;

    // Получаем уровни работ
    int lvl1 = job1.proc_lvl, lvl2 = job2.proc_lvl;

    // Если уровни разные, меняем их местами в rel[i]
    if (lvl1 != lvl2) {
        auto& rel_proc = rel[i];
        
        // Находим позиции для j1 и j2
        auto it1 = std::find(rel_proc.begin(), rel_proc.end(), j1);
        auto it2 = std::find(rel_proc.begin(), rel_proc.end(), j2);

        // Если оба элемента найдены
        if (it1 != rel_proc.end() && it2 != rel_proc.end()) {
            // Удаляем работы из их старых позиций
            std::swap(*it1, *it2);  // Просто меняем местами элементы

            // Вставляем работы в новые позиции
            // Поддерживаем правильный порядок
            if (lvl1 < lvl2) {
                rel_proc.insert(rel_proc.begin() + lvl2, j1);
                rel_proc.insert(rel_proc.begin() + lvl1, j2);
            } else {
                rel_proc.insert(rel_proc.begin() + lvl1, j2);
                rel_proc.insert(rel_proc.begin() + lvl2, j1);
            }
        }
    }
}

double Shed::calc_energy() {
    double energy = 0.0;
    for (size_t id = 0; id < n_procs; ++id) {
        Proc proc = procs[id];

        energy += proc.volt * proc.cur_freq * proc.cur_freq * (proc.job_time + alpha * extra_time[id] + (proc.max_time - proc.job_time) / 2);
    }

    energy *= 0.5 * procs[0].cap;

    this->energy = energy;
    return energy;
}

double Shed::get_energy() {
    return energy;
}

int Shed::get_proc_num() {
    return n_procs;
}

int Shed::get_job_num(Graph &graph) {
    return graph.get_job_num();
}

void Shed::build_time(Graph &graph) {
    if (min_job_id < 0) {
        for (size_t id = 0; id < n_procs; ++id) {
            procs[id].max_time = 0;
            procs[id].job_time = 0;
            extra_time[id] = 0.0;
        }
        for (auto &i : graph.get_topo())
            graph.set_prev_time(i, 0, 1);

        for (auto &i : graph.get_topo()) {
            Job j = graph.get_job(i);

            ull time = graph.set_arr_time(i, max(procs[j.proc_id].max_time, j.prev_time));

            procs[j.proc_id].max_time = time + j.cur_time;
            procs[j.proc_id].job_time += j.cur_time;

            if (time >= this->max_time)
                extra_time[j.proc_id] += j.cur_time;
            else if (time + j.cur_time > this->max_time)
                extra_time[j.proc_id] += time + j.cur_time - this->max_time;

            for (auto &fl : graph[i])
                graph.set_prev_time(fl, procs[j.proc_id].max_time);
        }
    } else {
        Job j = graph.get_job(min_job_id);
        ull beg_time = 0;

        if (rel[j.proc_id][0] != min_job_id) {
            Job tmp = graph.get_job(rel[j.proc_id][j.proc_lvl - 1]);
            beg_time = tmp.beg_time + tmp.cur_time;
        }

        for (const auto &prev : graph.get_prevs(min_job_id)) {
            Job tmp = graph.get_job(prev);
            beg_time = max(beg_time, tmp.beg_time + tmp.cur_time);
        }

        graph.set_arr_time(min_job_id, beg_time);

        vector<int> topo = graph.get_topo();
        for (int lvl = j.lvl; lvl < graph.get_job_num(); ++lvl) {
            int cur = topo[lvl];
            if (!graph.is_changed(cur))
                continue;

            for (auto &fl : graph[cur])
                graph.set_prev_time(fl, procs[j.proc_id].max_time);

            graph.set_unchanged(cur);
        }
    }
}

vector<int> Shed::get_topo(Graph &graph) {
    return graph.get_topo();
}

void Shed::build(Graph &graph) {
    // auto start = std::chrono::high_resolution_clock::now();
    
    build_time(graph);
    
    // Замер времени окончания и расчёт продолжительности
    // auto end = std::chrono::high_resolution_clock::now();
    // auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Вывод времени выполнения (можно заменить на логирование)
    // std::cout << "build_time() executed in: " << duration.count() << " microseconds" << std::endl;

    calc_energy();
    min_job_id = -1;
}

std::string Shed::serialize() const {
    std::ostringstream oss;
    
    // Сериализация основных полей
    oss << max_time << " " << energy << " " << min_job_id << " ";

    // Сериализация extra_time
    oss << extra_time.size() << " ";
    for (const auto& et : extra_time) {
        oss << et << " ";
    }

    // Сериализация rel (вектор векторов)
    oss << rel.size() << " ";
    for (const auto& inner_vec : rel) {
        oss << inner_vec.size() << " ";
        for (int val : inner_vec) {
            oss << val << " ";
        }
    }

    // Сериализация procs
    oss << procs.size() << " ";
    for (const auto& proc : procs) {
        oss << proc.serialize() << " ";  // Убираем \n для единообразия формата
    }

    return oss.str();
}

void Shed::deserialize(const std::string& data) {
    std::istringstream iss(data);
    
    // Десериализация основных полей
    iss >> max_time >> energy >> min_job_id;

    // Десериализация extra_time
    size_t et_size;
    iss >> et_size;
    extra_time.resize(et_size);
    for (auto& et : extra_time) {
        iss >> et;
    }

    // Десериализация rel
    size_t rel_size;
    iss >> rel_size;
    rel.resize(rel_size);
    for (auto& inner_vec : rel) {
        size_t inner_size;
        iss >> inner_size;
        inner_vec.resize(inner_size);
        for (auto& val : inner_vec) {
            iss >> val;
        }
    }

    // Десериализация procs
    size_t procs_size;
    iss >> procs_size;
    procs.resize(procs_size);
    
    // Очистка потока перед чтением объектов Proc
    iss.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
    
    for (auto& proc : procs) {
        std::string proc_data;
        std::getline(iss, proc_data, ' ');  // Читаем до следующего пробела
        proc.deserialize(proc_data);
    }
}

void Shed::print(Graph &graph) {
    graph.print();

    cout << "----------------------------" << endl;

    for (size_t id = 0; id < n_procs; ++id) {
        cout << "Processor " << id << "(volt=" << procs[id].volt << "; freq=" << procs[id].cur_freq << "; job_t=" << procs[id].job_time << "):: ";

        for (auto job_id : rel[id]) {
            Job j = graph.get_job(job_id);
            cout << "'id=" << j.id << " arrive_time=" << j.arrive_time << " dur=" << j.cur_time << "'    ";
        }

        cout << endl;
    }
    cout << endl << "Result energy consumption = " << energy << endl;
    cout << "----------------------------" << endl;
}