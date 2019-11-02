/*
 * Name: Liyao Jiang  
 * ID: 1512446
 */
#include "mapreduce.h"
#include "threadpool.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sys/stat.h>
#include <bits/stdc++.h>
#include <map>

using namespace std;

int num_reducers_global; // global number of reducers
Reducer concate_global; // global the reduce function

vector<multimap<string, string> > *partitions; // global pointer to intermediate data structure, multimap default to sorted in ascending order
pthread_mutex_t *partition_mutexes; // fine grain mutexes for each partition

/**
 * make the MR_ProcessPartition defined in mapreduce.h to be a thread_func_t
 */
void *MR_ProcessPartition_thread_func(void *num)
{
    int partition_num = *((int *)num);
    MR_ProcessPartition(partition_num);
    delete (int *)num;
    pthread_exit(NULL);
    return NULL;
}

/**
 * starting point of the mapreduce
 */
void MR_Run(int num_files, char *filenames[],
            Mapper map, int num_mappers,
            Reducer concate, int num_reducers)
{
    // initialize the resourse needed
    partitions = new vector<multimap<string, string> >(num_reducers);
    concate_global = concate;
    num_reducers_global = num_reducers;
    partition_mutexes = new pthread_mutex_t[num_reducers_global];
    for (int i = 0; i < num_reducers_global; i++)
    {
        pthread_mutex_init(&(partition_mutexes[i]), NULL);
    }

    // adding to max priority queue base on file size
    priority_queue<pair<int, char *> > filenames_pq;
    struct stat filesize_buf;
    for (int i = 0; i < num_files; i++)
    {
        stat(filenames[i], &filesize_buf);
        filenames_pq.push(make_pair(filesize_buf.st_size, filenames[i]));
    }

    // creating mappers
    ThreadPool_t *tp_mapper = ThreadPool_create(num_mappers);
    while (!filenames_pq.empty())
    {
        while (!ThreadPool_add_work(tp_mapper, (thread_func_t)map, filenames_pq.top().second))
            ;
        filenames_pq.pop();
    }
    ThreadPool_destroy(tp_mapper);

    // creating reducers
    pthread_t *reducer_threads = new pthread_t[num_reducers_global];
    for (int i = 0; i < num_reducers_global; i++)
    {
        // the for loop i will change, make a copy of it
        int *arg = new int;
        *arg = i;
        pthread_create(&(reducer_threads[i]), NULL, &MR_ProcessPartition_thread_func, (void *)arg);
    }
    sleep(1);

    // wait for reducer threads to exit
    for (int i = 0; i < num_reducers_global; i++)
    {
        pthread_join(reducer_threads[i], NULL);
    }

    // clean up
    delete[] reducer_threads;
    // free up partition mutexes
    for (int i = 0; i < num_reducers_global; i++)
    {
        pthread_mutex_destroy(&(partition_mutexes[i]));
    }
    delete[] partition_mutexes;
    delete partitions;
}

/**
 * MR Emit library function takes a key and a value associated with it,
 * and writes this pair to a specific partition which is determined by 
 * passing the key to the MR Partition library function.
 */
void MR_Emit(char *key, char *value)
{
    // cast to c++ string
    string key_string = key;
    string value_string = value;
    // find out which partition to insert
    unsigned long partition_number = MR_Partition(key, num_reducers_global);
    // fine grain lock, only the partition that we are accessing
    pthread_mutex_lock(&(partition_mutexes[partition_number]));
    (*partitions)[partition_number].insert(pair<string, string>(key_string, value_string));
    pthread_mutex_unlock(&(partition_mutexes[partition_number]));
}

/**
 * Hash Function
 * Citing: Using the DJB2 hash function from the assignment2 description
 */
unsigned long MR_Partition(char *key, int num_partitions)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
    {
        hash = hash * 33 + c;
    }
    return hash % num_partitions;
}

/**
 * The MR ProcessPartition library function takes the index of the partition
 * assigned to the thread that runs it. It invokes the user-defined Reduce function
 * in a loop, each time passing it the next unprocessed key. This continues until
 * all keys in the partition are processed.
 */
void MR_ProcessPartition(int partition_number)
{
    pthread_mutex_lock(&(partition_mutexes[partition_number]));
    // run until all key is processed
    while (!(*partitions)[partition_number].empty())
    {
        // copy and cast the string to char array
        char key_str[1 + (*partitions)[partition_number].begin()->first.length()];
        strcpy(key_str, (*partitions)[partition_number].begin()->first.c_str());
        // run the concate_global reducer function with the key at begin
        concate_global(key_str, partition_number);
    }
    pthread_mutex_unlock(&(partition_mutexes[partition_number]));
}

/**
 * library function takes a key and a partition number, and returns a value associated with
 * the key MR GetNext that exists in that partition. In particular, the ith call to this function 
 * should return the ith value associated with the key in the sorted partition or if i is greater than 
 * the number of values associated with the key.NULL
 */
char *MR_GetNext(char *key, int partition_number)
{
    // cast to c++ string
    string key_string = key;
    multimap<string, string>::iterator iterator = (*partitions)[partition_number].find(key_string);
    if (iterator != (*partitions)[partition_number].end())
    {
        // copy and cast the string to char array
        (*partitions)[partition_number].erase(iterator);
        // return const_cast<char *>(iterator->second.c_str());
        return key;
    }
    return NULL;
}