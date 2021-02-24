#include <iostream>
#include "sim_mem.h"

using namespace std;

int main()
{

char val;
sim_mem mem_sm("exec_file", "swap_file.txt" ,25, 50, 25,25, 25, 5);

mem_sm.load(25);
mem_sm.load(30);
mem_sm.load(35);
mem_sm.load(40);
mem_sm.load(45);
mem_sm.load(50);
mem_sm.load(55);
mem_sm.load(60);
mem_sm.load(65);
mem_sm.load(70);
mem_sm.load(0);
mem_sm.load(5);
mem_sm.load(10);
mem_sm.load(15);
mem_sm.load(20);
mem_sm.store(75,'A');
mem_sm.store(80,'B');
mem_sm.store(85,'C');
mem_sm.store(90,'D');
mem_sm.store(95,'E');
mem_sm.load(25);





 mem_sm.print_memory();
 mem_sm.print_page_table();
 mem_sm.print_swap();
 mem_sm.print_frame_index();



}