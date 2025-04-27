#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <random>
#include <limits>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <ctime>
#include <chrono>

using std::placeholders::_1;
using namespace std::chrono;

using ull = unsigned long long;

const double DEFAULT_TEMP = 50;
double INIT_TEMP = 50;

double CAUCHY_COEF = 5.7;
double BOLTZ_COEF = 6.15;
double COMMON_COEF = 17.8;

bool FIRST_TRY = true;

const double MIN_TEMP = 0.1;
const int STEP_NUM = 200;
const int PARALLEL_STEP_NUM = 301;
const double EPS = 0.1;

struct Job {
    int id = -1;
    int proc_id = -1;
    int lvl = 0;
    int proc_lvl = 0;

    ull arrive_time = 0;
    ull prev_time = 0;
    ull beg_time = 0;
    ull cur_time = 0;

    double beg_freq = 0.0;
    double weight = 0.0;

    Job() = default;
    Job(int id, ull time, double freq)
        : id(id), beg_time(time), beg_freq(freq), cur_time(time), weight(time / freq) {}

    std::string serialize() const {
        std::ostringstream oss;
        // Добавляем проверку на ошибки записи
        if (!(oss << proc_id << " "
                << lvl << " "
                << proc_lvl << " "
                << arrive_time << " "
                << prev_time << " "
                << cur_time << " "
                << weight)) {
            throw std::runtime_error("Failed to serialize Job");
        }
        return oss.str();
    }
    
    void deserialize(const std::string &data) {
        std::istringstream iss(data);
        // Читаем все поля последовательно
        if (!(iss >> proc_id
                >> lvl
                >> proc_lvl
                >> arrive_time
                >> prev_time
                >> cur_time
                >> weight)) {
            throw std::runtime_error("Failed to deserialize Job");
        }
        
        // Проверяем, что все данные были прочитаны
        std::string remaining;
        if (std::getline(iss, remaining) && !remaining.empty()) {
            throw std::runtime_error("Extra data in Job deserialization");
        }
    }

    Job& operator=(const Job&) = default;

    bool operator<(const Job& other) const { return weight < other.weight; }
    bool operator>(const Job& other) const { return weight > other.weight; }
};

struct Proc {
    double min_freq = 0.0;
    double max_freq = 0.0;
    double step = 0.0;
    double cur_freq = 0.0;
    double beg_volt = 0.0;
    double volt = 0.0;
    double cap = 0.0;
    ull max_time = 0;
    ull job_time = 0;

    Proc() = default;
    Proc(double min_f, double max_f, double s, double v, double c)
        : min_freq(min_f), max_freq(max_f), step(s), cur_freq(max_f), beg_volt(v), volt(v), cap(c) {}

    std::string serialize() const {
        std::ostringstream oss;
        oss << cur_freq << " "
            << volt << " " 
            << max_time << " "
            << job_time;
        
        // std::cout << "LELELELE1 ----- " << double(max_time) << std::endl;
        // Проверка на ошибки записи
        if (oss.fail()) {
            throw std::runtime_error("Failed to serialize Proc data");
        }
        
        return oss.str();
    }

    void deserialize(const std::string& data) {
        std::istringstream iss(data);
        iss.exceptions(std::ios::failbit | std::ios::badbit);

        try {
            iss >> cur_freq;
            iss >> volt;
            iss >> max_time;
            iss >> job_time;
            
            // std::cout << "ALL IS OKAY!!!!" << std::endl;
        
            // Проверка на лишние данные
            // char remaining;
            // if (iss >> remaining) {
            //     throw std::runtime_error("Extra data in Proc deserialization");
            // }
        }
        catch (const std::ios_base::failure& e) {
            throw std::runtime_error("Failed to deserialize Proc: " + std::string(e.what()));
        }
    }
};
