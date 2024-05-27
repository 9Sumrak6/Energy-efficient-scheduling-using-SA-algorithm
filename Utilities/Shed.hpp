#pragma once

#include <cmath>
#include <random>
#include <limits>
#include <time.h>

#include "Graph.hpp"

bool is_equal(double x, double y) {
    return fabs(x - y) < std::numeric_limits<double>::epsilon();
}

class Shed {
    string inp_path;
    int n_procs;

    double max_time;

    double energy;

    map<int, double> extra_time;
    map<int, vector<int>> rel;
    map<int, Proc> procs;
    map<int, int> proc_last_time;
public:
    Shed(Graph &graph, const string& path = "Input/sys/default.txt");

    bool is_correct(Graph &graph, map<int, set<int>> &follows);
    
    bool set_random_freq(Graph &graph, int proc_id = -1);
    
    bool move_job(Graph &graph, int job_id, int proc_id = -1);
    void push_back(Graph &graph, int job_id, int proc_id);
    bool move_random_job(Graph &graph);
    bool switch_jobs(Graph &graph, map<int, set<int>> &follows);

    double calc_energy();

    double get_energy();
    int get_proc_num();
    int get_job_num(Graph &graph);

    vector<int> get_topo(Graph &graph);
    void build_time(Graph &graph);
    void switch_lvl(int j1, int j2, Graph &graph);

    void build(Graph &graph);

    void print(Graph &graph);
    void print_rel();
};

Shed::Shed(Graph &graph, const string& path) {
    Reader reader;

    inp_path = path;

    pair<int, map<int, Proc>> res = reader.readSimpleSys(path, max_time);
    n_procs = res.first;
    procs = res.second;

    for (int i = 0; i < n_procs; i++) {
        proc_last_time[i] = 0;
        if (i == 0)
            for (auto j : graph.get_topo())
                this->push_back(graph, j, 0);
        else
            rel[i] = vector<int>(0);
        extra_time[i] = 0.0;
    }

    energy = 0.0;
}

bool Shed::is_correct(Graph &graph, map<int, set<int>> &fols) {
    for (auto &[id, vec] : rel) {
        double prev_time = 0.0, prev_dur = 0.0;
        for (auto &job_id : vec) {
            Job j = graph.get_job(job_id);

            if (prev_time + prev_dur > j.arrive_time) {
                cout << "1) Proc=" << id << " :: " << "Job=" << job_id << endl;
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

bool Shed::move_job(Graph &graph, int job_id, int proc_id) {
    if (n_procs == 1)
        return true;
    Job job = graph.get_job(job_id);
    int proc = proc_id;
    int cur_proc = job.proc_id;

    if (proc_id < 0) {
        proc = rand() % n_procs;

        while (proc == cur_proc)
            proc = rand() % n_procs;
    } else if (proc_id == cur_proc)
        return false;

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
}

void Shed::push_back(Graph &graph, int job_id, int proc_id) {
    rel[proc_id].push_back(job_id);
    graph.set_job(job_id, proc_id);
    graph.set_job_proc_lvl(job_id, rel[proc_id].size() - 1);
    graph.set_freq(job_id, procs[proc_id].cur_freq);    
}

bool Shed::switch_jobs(Graph &graph, map<int, set<int>> &flws) {
    for (auto &[i, jobs] : rel) {
        for (size_t j = 0; j < jobs.size(); j++) {
            if (j + 1 == jobs.size())
                break;

            int j1 = jobs[j];
            int j2 = jobs[j + 1];

            Job job1 = graph.get_job(j1);
            Job job2 = graph.get_job(j2);

            if (j == 0 && job1.arrive_time == 0)
                continue;

            if (job1.arrive_time + job1.cur_time < job2.arrive_time)
                continue;

            if (j > 0 && graph.get_job(j-1).arrive_time + graph.get_job(j-1).cur_time == job1.arrive_time)
                continue;

            bool fl = false;

            vector<int> topo = graph.get_topo();
            for (int i = job1.lvl; i <= job2.lvl; i++) {
                if ((i != job1.lvl && flws[j1].find(topo[i]) != flws[j1].end()) || (i != job2.lvl && flws[topo[i]].find(j2) != flws[topo[i]].end())) {
                    fl = true;
                    break;
                }
            }

            if (fl)
                continue;

            graph.switch_lvl(j1, j2);
            rel[i][j] = j2;
            rel[i][j + 1] = j1;

            return true;
        } 
    }
    return true;
}

void Shed::switch_lvl(int j1, int j2, Graph &graph) {
    graph.switch_lvl(j1, j2);
    Job job1 = graph.get_job(j1), job2 = graph.get_job(j2);
    int i = job1.proc_id;
    int i1 = job2.proc_id;

    if (i != i1 || i < 0 || i1 < 0)
        return;

    if (job1.proc_lvl < job2.proc_lvl) {
        rel[i].insert(rel[i].begin() + job2.proc_lvl + 1, j1);
        rel[i].erase(rel[i].begin() + job2.proc_lvl);
        rel[i].insert(rel[i].begin() + job1.proc_lvl + 1, j2);
        rel[i].erase(rel[i].begin() + job1.proc_lvl);
    } else {
        rel[i].insert(rel[i].begin() + job1.proc_lvl + 1, j2);
        rel[i].erase(rel[i].begin() + job1.proc_lvl);
        rel[i].insert(rel[i].begin() + job2.proc_lvl + 1, j1);
        rel[i].erase(rel[i].begin() + job2.proc_lvl);
    }
}

bool Shed::move_random_job(Graph &graph) {
    return move_job(graph, rand() % graph.get_job_num());
}


double Shed::calc_energy() {
    double energy = 0.0;
    for (auto &[id, vec] : rel) {
        Proc proc = procs[id];

        energy += proc.volt * proc.cur_freq * proc.cur_freq * (proc.job_time + 3 * extra_time[id] + (proc.max_time - proc.job_time) / 2);
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
    for (auto &[id, proc] : procs) {
        proc.max_time = 0;
        proc.job_time = 0;
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
}

vector<int> Shed::get_topo(Graph &graph) {
    return graph.get_topo();
}

void Shed::build(Graph &graph) {
    build_time(graph);
    calc_energy();
}

void Shed::print(Graph &graph) {
    graph.print();

    cout << "----------------------------" << endl;

    for (auto &[id, vec] : rel) {
        cout << "Processor " << id << "(volt=" << procs[id].volt << "; freq=" << procs[id].cur_freq << "; job_t=" << procs[id].job_time << "):: ";

        for (auto job_id : vec) {
            Job j = graph.get_job(job_id);
            cout << "'id=" << j.id << " arrive_time=" << j.arrive_time << " dur=" << j.cur_time << "'    ";
        }

        cout << endl;
    }
    cout << endl << "Result energy consumption = " << energy << endl;
    cout << "----------------------------" << endl;

}