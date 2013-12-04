//
//  main.c
//  PageReplacement
//
//  Created by Paulina Rubio Tarriba on 12/4/13.
//  Copyright (c) 2013 Paulina Rubio Tarriba. All rights reserved.
//  Ref. Stackoverflow, geeks for geeks

#include <sys/file.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define MAX_PAGE 0xFF+1


typedef struct
{
    unsigned short frame;
    unsigned int valid:1;
    unsigned int in_mem:1;
    unsigned int dirty:1;
    unsigned int last_frame;
} pt_entry;


struct list_item
{
    unsigned short frame;
    struct list_item *next;
    struct list_item *prev;
    int page_num;
};

typedef struct list_item *list;

void start_simulation(FILE *);
void resolve(int);
unsigned short find_frame(void);
unsigned short find_victim(void);
void display_stats(void);
void to_resident_set(list);
void free_mem(list);
void invalidate(unsigned short);


pt_entry pte[MAX_PAGE];
int mem_size;
list free_list_head;
list res_set_head;
int total_fault = 0;
int total_ref = 0;



int main(int argc, char *argv[])
{
    FILE *stream;
    
    if (argc != 3)
    {
        printf("The format is: pager file_name memory_size.\n");
        exit(1);
    }
    printf("File used %s, resident set size %d\n", argv[1], atoi(argv[2]));
    if ((stream = fopen(argv[1], "r")) == NULL)
    {
        perror("File open failed");
        exit(1);
    }
    
    mem_size = atoi(argv[2]);
    start_simulation(stream);
    fclose(stream);
}



void start_simulation(FILE * stream)
{
    char *addr_buf;
    int address;
    int i, n;
    list new_entry, current;
    
    for(i=0; i<MAX_PAGE;i++)
    {
        pte[i].frame = -1;
        pte[i].valid = 0;
        pte[i].dirty = 0;
        pte[i].in_mem = 0;
    }
    
    res_set_head = (list)malloc(sizeof(struct list_item));
    res_set_head->next = res_set_head;
    res_set_head->prev = res_set_head;
    
    free_list_head = (list)malloc(sizeof(struct list_item));
    free_list_head->next = free_list_head;
    free_list_head->prev = free_list_head;
    current = free_list_head;
    
    for(i=0; i<mem_size;i++)
    {
        new_entry = (list)malloc(sizeof(struct list_item));
        current->next = new_entry;
        new_entry->prev = current;
        new_entry->next = free_list_head;
        new_entry->frame = i;
        current = new_entry;
        free_list_head->prev = current;
    }
    
    while( (n = fscanf(stream, "%x", &address)) != -1)
    {
        resolve(address);
        total_ref++;
    }
    
    free_mem(free_list_head);
    free_mem(res_set_head);
    display_stats();
    return;
}



void resolve(int address)
{
    unsigned short frame_alloc;
    int virt_page;
    static int disp_counter = 0;
    virt_page = address >> 8;
    if (pte[virt_page].valid == 1)
    {
       
    }
    else
    {
        frame_alloc = find_frame();
        pte[virt_page].valid = 1;
        pte[virt_page].frame = frame_alloc;
        total_fault++;
    }
}


unsigned short find_frame()
{
    unsigned short frame;
    list current, new_tail;
    if (free_list_head == free_list_head->prev)   /* free list empty */
        frame = find_victim();
    else
    {
        new_tail = free_list_head->prev->prev;
        new_tail->next = free_list_head;
        current = free_list_head->prev;
        free_list_head->prev = new_tail;
        
        to_resident_set(current);
        frame = current->frame;
    }
    return frame;
}


void to_resident_set(list current)
{
    list tail;
    tail = res_set_head->prev;
    tail->next = current;
    current->next = res_set_head;
    current->prev = tail;
    res_set_head->prev = current;
}

unsigned short find_victim()
{
    int i;
    unsigned short frame=0;
    list current;
    
    for(i=0;i<MAX_PAGE;i++)
    {
        if (pte[i].frame == frame && pte[i].valid == 1)
        {
            frame = res_set_head->next->frame;
            invalidate(frame);
            current = res_set_head->next;
            res_set_head->next = current->next;
            res_set_head->next->prev = res_set_head;
            to_resident_set(current);
            break;
        }
    }
    return frame;
}



void invalidate(unsigned short frame)
{
    int i;
    
    for(i=0;i<MAX_PAGE;i++)
    {
        if (pte[i].frame == frame && pte[i].valid == 1)
        {
            pte[i].valid = 0;
            pte[i].frame = -1;
            break;
        }
        
    }
}

void display_stats()
{
    printf("\nProcess issued %d memory references\n", total_ref);
    printf("Process triggered %d page faults\n", total_fault);
    printf("Pafe fault rate is %d percent\n",((total_fault*100)/total_ref));
}


void free_mem(list head)
{
    list current,tail;
    tail = head->prev;
    current = head;
    while (current->prev != tail)
    {
        current = current->next;
        free(current->prev);
    }
}
