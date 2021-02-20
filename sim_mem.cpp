#include "sim_mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h> 

char main_memory[MEMORY_SIZE];

// Constructor
sim_mem::sim_mem(char const * exe_file_name, char const * swap_file_name, int text_size, int data_size, int bss_size, int heap_stack_size, int num_of_pages, int page_size)
{

    program_fd=open(exe_file_name,O_RDONLY);
    
    if(program_fd==-1)
    {
        perror("Cant open exe file");
        exit(1);
    }    
    swapfile_fd=open(swap_file_name,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

    if(swapfile_fd==-1){
        perror("Cant open exe file");
        exit(1);    
    }

    this->text_size=text_size;
    this->data_size=data_size;
    this->bss_size=bss_size;
    this->heap_stack_size=heap_stack_size;
    this->num_of_pages=num_of_pages;
    this->page_size=page_size;

    page_table=(page_descriptor*)malloc(sizeof(page_descriptor)*num_of_pages);

    if(page_table==NULL)
    {
        perror("Cannot allocate memory");
        exit(1);
    }

    int i;
    for(i=0;i<MEMORY_SIZE;i++)
        main_memory[i]='0';
    
    for(i=0;i<num_of_pages*page_size;i++)
        write(swapfile_fd,"0",1);

    for(i=0;i<num_of_pages;i++)
    {
        if(i<(text_size/page_size))
        {         
            page_table[i].P=0;
        }
        else
        {
            page_table[i].P=1;
        }
        page_table[i].V=0;
        page_table[i].D=0;
        page_table[i].frame=-1;
    }

    frame_index=(int*)malloc(sizeof(int)*(MEMORY_SIZE/page_size));
    if(frame_index==NULL)
    {
        perror("Cannot allocate memory");
        exit(1);
    }

    for(i=0;i<(MEMORY_SIZE/page_size);i++)
        frame_index[i]=0;       
}

// Destructor
sim_mem::~sim_mem()
{
    free(page_table);
    free(frame_index);
    close(program_fd);
    close(swapfile_fd);
}

// load
char sim_mem::load( int address)
{

    if(address<0 || address>=(num_of_pages*page_size))
    {
        perror("Address is invalid");
        return '\0';
    }
    int i;
    int page=address/page_size;
    int offset=address%page_size;
    int frame=-1;
    int physical_address=-1;

    // page in memory
    if(page_table[page].V==1)
    {
        frame=page_table[page].frame;
        physical_address=frame*page_size+offset;
        return main_memory[physical_address];
    }

    // page not in memory
    else if(page_table[page].V==0)
    {
        char * buff=(char*)malloc(sizeof(char)*page_size);

        if(buff==NULL)
            {
                perror("Cannot allocate memory");
                exit(1);
            }

        // page not in memory and page in 'text'
        if(page_table[page].P==0)
        {                      
            physical_address=bring_swap_OR_memory(program_fd,buff,page,frame,offset);                       
        }
        // page not in memory and page not in 'text'
        else if (page_table[page].P==1)
        {

            // not "dirty" page
            if(page_table[page].D==0)
            {
                // in data
                if(address>=text_size && address<text_size+data_size)
                {
                    physical_address=bring_swap_OR_memory(program_fd,buff,page,frame,offset);
                }

                // in bss/heap/stack but not allocated
                else if(address>=text_size+data_size)
                {
                    perror("Cannot load unallocated memory");
                    free(buff);
                    return '\0';
                }
            }

            // page is "dirty"
            else if(page_table[page].D==1)
            {
               physical_address=bring_swap_OR_memory(swapfile_fd,buff,page,frame,offset); 
            }
        }
    
        //check if memory full
        for(i=0;i<MEMORY_SIZE/page_size;i++)
            if(frame_index[i]==0)
                break;    
    
        //memory full
        if(i==MEMORY_SIZE/page_size)
        {
            int toSwap=swapOrder.front();
            swapOrder.pop();

            if(toSwap*page_size>=text_size)
            {
                strncpy(buff,(main_memory+(page_table[toSwap].frame*page_size)),page_size);
                lseek(swapfile_fd,toSwap*page_size,SEEK_SET);
                write(swapfile_fd,buff,page_size);
                page_table[toSwap].D=1;   
            }

            for(i=page_table[toSwap].frame*page_size;i<page_table[toSwap].frame*page_size+page_size;i++)
                main_memory[i]='0';

            page_table[toSwap].V=0;
            frame_index[page_table[toSwap].frame]=0;
            page_table[toSwap].frame=-1;             
        }

        free_main_memory(buff);    
        free(buff);
    }

return main_memory[physical_address];
}

// store
void sim_mem::store(int address, char value)
{

    if(address<0 || address>=(num_of_pages*page_size))
    {
        perror("Address is invalid");
        return;
    }
    int i;
    int page=address/page_size;
    int offset=address%page_size;
    int frame=-1;
    int physical_address=-1;

    if(page_table[page].P==0)
    {
        perror("Cannot change text");
        return;
    }

    // page in memory
    if(page_table[page].V==1)
    {
        frame=page_table[page].frame;
        page_table[page].D=1;
        physical_address=frame*page_size+offset;
        main_memory[physical_address]=value;
    }

    // page not in memory
    else if(page_table[page].V==0)
    {
        char * buff=(char*)malloc(sizeof(char)*page_size);

        if(buff==NULL)
            {
                perror("Cannot allocate memory");
                exit(1);
            }

        // not "dirty" page
        if(page_table[page].D==0)
        {
            // in data
            if(address>=text_size && address<text_size+data_size)
            {
                physical_address=bring_swap_OR_memory(program_fd,buff,page,frame,offset);
                page_table[page].D=1;
                main_memory[physical_address]=value;                                                     
            }

            // in bss/heap/stack but not allocated
            else if(address>=text_size+data_size)
            {                   
                for(i=0;i<page_size;i++)
                    buff[i]='0';  
               
                page_table[page].D=1;
                physical_address=bring_swap_OR_memory(-1,buff,page,frame,offset);
                main_memory[physical_address]=value;                                      
            }
        }

        // page is "dirty"
        else if(page_table[page].D==1)
        {
            physical_address=bring_swap_OR_memory(swapfile_fd,buff,page,frame,offset);
            main_memory[physical_address]=value;
        } 

        free_main_memory(buff);
        free(buff);
    }
}

// print memory
void sim_mem::print_memory() 
{ 
int i; 
printf("\nPhysical memory\n"); 

for(i = 0; i < MEMORY_SIZE; i++) 
    printf("[%c]\n", main_memory[i]); 
    
}


// print swap
void sim_mem::print_swap() 
{ 
char* str =(char*)malloc(this->page_size *sizeof(char)); 
int i;
printf("\nSwap memory\n"); 
lseek(swapfile_fd, 0, SEEK_SET);
    while(read(swapfile_fd, str, this->page_size) == this->page_size) 
    { 
        for(i = 0; i < page_size; i++)
            printf("%d - [%c]\t", i, str[i]);
        
        printf("\n"); 
    }
     free(str);
}


// print page table
void sim_mem::print_page_table() 
{ 
int i; 
printf("\npage table \n");
printf("Valid\t Dirty\t Permission \t Frame\n"); 
    for(i = 0; i < num_of_pages; i++) 
    { 
        printf("[%d]\t[%d]\t[%d]\t[%d]\n", 
        page_table[i].V, 
        page_table[i].D, 
        page_table[i].P, 
        page_table[i].frame); 
    }
}


// print frame index
void sim_mem :: print_frame_index()
{
 int i; 
printf("\nFrame index\n"); 

for(i = 0; i < MEMORY_SIZE/page_size; i++) 
    printf("[%d]\n",frame_index[i]); 

}

// method returns the physical address from swap or memory and push page to queue
int sim_mem::bring_swap_OR_memory(int fd,char * array,int page,int frame,int offset){

    if(fd!=-1)
    {
    lseek(fd,page*page_size,SEEK_SET);
    read(fd,array,page_size);
    }
    int i;
    int physical_address;

    for(i=0;i<MEMORY_SIZE/page_size;i++)

        if(frame_index[i]==0)
        {
            frame_index[i]=1;
            strncpy((main_memory+(i*page_size)),array,page_size);
            page_table[page].V=1;
            page_table[page].frame=i;
            frame=page_table[page].frame;
            physical_address=frame*page_size+offset;
            swapOrder.push(page);
            break;                  
        }   
    return physical_address; 
}

// check if main memory is full and send one page to swap in needed
void sim_mem :: free_main_memory(char* buff)
{
    int i;
    //check if memory full
        for(i=0;i<MEMORY_SIZE/page_size;i++)
            if(frame_index[i]==0)
                break;    

        //memory full
        if(i==MEMORY_SIZE/page_size)
        {
            int toSwap=swapOrder.front();
            swapOrder.pop();

            if(toSwap*page_size>=text_size)
            {
                strncpy(buff,(main_memory+(page_table[toSwap].frame*page_size)),page_size);
                lseek(swapfile_fd,toSwap*page_size,SEEK_SET);
                write(swapfile_fd,buff,page_size);
                page_table[toSwap].D=1;   
            }

            for(i=page_table[toSwap].frame*page_size;i<page_table[toSwap].frame*page_size+page_size;i++)
                main_memory[i]='0';

            page_table[toSwap].V=0;
            frame_index[page_table[toSwap].frame]=0;
            page_table[toSwap].frame=-1;                
        }    
}