# Компиляторы
CXX := g++
MPICXX := mpic++

# Флаги
CXXFLAGS := -std=c++17
MPI_CXXFLAGS := -std=c++23 -O3

# Исходники и цели
INP_GEN_SRC := inp_gen.cpp
INP_GEN_EXE := inp_gen

MAIN_SRC := main.cpp
MAIN_EXE := cons_main

FORK_SRC := fork_main.cpp
FORK_EXE := fork_main

MPI_SRC := mpi_main.cpp
MPI_EXE := mpi_main

# По умолчанию комиплируем все
all: $(MAIN_EXE) $(FORK_EXE) $(INP_GEN_EXE) $(MPI_EXE)

# Обычная программа
$(MAIN_EXE): $(MAIN_SRC)
	$(CXX) $(CXXFLAGS) -o $(MAIN_EXE) $(MAIN_SRC)

# Ветка с fork
$(FORK_EXE): $(FORK_SRC)
	$(CXX) $(CXXFLAGS) -o $(FORK_EXE) $(FORK_SRC)

# Ветка с fork
$(INP_GEN_EXE): $(INP_GEN_SRC)
	$(CXX) $(CXXFLAGS) -o $(INP_GEN_EXE) $(INP_GEN_SRC)

# MPI-программа
$(MPI_EXE): $(MPI_SRC)
	$(MPICXX) $(MPI_CXXFLAGS) -o $(MPI_EXE) $(MPI_SRC)

# Фасадные цели для make cons, make fork, make inp_gen, make mpi
cons: $(MAIN_EXE)
fork: $(FORK_EXE)
generator: $(INP_GEN_EXE)
mpi: $(MPI_EXE)

# Очистка
clean:
	rm -f $(MAIN_EXE) $(FORK_EXE) $(INP_GEN_EXE) $(MPI_EXE)