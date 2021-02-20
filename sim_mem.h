
#ifndef SIM_CLASS
#define SIM_CLASS

#include <queue>
using namespace std;
#define MEMORY_SIZE 100
extern char main_memory[MEMORY_SIZE];

typedef struct page_descriptor
{
unsigned int V; // valid
unsigned int D; // dirty
unsigned int P; // permission
unsigned int frame; //the number of a frame if in case it is page-mapped
} page_descriptor;


class sim_mem{

int swapfile_fd;
int program_fd;
int text_size;
int data_size;
int bss_size;
int heap_stack_size;
int num_of_pages;
int page_size;
page_descriptor *page_table;
int * frame_index;
queue <int> swapOrder;
int bring_swap_OR_memory(int fd,char* array,int page,int frame,int offset);
void free_main_memory(char* array);

public:
sim_mem(char const * exe_file_name, char const *  swap_file_name, int text_size, int data_size, int bss_size, int heap_stack_size, int num_of_pages, int page_size);
~sim_mem();
char load(int address);
void store(int address, char value);
void print_memory();
void print_swap ();
void print_page_table();
void print_frame_index();
};
#endif
