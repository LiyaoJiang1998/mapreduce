# MapReduce Library

**Name: Liyao Jiang**
- - -

## Implementations and Analysis

- - -

The program is written in C++, I implemented the mapreduce.cc and threadpool.cc files according to the requirements in the .h files.

**Note:** I change the Thread_run function signature in threadpool.h: since the thread_func_t takes input parameter of type void *, I changed it from `void *Thread_run(ThreadPool_t *tp);` to `void *Thread_run(void *void_tp);`

- ### Explanation of global variables

  - mapreduce.cc:
    - `int num_reducers_global;` // global variable that stores the number of reducers
    - `Reducer concate_global;` // global that stores the reducer function so it can be called in MR_ProcessPartition function
    - `vector<multimap<string, string> > *partitions;` // global pointer to intermediate data structure, a vector of multimap of string, string pairs, multimap is to be sorted in ascending order by default
    - `pthread_mutex_t *partition_mutexes;` // fine grain mutex locks, one for each partition

- ### How to store the intermediate key/value pairs
  
  - I used a vector of multimap of (string,string) pairs, stored the pointer to the vector as a global variable
  - the vector length is set to be the number of reducers R, which is also the number of partitions needed, each partition multimap, stores a pair of key and value strings.
  - since the key/value pairs should be order based on ascending key order, this is achieved because the multimap sorts the pairs with ascending order by default

- ### The time complexity of MR Emit and MR GetNext functions

  - MR Emit: O(log(n))
    - since this function calls the multimap insert method, which we used to insert a single key/value pair to the partition multimap, and the insert method has a running time complexity of log(size of multimap) when inserting a single element. So overall, O(log(n)) (where n is the size of partition multimap)
  - MR GetNext: O(1)
    - since the ith call to this function should return the ith value associated with the key in the sorted partition or if i is greater than the number of values associated with the key.NULL, and only one thread is accessing that partition, the key value pair if exist will always be at the begin of the multipmap, so multimap.find method will take O(1) time, and the erase method takes O(1) as well. So overall, O(1)

- ### The data structure used to implement the task queue in the thread pool library

  - I used `std::queue<ThreadPool_work_t*>` which stores the works and is FIFO.
  - Since the required scheduling is longest job first, inside the mapreduce library, I added all the filenames to a `priority_queue<pair<int, char *> >`, I used stat to get the file size, and the filenames are added to the priority queue with their file sizes. When using ThreadPool_add_work to add the tasks to the threadpool work queue, I loop to take the longest job (i.e largest file) from the top of the priority queue and add to the task queue. So the longest job first scheduling policy is achieved.

- ### Implementation of the thread pool library

  - the thread pool struct has members:
    - `pthread_mutex_t mutex;` // global mutex lock, lock the work queue before adding or getting work from it.
    - `pthread_cond_t cond;` // global conditional variable, inside ThreadPool_get_work function, it will wait from signal if the work queue is empty, and inside ThreadPool_add_work function, it signals one thread that is waiting that now there is work.
    - When we are on destroy using ThreadPool_destroy function, I designed the master thread will broadcast the conditional variable, so every thread will try to get work from work queue, however, there is no work queue, and since on_destroy flag is set to true, the threads will exit. The master threads waits for all the threads to exit, and the clean up and exit.
    - `bool on_destroy;` // boolean flag, set to true inside ThreadPool_destroy function, when the job queue is empty which means all jobs are assigned to some thread, and the master threads broadcast the conditional variable, so every thread will try to get work from work queue, however, there is no work queue, and since on_destroy flag is set to true, the threads will exit.
    - `pthread_t *threads;` // points to the array of threads created using pthread_create
    - `int number_of_threads;` // keeps track of the number of threads
    - `ThreadPool_work_queue_t work_queue;` // the work queue which has a member std::queue that holds the works FIFO
    - `int number_of_works;` // keeps track of the number of works
    - each thread will continue to get task form work queue, and work on the work in a loop.

- ### Synchronization primitives used and test of correctness

  - threadpool.cc
    - **a global mutex lock:**  
    global mutex lock, lock the work queue before adding or getting work from it, so each work will only be worked on by one thread, and each work is only added to the work queue once.
    - **a global conditional variable:**  
    global conditional variable, inside ThreadPool_get_work function, it will wait from signal if the work queue is empty, and inside ThreadPool_add_work function, it signals one thread that is waiting that now there is work.
    - When we are on destroy using ThreadPool_destroy function, I designed the master thread will broadcast the conditional variable, so every thread will try to get work from work queue, however, there is no work queue, and since on_destroy flag is set to true, the threads will exit. The master threads waits for all the threads to exit, and the clean up and exit.
    - so when a thread getting work, it will either get a work, or wait for work, or exit when on destroy.

  - mapreduce.cc
    - **fine grain mutex locks:**  
    each partition has a mutex lock, so when a thread access the partition, only that partition will be locked, the other threads can still access other partitions at the same time safely, this increase the efficiency of the program. Since the each thread will try to obtain the corresponding lock to the partition before accessing that partition and unlock afterwards, the integrity of the data is protected.

## Testing

- - -

- I tested the code by running the sample test cases provided on eclass with different number of mappers and reducers in distwc.c, and compared the expected results. Also, I used `valgrind --tool=memcheck --leak-check=yes` when running the sample test cases in the wordcount program, no memory leaks.

- I tested my program using the tests found on eclass course discussion forum with level easy, medium, hard, hell-incarnate and passed all the tests.

## Acknowledgement

- - -

The distwc.c, mapreduce.h, threadpool.h are provided by the start code on eclass. The Makefile I wrote references the Makefile example in LAB 1.

Referenced <https://www.geeksforgeeks.org/priority-queue-of-pairs-in-c-ordered-by-first/> on how to use priority queue

Referenced <https://linux.die.net/man/2/stat> on how to get filesize

Referenced <http://www.cplusplus.com/reference/map/multimap/> on use of multiomap