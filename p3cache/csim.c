#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <getopt.h>
#include "cachelab.h"

int hits, misses, evictions;
typedef struct
{
    unsigned long address;
    int size;
    char mode;
}traceline;


typedef struct linkedlist queue;
struct linkedlist
{
    int val;
    queue *next;
};

void free_queue(queue *x)
{
    free(x->next);
    free(x);
}

queue *new_queue(int val, queue *next)
{
    queue *result = (queue*)malloc(sizeof(queue));
    result->val = val;
    result->next = next;
    return result;
}

traceline *newtracelinep(char mode, unsigned long address, int size)
{
    traceline *toreturn = (traceline*)malloc(sizeof(traceline));
    toreturn->mode = mode;
    toreturn->address = address;
    toreturn->size = size;
    return toreturn;
}

queue *delete_index(queue *result, int tbdeleted)
{
    //printf("here\n");
    queue *temp = result;
    queue *prev = result;
    if(temp != NULL && temp->val == tbdeleted)
    {
        result = temp->next;
        free(temp);
        return result;
    }
    while(temp != NULL && temp->val != tbdeleted)
    {
        prev = temp;
        temp = temp->next;
    }
    if(temp == NULL)
        return result;
    prev->next = temp->next;
    free(temp);
    return result;
}

int delete_last(queue *aqueue)
{
    queue *todelete = aqueue;
    queue *newlast = aqueue;
    int last = todelete->val;
    if(aqueue == NULL)
        return last;
    while(todelete->next != NULL)
    {
        newlast = todelete;
        todelete = todelete->next;
        last = todelete->val;
    }
  //  printf("ehuadskjn\n");
    if(todelete == aqueue)
        aqueue = NULL;
    else
        newlast->next = NULL;
    return last;
}

void printqueue(queue *aqueue)
{
    queue *temp = aqueue;
    while(temp != NULL)
    {
        printf("%d, ", temp->val);
        temp = temp->next;
    }
    printf("\n");
}

// how many stored vs how many want to load
void simuhelper(char* mode, unsigned int setid, unsigned int tag, queue** setlrus, unsigned int **cache, int e)
{
    //printqueue(setlrus[setid]);

    int modwrite_index;

    printf("    %s, setid: %d, tag: %X\n", mode, setid, tag);// address: %X / size: %d\n", curaddress, cursize);
    int k;
    for(k = 0; k < e; k++)
    {
        printf("    cache[%d][%d] is %X\n", setid, k, cache[setid][k]);
        if(cache[setid][k] == tag)
        {
            hits++;
            printf("    %s hit! set: %d line: %d tag: %X\n", mode, setid, k, tag);
            modwrite_index = k;

            
            if(setlrus[setid] == NULL)
            {
               // printf("akjdsfnacsd\n");
                free(setlrus[setid]);
                setlrus[setid] = new_queue(k, NULL); 
               // printqueue(setlrus[setid]);
            }
            else
            {
              
                setlrus[setid] = new_queue(k, delete_index(setlrus[setid], k));//setlrus[setid]);
                //printqueue(setlrus[setid]);
            }

           // printqueue(setlrus[setid]);
            

            break;
        }
        else if( cache[setid][k] == INT_MIN)
        {
            cache[setid][k] = tag;
            misses++; 
            printf("    %s miss! now, set: %d line: %d tag: %X\n", mode, setid, k, tag);
            modwrite_index = k;

            //setlrus[setid] = new_queue(k, setlrus[setid]);
            if(setlrus[setid] == NULL)
            {
                free(setlrus[setid]);
                setlrus[setid] = new_queue(k, NULL); 
            }
            else
            {
                setlrus[setid] = new_queue(k, delete_index(setlrus[setid], k));//setlrus[setid]);
            }
            break;
        }
                
    }
    if(k >= e)
    {    
        int evict_index = delete_last(setlrus[setid]);
        setlrus[setid] = new_queue(evict_index, setlrus[setid]);
        int evicted = cache[setid][evict_index];
        cache[setid][evict_index] = tag;
        misses++;
        evictions++;
        modwrite_index = evict_index;
        printf("    %s miss eviction! set: %d lru_index: %d, evicted tag: %X new tag: %X\n", mode, setid, evict_index, evicted, tag);

    }
    if((strcmp(mode, "modify")==0) && (cache[setid][modwrite_index] == tag))
    {
        hits++;
        printf("    mod write hit! set: %d line: %d tag: %X\n", setid, k, tag);
    }


}


void simulator(traceline **actions, int s, int e, int b) // **actions
{
    int numsets = pow(2, s);
   // int blocksize = pow(2, b);  //incorporate later
    int i = 0;
    char curmode;
    unsigned int cursize, tag, setid; //cursize, offset
    unsigned long curaddress;

    queue **setlrus = calloc(numsets, sizeof(queue*));

    unsigned int **cache = (unsigned int**)malloc(sizeof(unsigned int*)*numsets);
    for(int i = 0; i < numsets; i++)
    {
        cache[i] = (unsigned int*)malloc(sizeof(unsigned int)*e);
        for(int j = 0; j < e; j++)
        {
            cache[i][j] = INT_MIN;
        }
    }

    while(actions[i] != NULL)
    {
        curmode = actions[i]->mode;
        curaddress = actions[i]->address; 
        cursize = actions[i]->size; // num bytes accessed

        // constant form of the address, extract info from the address
        tag = curaddress>>(s+b);
        setid = (curaddress>>b)&(numsets-1);


        if(curmode != 'I')
            printf("%c %lx, %d\n", curmode, curaddress, cursize);
        if(curmode == 'L') // read
        {
            simuhelper("read", setid, tag, setlrus, cache, e);
        }
        else if(curmode == 'S') // write
        {
            simuhelper("write", setid, tag, setlrus, cache, e);
        }
        
        else if(curmode == 'M') // modify, read then write
        {
            simuhelper("modify", setid, tag, setlrus, cache, e);
        }
        i++;
    }
    i = 0;
    while(setlrus[i] != NULL && cache[i] != NULL)
    {
        free_queue(setlrus[i]);
        free(cache[i]);
        i++;
    }
    free(setlrus);
    free(cache);
}



int main(int argc, char *argv[])
{
    int c;
    int s, e, b;
    char* tfpath = "";

    while( (c = getopt(argc, argv, "s:E:b:t:")) != -1 )
    {
        switch(c)
        {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                e = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                tfpath = optarg; 
        }
    }
    traceline **cache_actions = (traceline**)malloc(sizeof(traceline*)); // list of actions; array of pointers to trace line structs

    int size;
    unsigned long address;
    char mode;
    FILE *tracefile = fopen(tfpath, "r");
    if(tracefile == NULL)
    {
        fprintf(stderr, "this trace doesn't exist!\n");
        exit(1);
    }
    int i = 0;
    while( (fscanf(tracefile, " %c %lx,%d\n", &mode, &address, &size) != EOF))// || (fscanf(tracefile, "%c  %X,%d\n", &mode, &address, &size) != EOF) )
    {
        cache_actions[i] = newtracelinep(mode, address, size); // new memory allocated traceline struct
        i++;
        cache_actions = (traceline**)realloc(cache_actions, (sizeof(traceline*)*(i+1))); // reallocating 
    }
    i = 0;
    simulator(cache_actions, s, e, b);
    printSummary(hits, misses, evictions);
    while(cache_actions[i] != NULL)
    {
        free(cache_actions[i]);
        i++;
    }
    free(cache_actions[i]);
    free(cache_actions);
    return 0;
}
