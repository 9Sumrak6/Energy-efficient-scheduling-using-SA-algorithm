all:
	g++ inp_gen.cpp -o inp_gen -std=gnu++17 -W -Wall -Werror 
	g++ main.cpp -o main -std=gnu++17 -W -Wall -Werror -Ofast