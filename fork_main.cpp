#include <functional>
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Utilities/Graph.hpp"
#include "Utilities/Shed.hpp"

char TEMP_LAW = 1;

double boltz(int i) {
    return INIT_TEMP / log(i + 1);
}

double cauchy(int i) {
    return INIT_TEMP / (i + 1);
}

double law(int i) {
    return INIT_TEMP * log(i + 1) / (i + 1);
}

void write_bytes(int fd, const void* ptr, size_t n) {
    size_t total = 0; const char* cptr = (const char*)ptr;
    while (total < n) {
        ssize_t w = write(fd, cptr+total, n-total);
        if (w <= 0)
            exit(21);
        total+=w;
    }
}
void read_bytes(int fd, void* ptr, size_t n) {
    size_t total = 0; char* cptr = (char*)ptr;
    while (total < n) {
        ssize_t r = read(fd, cptr + total, n - total);
        if (r <= 0)
            exit(22);
        total += r;
    }
}
void write_double(int fd, double d) { write_bytes(fd, &d, sizeof(d)); }
double read_double(int fd) { double d; read_bytes(fd, &d, sizeof(d)); return d; }
void write_char(int fd, char c) { write_bytes(fd, &c, 1); }
char read_char(int fd) { char c; read_bytes(fd, &c, 1); return c; }

// Генерация соседа
std::pair<bool, std::unique_ptr<Shed>> generate_neighbor(const std::unique_ptr<Shed>& shed, Graph& graph, std::map<int, std::set<int>>& flws, std::mt19937& rng) {
    auto new_shed = std::make_unique<Shed>(*shed);
    bool success = true;
    std::uniform_int_distribution<int> action_dist(0, 2);
    int action = action_dist(rng);

    switch (action) {
        case 0:
            success = new_shed->move_random_job(graph);
            break;
        case 1:
            success = new_shed->switch_jobs(graph, flws);
            break;
        case 2:
            success = new_shed->set_random_freq(graph);
            break;
    }

    return {success, std::move(new_shed)};
}

// Функция симулированного отжига
void sa_shed(
    std::unique_ptr<Shed>& shed,
    Graph& graph,
    Graph& best_graph,
    std::map<int, std::set<int>>& flws,
    double& best,
    std::function<double(int)> temp_func,
    int cmd_fd,
    int data_fd,
    std::string filename) 
{
    std::random_device rd;
    std::mt19937 rng(rd());

    auto cur_shed = std::make_unique<Shed>(*shed);
    int iteration = 0;
    double temperature = INIT_TEMP;

    while (true) {
        int non_progress = 0;
        for (int i = 1; i < PARALLEL_STEP_NUM; ++i) {
            auto [success, new_shed] = generate_neighbor(cur_shed, graph, flws, rng);
            if (!success) {
                ++non_progress;
                continue;
            }
            new_shed->build(graph);

            if (!new_shed->is_correct(graph, flws)) {
                std::runtime_error("Incorrect scheduling on iteration=" + std::to_string(i));
            	return;
            }

            double cur_energy = cur_shed->get_energy();
            double new_energy = new_shed->get_energy();

            std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
            if (FIRST_TRY && cur_energy > new_energy) {
                if (TEMP_LAW == 0) {
                    INIT_TEMP = BOLTZ_COEF * (cur_energy - new_energy);
                } else if (TEMP_LAW == 1) {
                    INIT_TEMP = CAUCHY_COEF * (cur_energy - new_energy);
                } else {
                    INIT_TEMP = COMMON_COEF * (cur_energy - new_energy);
                }

                FIRST_TRY = false;
                temperature = INIT_TEMP;
            }
            if (new_energy < cur_energy || prob_dist(rng) < exp((cur_energy - new_energy) / temperature)) {
                graph.erase_ch();
                cur_shed = std::move(new_shed);

                if (new_energy < best) {
                    *shed = *cur_shed;
                    best_graph = graph;
                    best = new_energy;
                    non_progress = 0;
            }
            } else {
                graph.backup();
                ++non_progress;
            }

            
            if (i % 100 == 0) {
                write_double(data_fd, best);
                char action = read_char(cmd_fd);

                // обновление графа и Sheds
                if (action == 2) { // конец алгоритма
                    return;
                } else if (action == 1) { // получить лучшее решение
                    char action = read_char(cmd_fd);

                    std::ifstream fin(filename, std::ios::binary);
                    uint32_t gsz, ssz;
                    fin.read((char*)&gsz, sizeof(gsz));
                    std::string gdat(gsz, '\0'); fin.read(&gdat[0], gsz);

                    fin.read((char*)&ssz, sizeof(ssz));
                    std::string sdat(ssz, '\0'); fin.read(&sdat[0], ssz);

                    best_graph.deserialize(gdat);
                    graph.deserialize(gdat);
                    shed->deserialize(sdat);
                    cur_shed->deserialize(sdat);
                    fin.close();
                } else if (action == 0) { // отослать лучшее решение
                    std::string gdat = best_graph.serialize();
                    std::string sdat = shed->serialize();
                    uint32_t gsz = gdat.size();
                    uint32_t ssz = sdat.size();

                    std::ofstream fout(filename, std::ios::binary);
                    fout.write((char*)&gsz, sizeof(gsz));
                    fout.write(gdat.data(), gsz);
                    fout.write((char*)&ssz, sizeof(ssz));
                    fout.write(sdat.data(), ssz);
                    fout.close();

                    write_char(data_fd, 1);
                }

                // cout << "OK" << endl;
            }
        }
        temperature = temp_func(++iteration);
    }
}

