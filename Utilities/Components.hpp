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

constexpr double INIT_TEMP = 20;
constexpr double MIN_TEMP = 0.1;
constexpr int STEP_NUM = 201;
constexpr double EPS = 0.1;

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
        oss << id << " "
            << proc_id << " "
            << lvl << " "
            << proc_lvl << " "
            << arrive_time << " "
            << prev_time << " "
            << beg_time << " "
            << cur_time << " "
            << beg_freq << " "
            << weight;
        return oss.str();
    }

    void deserialize(const std::string &data) {
        std::istringstream iss(data);
        iss >> this->id
            >> this->proc_id
            >> this->lvl
            >> this->proc_lvl
            >> this->arrive_time
            >> this->prev_time
            >> this->beg_time
            >> this->cur_time
            >> this->beg_freq
            >> this->weight;
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
        oss << min_freq << " " 
            << max_freq << " " 
            << step << " "
            << cur_freq << " "
            << beg_volt << " " 
            << volt << " " 
            << cap << " "
            << max_time << " "
            << job_time;
        return oss.str();
    }
    void deserialize(const std::string &data) {
        std::istringstream iss(data);
        iss >> this->min_freq
            >> this->max_freq
            >> this->step
            >> this->cur_freq
            >> this->beg_volt
            >> this->volt
            >> this->cap
            >> this->max_time
            >> this->job_time;
    }
};
