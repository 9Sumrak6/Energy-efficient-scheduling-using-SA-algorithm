#pragma once

#include "Components.hpp"

using std::pair, std::vector, std::string, std::set, std::map;


class Reader {
public:
    pair<int, vector<set<int>>> readGraph(const string &path, vector<Job> &jobs);
    pair<int, vector<Proc>> readSimpleSys(const string &path, double &max_time);
};

pair<int, vector<set<int>>> Reader::readGraph(const string &path, vector<Job> &jobs) {
    std::ifstream f(path);
    string line;
    getline(f, line);

    int n = stoi(line);
    pair<int, vector<set<int>>> res(n, vector<set<int>>(n));
    jobs = vector<Job>(n);

    for (int i = 0; i < n; i++) {
        getline(f, line);
        std::stringstream ss(line);

        string id, time, freq;
        ss >> id >> time >> freq;
		
		jobs[stoi(id)] = Job(stoi(id), stod(time), stod(freq));
        // res.second[stoi(id)] = {};
    }

    while (getline(f, line)) {
        std::stringstream ss(line);
        
        string from, in;
        ss >> from >> in;

        res.second[stoi(from)].insert(stoi(in));
    }

    return res;
}

pair<int, vector<Proc>> Reader::readSimpleSys(const string &path, double &max_time) {
    std::ifstream f(path);

    string line;
    
    getline(f, line);
    max_time = stod(line);

    getline(f, line);    
    int n = stoi(line);

    vector<Proc> res(n);

    getline(f, line);
    std::stringstream ss(line);

    string min_freq, max_freq, step, volt, cap;
    ss >> min_freq >> max_freq >> step;

    getline(f, volt);
    getline(f, cap);
    
    for (int i = 0; i < n; i++)
        res[i] = Proc(stod(min_freq), stod(max_freq), stod(step), stod(volt), stod(cap));

    return pair(n, res);
}