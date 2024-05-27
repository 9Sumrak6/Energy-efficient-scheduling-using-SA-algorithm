#include <iostream>

typedef unsigned long long ull;

typedef struct Job {
	int id;
    int proc_id;

    int lvl;
    int proc_lvl;

    ull arrive_time;
    ull prev_time;

	ull beg_time;
    double beg_freq;

	ull cur_time;  
	
    double weight;

	Job() = default;
	Job(int id, ull time, double freq) :id(id), proc_id(-1), lvl(0), proc_lvl(0),
                                        arrive_time(0.0), prev_time(0), beg_time(time),
                                        beg_freq(freq), cur_time(time),// cur_freq(freq),
                                        weight(beg_time / beg_freq) {}

    bool operator<(const Job& j) const { return this->weight < j.weight; }
    bool operator>(const Job& j) const { return this->weight > j.weight; }
} Job;

typedef struct Proc {
    double min_freq;
    double max_freq;
    double step;
    double cur_freq;
    double beg_volt;
    double volt;
    double cap;

    ull max_time;
    ull job_time;

    Proc() = default;
    Proc(double min_f, double max_f, double s, double v, double c) :min_freq(min_f), max_freq(max_f), step(s),
                                                                    cur_freq(max_f), beg_volt(v), volt(v),
                                                                    cap(c), max_time(0), job_time(0) {}
} Proc;