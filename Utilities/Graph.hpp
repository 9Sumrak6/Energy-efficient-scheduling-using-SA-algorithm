#pragma once

#include <algorithm>

#include "Reader.hpp"


using std::cout, std::endl;
using std::max;


class Graph {
    string inp_path;

    int n_jobs;

    map<int, Job> jobs;
    map<int, Job> jobs_ch;

    map<int, set<int>> graph;

    vector<int> topo;
    map<int, int> topo_ch;
    void find_follows(int id, map<int, set<int>> &follows, set<int> &checked);
    void topological_sort_util(int v, bool visited[]);
    void topological_sort();
public:
    int free_jobs;

    Graph(map<int, set<int>> &follows, const string& path = "");

    Job get_job(int id);

    ull set_arr_time(int job_id, ull arr_time, bool fl = false);
    ull set_prev_time(int job_id, ull prev_time, bool restart = false, bool fl = false);
    bool set_freq(int job_id, double freq, bool fl = false);
    bool set_job(int job_id, int proc_id, bool fl = false);
    void set_job_proc_lvl(int job_id, int proc_lvl, bool fl = false);

    void switch_lvl(int job1, int job2);
    
    int get_job_num();

    vector<int> get_topo();

    void erase_ch();
    void backup();

    void print();
    void print_procid();

    set<int> operator[](unsigned index) { return graph[index]; }
};

void Graph::find_follows(int id, map<int, set<int>> &flws, set<int> &checked) {
    if (flws[id].size() > 0)
        return;

    for (auto i : graph[id]) {
        flws[id].insert(i);
        if (checked.insert(i).second) {
            find_follows(i, flws, checked);
            flws[id].insert(flws[id].begin(), flws[id].end());
        }
    }
}

void Graph::topological_sort_util(int v, bool visited[]) {
    // Помечаем текущий узел как посещенный
    visited[v] = true;
  
    // Рекурсивно вызываем функцию для всех смежных вершин
    for (auto i : graph[v])
        if (!visited[i])
            topological_sort_util(i, visited);
  
    // Добавляем текущую вершину в стек с результатом
    topo.push_back(v);
    jobs[v].lvl = n_jobs - topo.size();
}

void Graph::topological_sort() {
    // Помечаем все вершины как непосещенные
    bool *visited = new bool[n_jobs];

    for (int i = 0; i < n_jobs; i++)
        visited[i] = false;
  
    // Вызываем рекурсивную вспомогательную функцию 
    // для поиска топологической сортировки для каждой вершины
    for (int i = 0; i < n_jobs; i++)
        if (visited[i] == false)
            topological_sort_util(i, visited);

    reverse(topo.begin(), topo.end());
}

Graph::Graph(map<int, set<int>> &follows, const string& path) {
    if (path == "")
        return;

    Reader reader;

    pair<int, map<int, set<int>>> in = reader.readGraph(path, jobs);

    n_jobs = in.first;
    free_jobs = n_jobs;
    graph = in.second;

    inp_path = path;

    for (auto &[id, jobs] : graph) {
        set<int> checked;

        find_follows(id, follows, checked);
    }
    topological_sort();
}

Job Graph::get_job(int id) {
    return jobs[id];
}

bool Graph::set_freq(int job_id, double freq, bool fl) {

    if (jobs.count(job_id) > 0) {
        if (fl)
            jobs_ch[job_id] = jobs[job_id];
        jobs[job_id].cur_time = ceil((jobs[job_id].beg_time * jobs[job_id].beg_freq) / freq);

        return true;
    }

    return false;
}

void Graph::set_job_proc_lvl(int job_id, int lvl, bool fl) {
    if (fl)
        jobs_ch[job_id] = jobs[job_id];

    if (lvl == -1)
        ++jobs[job_id].proc_lvl;
    else if (lvl == -2)
        --jobs[job_id].proc_lvl;
    else
        jobs[job_id].proc_lvl = lvl;
}

void Graph::switch_lvl(int job1, int job2) {
    int lvl1 = jobs[job1].lvl, lvl2 = jobs[job2].lvl, proc_lvl1 = jobs[job1].proc_lvl;

    jobs_ch[job1] = jobs[job1], jobs_ch[job2] = jobs[job2];
    topo_ch[lvl1] = job1, topo_ch[lvl2] = job2;
    topo[lvl1] = job2, topo[lvl2] = job1;

    jobs[job1].lvl = jobs[job2].lvl, jobs[job1].proc_lvl = jobs[job2].proc_lvl;
    jobs[job2].lvl = lvl1, jobs[job1].proc_lvl = proc_lvl1;
}

ull Graph::set_prev_time(int job_id, ull arr_time, bool restart, bool fl) {
    if (restart) {
        jobs[job_id].prev_time = 0;
        if (fl && jobs_ch.count(job_id) > 0)
           jobs_ch[job_id].prev_time = 0;
    }
    else {
        jobs[job_id].prev_time = max(jobs[job_id].prev_time, arr_time);
        if (fl && jobs_ch.count(job_id) > 0)
            jobs_ch[job_id].prev_time = max(jobs[job_id].prev_time, arr_time);
    }

    return jobs[job_id].prev_time;
}

ull Graph::set_arr_time(int job_id, ull arr_time, bool fl) {
    jobs[job_id].arrive_time = arr_time;
    if (fl && jobs_ch.count(job_id) > 0)
        jobs_ch[job_id].arrive_time = arr_time;

    return arr_time;
}

bool Graph::set_job(int job_id, int proc_id, bool fl) {
    if (jobs.count(job_id) > 0) {
        if (fl)
            jobs_ch[job_id] = jobs[job_id];

        if (proc_id < 0)
            ++free_jobs;
        if (jobs[job_id].proc_id < 0 && proc_id >= 0)
            --free_jobs;

        jobs[job_id].proc_id = proc_id;

        return true;
    }

    return false;
}

int Graph::get_job_num() {
    return n_jobs;
}

vector<int> Graph::get_topo() {
    return topo;
}

void Graph::erase_ch() {
    jobs_ch.clear();
    topo_ch.clear();
}

void Graph::backup() {
    for (auto &[id, job] : jobs_ch)
        jobs[id] = job;

    for (auto &[id, job] : topo_ch)
        topo[id] = job;

    erase_ch();
}

void Graph::print() {
    int id = 0;

    // for (auto i : follows)
    //     cout << i.first << " = " << i.second.size() << endl;

    // cout << "List of vertices (id of vertex, freq of vertex, time of vertex):" << endl;

    // for (auto [i, j] : jobs) {
    //     cout << "ID = " << i << ". Freq = " << j.beg_freq << ". Time = " << j.beg_time << endl; 
    //     ++id;
    // }
    // cout << endl;

    cout << "List of edges (id of vertex_from, set of vertexes to):" << endl;

    id = 0;
    for (auto i : graph) {
        cout << i.first << " :: ";
        for (auto j : i.second)
            cout << j << " "; 
        cout << endl;
        ++id;
    }

    cout << "Topo sort: ";
    for (auto i : topo)
        cout << i << " ";
    cout << endl;
}

void Graph::print_procid() {
    cout << "-------------->PROC_ID" << endl;
    for (auto i : graph)
        cout << jobs[i.first].id << " ";
    cout << endl;
    for (auto i : graph)
        cout << jobs[i.first].proc_id << " ";
    cout << endl;
    for (auto i : graph)
        cout << jobs[i.first].proc_lvl << " ";
    cout << endl;
    cout << "-------------->PROC_ID" << endl;
}