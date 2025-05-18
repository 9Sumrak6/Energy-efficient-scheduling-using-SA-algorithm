# Energy-efficient scheduling using SA algorithm
Implementation of Energy-efficient scheduling using SA algorithm in C++.

Input and output examples are given in "Input" and "Output" directories.
Although multiple examples of graphics are given in "Graphics" directory.

***Minimum system requirements***:
```
c++17
MPI version >= 4.0
Python 3.10
```
## Launch the programs
The project contains:
1. consecutive SA algorithm - "main.cpp";
2. parallel SA algorithm using MPI - "mpi_main.cpp";
3. parallel SA algorithm using MPI - "fork_main.cpp";
4. input files generator - "inp_gen.cpp"
To compile programs run these commands:
```Python
1. source env/bin/activate - to atcivate the python environment for python scripts; 
2. make - to compile all programs;
3. make [cons|mpi|fork|generator] - to compile chosen programs;
4. make clean - to delete all executable files;
5. python3 compare.py - runs a script that builds graphs.
```
