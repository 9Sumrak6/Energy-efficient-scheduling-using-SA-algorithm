#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <random>
using std::cout, std::endl, std::cin, std::getline;
using std::vector, std::string, std::map, std::set;
using std::ifstream, std::ofstream;


const double K = 0.5;
const int CAP = 1;
const size_t MAX_EDGE_NUM = 10;

struct Job {
	int id;
	int proc;

	int arr_time;
	int end_time;
	int dur;

	Job() = default;
    Job(int id, int proc, int arr_time, int dur) : id(id), proc(proc), arr_time(arr_time), end_time(arr_time + dur), dur(dur) {}

    bool operator<(const Job& j) const {
    	if (this->end_time == j.arr_time) {
    		return true;
    	}
    	if (this->end_time < j.arr_time) {
    		return true;
    	}
    	if (this->end_time != j.end_time) {
    		return this->end_time < j.end_time;
    	}
    	if (this->arr_time != j.arr_time) {
    		return this->arr_time < j.arr_time;
    	}
		return this->id < j.id;
    }
};

void get_graph(map<int, Job> &jobs, map<int, set<int>> &graph, map<int, set<int>> &prev, set<int> &checked, int id) {
	if (graph.count(id) == 0)
		return;

	for (auto i : graph[id]) {
		if (!checked.insert(i).second)
			continue;

		for (auto pr : prev[i]) {
			if (!checked.insert(pr).second)
				continue;

			get_graph(jobs, graph, prev, checked, pr);
		}
	}
}

int main() {
	vector<unsigned> job_num = {10, 20, 30, 40, 50, 100, 200, 300, 400, 500, 1000, 2000, 3000, 4000};
	vector<unsigned> proc_num = {2, 3, 4, 5, 10, 20, 40, 60, 80, 100, 120, 140, 160};

	map<int, double> volt{{3, 1.4}, {4, 1.5}, {5, 1.6}};

	for (auto j : job_num) {
		for (auto p : proc_num) {
			if (p >= j)
				break;

			map<int, Job> jobs;

			map<int, set<int>> graph;
			map<int, set<int>> later;
			map<int, set<int>> prev;

			int deadline = j * 20;

			int max_freq = 50, min_freq = 25;
			max_freq = rand() % (max_freq - min_freq) + min_freq + 1;
			min_freq = rand() % (max_freq - min_freq) + min_freq;
			double real_max_freq = max_freq * 0.1, real_min_freq = min_freq * 0.1;
			
			double vlt = volt[ceil(max_freq / 10.0)];

			double absolute_minim_energy = K * CAP * p * vlt * (min_freq * min_freq * min_freq / max_freq) * deadline * 1e-2;
			double cur_min_energy = 0.0;

			vector<double> proc_freq;
			vector<double> proc_volt;
			for (unsigned i = 0; i < p; i++) {
				proc_freq.push_back(float(rand() % (max_freq - min_freq) + min_freq) * 0.1);
				proc_volt.push_back((vlt * proc_freq[i]) / real_max_freq);
			}

			int taken = 0, job_id = 0, minim_id = 0;
			bool next = false;
			for (unsigned i = 0; i < p; i++) {
				cur_min_energy += proc_volt[i] * proc_freq[i] * proc_freq[i];

				if (i == p - 1)
					minim_id = job_id;

				int num = i == p - 1 ? j - taken : j / p;
				if (num == 1) {
					next = true;
					break;
				}
				taken += num;

				int job_time = deadline / num, arr_time = 0;
				for (int k = 0; k < num; k++) {
					job_time = k == num - 1 ? deadline - arr_time : job_time;
					jobs[job_id] = Job(job_id, i, arr_time, job_time);

					++job_id;
					arr_time += job_time;
				}
			}
			if (next) {
				cout << "Skip: " << j << "_" << p << endl;
				continue;
			}
			cur_min_energy *= K * CAP * deadline;
			if (cur_min_energy < absolute_minim_energy)
				cout << "WARNING!!!!!!" << endl << "\t" << j << " " << p << endl;

			for (auto &[id, j] : jobs)
				for (auto &[id1, j1] : jobs) {
					if (id == id1 || j.end_time > j1.arr_time)
						continue;

					later[id].insert(id1);
				}
			for (auto &[id, j] : later) {
				if (j.size() == 0)
					continue;

				unsigned n = std::min(rand() % j.size(), MAX_EDGE_NUM);
				n = n == 0 ? 1 : n;
				auto it = j.begin();
				for (unsigned S = j.size(); S > 0 && n > 0; --S, ++it)
			    	if (rand() % S < n) {
			        	graph[id].insert(*it);
						prev[*it].insert(id);
			        	--n;
			      	}
			}
			cout << endl;

			set<int> checked;
			checked.insert(minim_id);

			get_graph(jobs, graph, prev, checked, minim_id);

			bool get_new = true;
			while (checked.size() != j) {
				Job min = jobs[*checked.begin()];
				Job max = jobs[*checked.end()];

				if (get_new) {
					get_new = false;

					for (auto i : checked) {
						if (jobs[i].end_time < min.end_time)
							min = jobs[i];
						if (jobs[i].arr_time > max.arr_time)
							max = jobs[i];
					}
				}

				for (auto &[id, job] : jobs) {
					if (checked.insert(id).second) {
						if (jobs[id].arr_time > min.end_time) {
							graph[min.id].insert(job.id);
							prev[job.id].insert(min.id);
							get_new = true;
						} else if (jobs[id].end_time < max.arr_time) {
							graph[job.id].insert(max.id);
							prev[max.id].insert(job.id);
							get_new = true;
						}

						get_graph(jobs, graph, prev, checked, id);
					}
				}
			}

			string path_data = "Input/data/" + std::to_string(j) + "_" + std::to_string(p) + ".txt";
			string path_system = "Input/sys/" + std::to_string(j) + "_" + std::to_string(p) + ".txt";
			string path_opt = "Input/opt/" + std::to_string(j) + "_" + std::to_string(p) + ".txt";

			ofstream outfile;
		    outfile.open(path_opt);
		    outfile << std::to_string(absolute_minim_energy) << endl << std::to_string(cur_min_energy) << endl;
		    outfile.close();

		    outfile.open(path_system);
		    outfile << deadline << endl << p << endl;
		    outfile << real_min_freq << " " << real_max_freq << " " << 0.1 << endl;
		    outfile << vlt << endl << CAP << endl;
		    outfile.close();

		    outfile.open(path_data);
		    outfile << j << endl;
		    for (auto &[id, job] : jobs)
		    	outfile << id << " " << job.dur << " " << proc_freq[job.proc] << endl;

		    for (auto &[from, set] : graph)
		    	for (auto &to : set)
		    		outfile << from << " " << to << endl;
		    outfile.close();

		    cout << "Done: " << std::to_string(j) + "_" + std::to_string(p) << endl;
		}

	}
	return 0;
}