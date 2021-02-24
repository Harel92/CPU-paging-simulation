all: main.cpp
	g++ main.cpp sim_mem.cpp -o main
all-GDB: main.cpp
	g++ -g main.cpp sim_mem.cpp -o main