#include <ctime>
#include <cmath>
#include <time.h>
#include "Utilities/Shed.hpp"

using std::placeholders::_1;

const double INIT_TEMP = 10;
const double MIN_TEMP = 0.01;
const int STEP_NUM = 500;


double boltz(int i) {
	return INIT_TEMP / log(i + 1);
}

double cauchy(int i) {
	return INIT_TEMP / (i + 1);
}

double law(int i) {
	return INIT_TEMP * log(i + 1) / (i + 1);
}

std::pair<bool, Shed *> generate_neighbor(Shed *shed, Graph &graph, map<int, set<int>> &flws, char &fl) {
	Shed *new_shed = new Shed(*shed);
	bool ans = true;

	int a = rand() % 3;
	switch (a) {
		// move to another proc
		case 0:
			fl = 0;
			ans = new_shed->move_random_job(graph);
			break;
		// switch 
		case 1:
			fl = 1;
			break;
			ans = new_shed->switch_jobs(graph, flws);
			break;
		// set freq
		case 2:
			fl = 2;
			ans = new_shed->set_random_freq(graph);
			break;
	}

    return std::pair{ans, new_shed};
}

void sa_shed(Shed *shed, Graph &graph, Graph &best_graph, map<int, set<int>> &flws, double &best, std::function<double(int)> f, char fl) {
    srand(time(0));

    Shed *cur_shed = new Shed(*shed);

    int i = 0, non_progress = 0;
    double t = INIT_TEMP;
    // clock_t start;
    while(non_progress <= 5000){
        for (int j = 0; j < STEP_NUM; j++) {
	        std::pair<bool, Shed *> neigbor = generate_neighbor(cur_shed, graph, flws, fl);
	        if (!neigbor.first) {
	        	++non_progress;
	        	delete neigbor.second;

	        	continue;
	        }
	        Shed *new_shed = neigbor.second;
	        new_shed->build(graph);
	        double cur_energy = cur_shed->get_energy(), new_energy = new_shed->get_energy();

	        if (new_energy < cur_energy || (rand() / double(RAND_MAX)) < exp((cur_energy - new_energy) / t)) {
	        	graph.erase_ch();
	        	delete cur_shed;
	        	cur_shed = new_shed;

	            if (new_energy < best) {
	            	// delete shed;
	                *shed = *cur_shed;
	                best_graph = graph;
	                best = new_energy;
	                non_progress = 0;
	            }
	            
	            // delete new_shed;
	        } else {
	        	graph.backup();
	        	delete new_shed;
	        }
 
			if (non_progress > 6000)
	           	return;

	        ++i;
	        ++non_progress;
       	}
       	t = f(i++);
    }
    // Распечатать окончательное решение
    return;
}

int main() {
	std::ios_base::sync_with_stdio(false);

	string path_data, path_system, path_res;

	cout << "Enter name of file with graph (file must be located in directory 'Input/data/')" << endl << ">";
	std::cin >> path_data;
	path_data = "Input/data/" + path_data;
	cout << endl;

	cout << "Enter name of file with system (file must be located in directory 'Input/sys/')" << endl << ">";
	std::cin >> path_system;
	path_system = "Input/sys/" + path_system;
	cout << endl;

	cout << "Enter name of file with optimum energy (file must be located in directory 'Input/opt/')" << endl;
	cout << "The found solution will be written to the file of same name in directory 'Output/'" << endl << ">";
	std::cin >> path_res;
	path_res = "Output/" + path_res;
	cout << endl;

	cout << "If data file or the file with system info does not exist, the program will terminate with error!" << endl << endl;
	cout << path_data << endl << path_system << endl << path_res << endl;

	map<int, set<int>> flws;
	Graph graph(flws, path_data), best_graph(graph);
    Shed *shed = new Shed(graph, path_system);
    shed->build(graph);

    double best = shed->get_energy();

    cout << "----------------------" << endl;
    cout << "Begin energy = " << best << endl << endl;

    char fl = -1;
    sa_shed(shed, graph, best_graph, flws, best, std::bind(cauchy, _1), fl);

    if (shed->is_correct(best_graph, flws))
    	cout << "Schedule is correct." << endl;
	else {
		best_graph.print_procid();
		shed->print(best_graph);
    	cout << "-----------Schedul is not correct!!!-----------" << endl;
    	return 0;
    }

    // shed->print(graph);
    cout << endl << "Best energy: " << best << endl;
    cout << "Best energy: " << shed->get_energy() << endl;

	std::ofstream outfile;
	outfile.open(path_res);
	outfile << std::to_string(best) << endl;
	outfile.close();

	delete shed;

    return 0;
}