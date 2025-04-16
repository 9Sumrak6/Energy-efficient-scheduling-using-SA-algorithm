#include <iostream>
#include <memory>
#include <random>
#include <functional>
#include <cmath>
#include <vector>
#include "mpi.h"
#include "Utilities/Shed.hpp"
#include "Utilities/Graph.hpp"

// Константы:
const int NUM_WORKERS = 4; // Рабочие процессы
const int COORDINATOR = NUM_WORKERS; // Процесс-координатор
int rank, world_size, number_iters;

double boltz(int i) {
    return INIT_TEMP / log(i + 1);
}

double cauchy(int i) {
    return INIT_TEMP / (i + 1);
}

double law(int i) {
    return INIT_TEMP * log(i + 1) / (i + 1);
}

// Генерация соседа
std::pair<bool, std::unique_ptr<Shed>> generate_neighbor(const std::unique_ptr<Shed>& shed, Graph& graph, std::map<int, std::set<int>>& flws, char& fl, std::mt19937& rng) {
    auto new_shed = std::make_unique<Shed>(*shed);
    bool success = true;
    std::uniform_int_distribution<int> action_dist(0, 2);
    int action = action_dist(rng);

    switch (action) {
        case 0:
            fl = 0;
            success = new_shed->move_random_job(graph);
            break;
        case 1:
            fl = 1;
            success = new_shed->switch_jobs(graph, flws);
            break;
        case 2:
            fl = 2;
            success = new_shed->set_random_freq(graph);
            break;
    }

    return {success, std::move(new_shed)};
}

