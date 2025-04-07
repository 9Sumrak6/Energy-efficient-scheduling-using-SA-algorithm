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
constexpr int STEP_NUM = 200;

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
};
