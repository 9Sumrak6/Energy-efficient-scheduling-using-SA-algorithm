#include <memory>      // Для std::unique_ptr
#include <random>      // Для случайных чисел
#include <functional>  // Для std::function
#include <cmath>       // Для log и exp
#include "Utilities/Shed.hpp"

int number_iters = 0;
int TEMP_LAW = 1;
bool STOP_CRITEO = false;


double boltz(int i) {
    return INIT_TEMP / log(i + 1);
}

double cauchy(int i) {
    return INIT_TEMP / (i + 1);
}

double law(int i) {
    return INIT_TEMP * log(i + 1) / (i + 1);
}

std::pair<bool, std::unique_ptr<Shed>> generate_neighbor(const std::unique_ptr<Shed>& shed, Graph& graph, map<int, set<int>>& flws, char& fl, std::mt19937& rng) {
    auto new_shed = std::make_unique<Shed>(*shed);
    bool success = true;

    std::uniform_int_distribution<int> action_dist(0, 2);  // Генерация случайного действия
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

void sa_shed(std::unique_ptr<Shed>& shed, Graph& graph, Graph& best_graph, map<int, set<int>>& flws, double& best, std::function<double(int)> temp_func, char fl) {
    std::random_device rd;
    std::mt19937 rng(rd());

    auto cur_shed = std::make_unique<Shed>(*shed);
    int iteration = 0;
    double temperature = INIT_TEMP;

    int non_progress = 0;
    while (non_progress < 500 || !STOP_CRITEO) {
        for (int i = 0; i < STEP_NUM; ++i) {
        	// auto start = std::chrono::high_resolution_clock::now();
            auto [success, new_shed] = generate_neighbor(cur_shed, graph, flws, fl, rng);
            if (!success) {
                ++non_progress;
                continue;
            }
            new_shed->build(graph);

            if (!new_shed->is_correct(graph, flws)) {
            	cout << "--------------------" << endl;
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
            double prob = exp((cur_energy - new_energy) / temperature);
            if (prob < 0.0001)
                STOP_CRITEO = true;
            if (new_energy < cur_energy || prob_dist(rng) < prob) {
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
            ++number_iters;
        }

        temperature = temp_func(++iteration);
    }
}

int main() {
    std::ios_base::sync_with_stdio(false);

    vector<int> _jobs = {10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 2000, 3000, 4000};
    vector<int> _procs = {2, 3, 4, 5, 10, 20, 40, 60, 80, 100, 120, 140, 160};
    vector<std::function<double(int)>> laws = {boltz, cauchy, law};

    for (auto j : _jobs) {
        for (auto p : _procs) {
            if (2 * p > j)
                break;

            cout << "Jobs : " << j << ",  Procs : " << p << endl;
            for (int TEMP_LAW = 0; TEMP_LAW < 3; ++TEMP_LAW) {
                INIT_TEMP = DEFAULT_TEMP;
                FIRST_TRY = true;

                std::string temp_dir = TEMP_LAW == 0 ? "boltz/" : (TEMP_LAW == 1 ? "cauchy/" : "common/");
                std::string path_data, path_system, path_res;

                // std::cout << "Enter name of file with graph (file must be located in directory 'Input/data/')\n>";
                // std::cin >> path_data;
                // path_system = "Input/sys/" + path_data;
                path_data = "Input/data/" + std::to_string(j) + "_" + std::to_string(p) + ".txt";

                // std::cout << "Enter name of file with system (file must be located in directory 'Input/sys/')\n>";
                // std::cin >> path_system;
                // path_system = "Input/sys/" + path_system;
                path_system = "Input/sys/" + std::to_string(j) + "_" + std::to_string(p) + ".txt";

                // std::cout << "Enter name of file with optimum energy (file must be located in directory 'Input/opt/')\n>";
                // std::cin >> path_res;
                // path_res = "Output/" + path_res;
                // path_res = "Output/consecutive/" + std::to_string(j) + "_" + std::to_string(p) + ".txt";

                map<int, set<int>> flws;
                Graph graph(flws, path_data), best_graph(graph);

                auto shed = std::make_unique<Shed>(graph, path_system);
                shed->build(graph);

                double best_energy = shed->get_energy();

                char fl = -1;
                auto start = std::chrono::high_resolution_clock::now();
                sa_shed(shed, graph, best_graph, flws, best_energy, laws[TEMP_LAW], fl);
                std::cout << "----------------------" << endl;
                auto end = std::chrono::high_resolution_clock::now();

                if (shed->is_correct(best_graph, flws)) {
                    std::cout << "Schedule is correct.\n";
                } else {
                    best_graph.print_procid();
                    shed->print(best_graph);
                    std::cerr << "-----------Schedule is not correct!!!-----------\n";
                    continue;
                }

                // std::cout << "\nBest energy: " << best_energy << "\n";

                // std::ifstream infile(path_res);
                // string in;
                // getline(infile, in);
                // double best_prev = stod(in);

                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                path_res = "Output/consecutive/" + temp_dir + std::to_string(j) + "_" + std::to_string(p) +  ".txt";
                std::ofstream outfile(path_res);
                outfile << "Energy=" << best_energy << "\n";
                outfile << "Time=" << duration.count() << "\n";
                outfile << "Iterations=" << number_iters << "\n";
                outfile.close();
            }
        }
    }

    return 0;
}