int main(int argc, char* argv[]) {
    int proc_num = 0;
    std::cout << "Enter the number of workers except coordinator:" << endl;
    std::cin >> proc_num;

    if (proc_num <= 0)
        return 1;
    
    std::vector<int> _jobs = {10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 2000, 3000, 4000};
    std::vector<int> _procs = {2, 3, 4, 5, 10, 20, 40, 60, 80, 100, 120, 140, 160};
    std::vector<std::function<double(int)>> temp_funcitons{boltz, cauchy, law};

    struct WPipe { int to_w[2], from_w[2]; };
    std::vector<WPipe> pipes(proc_num);
    std::vector<pid_t> children(proc_num);

    for(int i=0;i<proc_num;++i) {
        assert(pipe(pipes[i].to_w)==0); // coord->worker
        assert(pipe(pipes[i].from_w)==0); // worker->coord (энергия)
    }

    for(int w = 0; w < proc_num; ++w) {
        pid_t pid = fork();
        if(pid == 0) {
            // worker: закроет лишние концы, запустит sa_shed в цикле для всех задач
            for(int x=0; x<proc_num;++x) {
                if(x!=w) {
                    close(pipes[x].to_w[0]); close(pipes[x].to_w[1]);
                    close(pipes[x].from_w[0]); close(pipes[x].from_w[1]);
                }
            }
            close(pipes[w].to_w[1]);   // оставим только to_w[0] (чтение команд)
            close(pipes[w].from_w[0]); // оставим только from_w[1] (передача энергии)

            int cmd_fd = pipes[w].to_w[0], data_fd = pipes[w].from_w[1];

            for (auto j : _jobs) {
                for (auto p : _procs) {
                    if (2 * p > j)
                        break;

                    for (TEMP_LAW = 0; TEMP_LAW < 3; ++TEMP_LAW) {
                        INIT_TEMP = DEFAULT_TEMP;

                        std::string path_data = "Input/data/"+std::to_string(j)+"_"+std::to_string(p)+".txt";
                        std::string path_system = "Input/sys/"+std::to_string(j)+"_"+std::to_string(p)+".txt";

                        std::map<int, std::set<int>> flws;
                        Graph graph(flws, path_data), best_graph(graph);
                        auto shed = std::make_unique<Shed>(graph, path_system);
                        shed->build(graph);
                        double best_energy = shed->get_energy();

                        sa_shed(shed, graph, best_graph, flws, best_energy, temp_funcitons[TEMP_LAW], cmd_fd, data_fd,
                            "tmp/shared_best_"+std::to_string(j)+"_"+std::to_string(p)+".bin");

                        if(!shed->is_correct(best_graph, flws))
                            std::cerr<<"Worker "<<w<<" BAD\n";
                    }
                }
            }

            _exit(0);
        } else {
            children[w] = pid;

            close(pipes[w].to_w[0]);
            close(pipes[w].from_w[1]);
        }
    }

    for (auto j : _jobs) {
        for (auto p : _procs) {
            if (2 * p > j)
                break;

            for (TEMP_LAW = 0; TEMP_LAW < 3; ++TEMP_LAW) {
                std::string temp_dir = TEMP_LAW == 0 ? "boltz/" : (TEMP_LAW == 1 ? "cauchy/" : "common/");

                std::cout << "-----------------------------------------------" << std::endl;
                std::cout << "Proc=" << p << "; Job=" << j << "; " << temp_dir << std::endl;
                std::cout << "-----------------------------------------------" << std::endl << std::endl;
                INIT_TEMP = DEFAULT_TEMP;
                auto start = std::chrono::high_resolution_clock::now();

                int no_improvement_count = 0, bad_upgrade = 0, best_process_rank = -1;
                double global_best = std::numeric_limits<double>::infinity();
                int number_iters = 0;
                double cur_best = 0;

                while(no_improvement_count < 10 && bad_upgrade < 10) {
                    // Получить энергии от всех
                    for(int w = 0;w < proc_num; ++w) {
                        double e = read_double(pipes[w].from_w[0]);
                        // лучшая энергия, запоминаем воркера
                        if (e < global_best) {
                            cur_best = e;
                            best_process_rank = w;
                            no_improvement_count = 0;
                        }
                    }
                    if((global_best-cur_best) < 10)
                        ++bad_upgrade;
                    else
                        bad_upgrade = 0;

                    global_best = cur_best==0 ? global_best : cur_best;
                    ++number_iters; ++no_improvement_count;

                    // Рассылаем команды
                    if (no_improvement_count == 10 || bad_upgrade == 10) {
                        for (int i = 0; i < proc_num; ++i) {
                            write_char(pipes[i].to_w[1], 2);
                        }
                    } else if (best_process_rank >= 0) {
                        // Посылаем '0' процессу с лучшей энергией, остальным — '1'
                        for (int i = 0; i < proc_num; ++i) {
                            char action = best_process_rank == i ? 0 : 1;
                            write_char(pipes[i].to_w[1], action);
                        }
                    } else {
                        // Посылаем значение '-1' всем
                        for (int i = 0; i < proc_num; ++i) {
                            write_char(pipes[i].to_w[1], -1);
                        }
                    }
                    // --- если best_process_rank>=0: получить от него лучший граф и shed, сохранить в файл-обменник
                    if(no_improvement_count!=10 && bad_upgrade!=10 && best_process_rank>=0) {
                        char action = read_char(pipes[best_process_rank].from_w[0]);

                        for (int i = 0; i < proc_num; ++i) {
                            if (i == best_process_rank)
                                continue;

                            write_char(pipes[i].to_w[1], 1);
                        }
                    }
                }
                // Лог необязателен
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                std::string path_res = "Output/fork_mpi/10/" + temp_dir + std::to_string(j) + "_" + std::to_string(p) +  ".txt";
                std::ofstream outfile(path_res);
                outfile << "Energy=" << global_best << "\n";
                outfile << "Time=" << duration.count() << "\n";
                outfile << "Iterations=" << number_iters * 100 << "\n";
                outfile.close();
            }
        }
    }
    // дожидаемся воркеров
    for(int i = 0;i < proc_num; ++i)
        waitpid(children[i],0,0);

    return 0;
}