// Функция симулированного отжига
void sa_shed(std::unique_ptr<Shed>& shed, Graph& graph, Graph& best_graph, std::map<int, std::set<int>>& flws, double& best, std::function<double(int)> temp_func, char fl, int rank) {
    std::random_device rd;
    std::mt19937 rng(rd());

    auto cur_shed = std::make_unique<Shed>(*shed);
    int iteration = 0;
    double temperature = INIT_TEMP;

    while (true) {
        int non_progress = 0;
        for (int i = 1; i < PARALLEL_STEP_NUM; ++i) {
            auto [success, new_shed] = generate_neighbor(cur_shed, graph, flws, fl, rng);
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
                char toUpdate;
                MPI_Send(&best, 1, MPI_DOUBLE, COORDINATOR, 0, MPI_COMM_WORLD);
                MPI_Recv(&toUpdate, 1, MPI_CHAR, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // обновление графа и Sheds
                if (toUpdate == 2) { // конец алгоритма
                    return;
                } else if (toUpdate == 1) { // получить лучшее решение
                    unsigned graph_data_size;
                    MPI_Recv(&graph_data_size, 1, MPI_UNSIGNED, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    std::vector<char> graph_data(graph_data_size);
                    MPI_Recv(graph_data.data(), graph_data_size, MPI_CHAR, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    try {
                        best_graph.deserialize(std::string(graph_data.begin(), graph_data.end()));
                        graph.deserialize(std::string(graph_data.begin(), graph_data.end()));
                    } catch (const std::exception& e) {
                        throw std::runtime_error("Graph deserialization failed: " + std::string(e.what()));
                    }

                    // Получение данных shed
                    unsigned shed_data_size;
                    MPI_Recv(&shed_data_size, 1, MPI_UNSIGNED, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    std::vector<char> shed_data(shed_data_size);
                    MPI_Recv(shed_data.data(), shed_data_size, MPI_CHAR, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                    try {
                        shed->deserialize(std::string(shed_data.begin(), shed_data.end()));
                        cur_shed->deserialize(std::string(shed_data.begin(), shed_data.end()));
                    } catch (const std::exception& e) {
                        throw std::runtime_error("Shed deserialization failed: " + std::string(e.what()));
                    }
                } else if (toUpdate == 0) { // отослать лучшее решение

                    {
                        std::string data = best_graph.serialize();
                        unsigned data_size = data.size() + 1; 
                    
                        // Отправляем размер данных
                        MPI_Send(&data_size, 1, MPI_UNSIGNED, COORDINATOR, 0, MPI_COMM_WORLD);
                        
                        // Отправляем сами данные (с нуль-терминатором)
                        MPI_Send(data.c_str(), data_size, MPI_CHAR, COORDINATOR, 0, MPI_COMM_WORLD);
                    }
                    
                    // Отправка данных shed
                    {
                        std::string data = shed->serialize();
                        unsigned data_size = data.size() + 1;
                    
                        // Отправляем размер данных
                        MPI_Send(&data_size, 1, MPI_UNSIGNED, COORDINATOR, 0, MPI_COMM_WORLD);
                        
                        // Отправляем сами данные (с нуль-терминатором)
                        MPI_Send(data.c_str(), data_size, MPI_CHAR, COORDINATOR, 0, MPI_COMM_WORLD);
                    }
                }

                // cout << "OK" << endl;
                MPI_Barrier(MPI_COMM_WORLD);
            }
        }
        temperature = temp_func(++iteration);
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (world_size != NUM_WORKERS + 1) {
        std::cerr << "This program requires " << NUM_WORKERS + 1 << " processes." << std::endl;
        MPI_Finalize();
        return 1;
    }

    std::vector<int> _jobs = {10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 2000, 3000, 4000};
    std::vector<int> _procs = {2, 3, 4, 5, 10, 20, 40, 60, 80, 100, 120, 140, 160};

    if (rank == COORDINATOR) {
        // Логика координатора
        for (auto j : _jobs) {
            for (auto p : _procs) {
                if (2 * p > j)
                    break;
                auto start = std::chrono::high_resolution_clock::now();
                auto end = start;

                int no_improvement_count = 0;
                double global_best = std::numeric_limits<double>::infinity();
                number_iters = 0;
                while (no_improvement_count < 10) {
                    double worker_best_energies;
                    int best_process_rank = -1;

                    // Получение энергий от всех рабочих процессов
                    for (int i = 0; i < NUM_WORKERS; ++i) {
                        MPI_Recv(&worker_best_energies, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        if (worker_best_energies < global_best) {
                            global_best = worker_best_energies;
                            best_process_rank = i;
                            no_improvement_count = 0;
                        }
                    }
                    ++number_iters;
                    // Увеличение количества итераций без улучшений
                    ++no_improvement_count;

                    if (no_improvement_count == 10) {
                        for (int i = 0; i < NUM_WORKERS; ++i) {
                            char end = 2;
                            MPI_Send(&end, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
                        }
                    } else if (best_process_rank >= 0) {
                        // Посылаем true процессу с лучшей энергией, остальные получают false
                        for (int i = 0; i < NUM_WORKERS; ++i) {
                            char action = best_process_rank == i ? 0 : 1;
                            MPI_Send(&action, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
                        }
                    } else {
                        // Посылаем true процессу с лучшей энергией, остальные получают false
                        for (int i = 0; i < NUM_WORKERS; ++i) {
                            char action = -1;
                            MPI_Send(&action, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
                        }
                    }

                    // Получение и рассылка улучшенного Shed и Graph
                    if (no_improvement_count != 10 && best_process_rank >= 0) {
                        // Получение улучшенного состояния от процесса
                        unsigned data_size;

                        MPI_Recv(&data_size, 1, MPI_UNSIGNED, best_process_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        std::vector<char> serialized_graph(data_size);
                        MPI_Recv(serialized_graph.data(), data_size, MPI_CHAR, best_process_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                        MPI_Recv(&data_size, 1, MPI_UNSIGNED, best_process_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        std::vector<char> serialized_shed(data_size);
                        MPI_Recv(serialized_shed.data(), data_size, MPI_CHAR, best_process_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                        // Рассылка улучшенного состояния остальным процессам
                        for (int i = 0; i < NUM_WORKERS; ++i) {
                            if (i == best_process_rank) continue;
                            data_size = serialized_graph.size();
                            MPI_Send(&data_size, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
                            MPI_Send(serialized_graph.data(), serialized_graph.size(), MPI_CHAR, i, 0, MPI_COMM_WORLD);

                            data_size = serialized_shed.size();
                            MPI_Send(&data_size, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD);
                            MPI_Send(serialized_shed.data(), serialized_shed.size(), MPI_CHAR, i, 0, MPI_COMM_WORLD);
                        }
                    }
                    MPI_Barrier(MPI_COMM_WORLD);
                }
                end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

                std::string path_res = "Output/" + std::to_string(j) + "_" + std::to_string(p) + ".txt";
                std::ofstream outfile(path_res);
                outfile << "Energy=" << global_best << "\n";
                outfile << "Time=" << duration << "\n";
                outfile << "Iterations=" << number_iters * 100 << "\n";
                outfile.close();
                // cout << "Best = " << global_best << endl;

                // char end = 2;
                // for (int i = 0; i < NUM_WORKERS; ++i) {
                //     MPI_Send(&end, 1, MPI_CHAR, i, 0, MPI_COMM_WORLD);
                // }
            }
        }
    } else {
        for (auto j : _jobs) {
            for (auto p : _procs) {
                if (2 * p > j)
                    break;

                std::cout << "Process " << rank << ": Jobs : " << j << ", Procs : " << p << std::endl;

                std::string path_data = "Input/data/" + std::to_string(j) + "_" + std::to_string(p) + ".txt";
                std::string path_system = "Input/sys/" + std::to_string(j) + "_" + std::to_string(p) + ".txt";

                std::map<int, std::set<int>> flws;
                Graph graph(flws, path_data), best_graph(graph);
                auto shed = std::make_unique<Shed>(graph, path_system);
                shed->build(graph);

                double best_energy = shed->get_energy();
                char fl = -1;
                sa_shed(shed, graph, best_graph, flws, best_energy, cauchy, fl, rank);

                if (shed->is_correct(best_graph, flws)) {
                    std::cout << "Process " << rank << ": Schedule is correct.\n";
                } else {
                    std::cerr << "Process " << rank << ": Schedule is not correct!\n";
                }

                MPI_Barrier(MPI_COMM_WORLD);
                // ждём указания на отправку координатору
                // bool should_send;
                // MPI_Recv(&should_send, 1, MPI_CXX_BOOL, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                // if (should_send) {
                //     // Сериализация `Shed` и `Graph`
                //     std::vector<char> serialized_shed = shed->serialize();  // Режим сериализации
                //     std::vector<char> serialized_graph = graph.serialize(); 

                //     // Передача данных координатору
                //     MPI_Send(serialized_shed.data(), serialized_shed.size(), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD);
                //     MPI_Send(serialized_graph.data(), serialized_graph.size(), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD);
                // } else {
                //     // Получение нового `Shed` и `Graph`
                //     std::vector<char> serialized_shed(serialized_shed.size());
                //     std::vector<char> serialized_graph(serialized_graph.size());

                //     MPI_Recv(serialized_shed.data(), serialized_shed.size(), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //     MPI_Recv(serialized_graph.data(), serialized_graph.size(), MPI_BYTE, COORDINATOR, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                //     // Десериализация
                //     *shed = Shed::deserialize(serialized_shed);
                //     graph = Graph::deserialize(serialized_graph);
                // }
            }
        }
    }

    // MPI_Finalize();
    return 0;
}
