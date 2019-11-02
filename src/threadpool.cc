/*
 * Name: Liyao Jiang  
 * ID: 1512446
 */
#include "threadpool.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <queue>

using namespace std;

/**
* A C style constructor for creating a new ThreadPool object
* Parameters:
*     num - The number of threads to create
* Return:
*     ThreadPool_t* - The pointer to the newly created ThreadPool object
*/
ThreadPool_t *ThreadPool_create(int num)
{
    ThreadPool_t *threadpool = new ThreadPool_t; // new object of the Threadpool struct
    threadpool->on_destroy = false;

    // invalid number of threads
    if (num <= 0)
    {
        return NULL;
    }

    threadpool->number_of_works = 0;
    // init the threads and number of threads
    threadpool->number_of_threads = 0;
    threadpool->threads = new pthread_t[num];

    // init the condition variable and mutex
    pthread_cond_init(&(threadpool->cond), NULL);
    pthread_mutex_init(&(threadpool->mutex), NULL);

    // create the threads
    for (int i = 0; i < num; i++)
    {
        pthread_create(&(threadpool->threads[i]), NULL, Thread_run, threadpool);
        threadpool->number_of_threads += 1; //increment the number of threads
    }
    return threadpool;
}

/**
* A C style destructor to destroy a ThreadPool object
* Parameters:
*     tp - The pointer to the ThreadPool object to be destroyed
*/
void ThreadPool_destroy(ThreadPool_t *tp)
{
    sleep(1);
    while (tp->number_of_works != 0)
        ;

    // threads will know we are on destroy now
    tp->on_destroy = true;
    pthread_cond_broadcast(&(tp->cond));

    // wait for threads to exit
    for (int i = 0; i < tp->number_of_threads; i++)
    {
        pthread_join(tp->threads[i], NULL);
    }

    // clean up
    if (tp->threads)
    {
        delete[] tp->threads;
    }

    pthread_cond_destroy(&(tp->cond));
    pthread_mutex_destroy(&(tp->mutex));

    delete tp;
}

/**
* Add a task to the ThreadPool's task queue
* Parameters:
*     tp   - The ThreadPool object to add the task to
*     func - The function pointer that will be called in the thread
*     arg  - The arguments for the function
* Return:
*     true  - If successful
*     false - Otherwise
*/
bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg)
{
    try
    {
        // create the work object from arguments
        ThreadPool_work_t *work = new ThreadPool_work_t;
        work->func = func;
        work->arg = arg;
        pthread_mutex_lock(&(tp->mutex));
        // push pointer to the work onto the work queue FIFO
        tp->work_queue.queue.push(work);
        if (tp->number_of_works == 0)
        {
            // if some thread is waiting for work, signal it
            tp->number_of_works += 1;
            pthread_cond_signal(&(tp->cond));
        }
        else
        {
            tp->number_of_works += 1;
        }

        pthread_mutex_unlock(&(tp->mutex));
        // work adding successful
        return true;
    }
    catch (bad_alloc)
    {
        // work adding failed
        return false;
    }
}

/**
* Get a task from the given ThreadPool object
* Parameters:
*     tp - The ThreadPool object being passed
* Return:
*     ThreadPool_work_t* - The next task to run
*/
ThreadPool_work_t *ThreadPool_get_work(ThreadPool_t *tp)
{
    // unless we get a work from the queque, otherwise there is no work
    ThreadPool_work_t *work = NULL;
    pthread_mutex_lock(&(tp->mutex));
    // wait work cond signal if there is no work
    if (tp->number_of_works == 0)
    {
        pthread_cond_wait(&(tp->cond), &(tp->mutex));
    }
    // check if there is actual work
    if (tp->number_of_works > 0)
    {
        work = tp->work_queue.queue.front();
        // need to pop the work from queue
        tp->work_queue.queue.pop();
        tp->number_of_works -= 1;
    }
    // otherwise the signal just means threadpool is on destory, just return NULL
    pthread_mutex_unlock(&(tp->mutex));
    return work;
}

/**
* Run the next task from the task queue
* Parameters:
*     tp - The ThreadPool Object this thread belongs to
*/
void *Thread_run(void *void_tp)
{
    ThreadPool_t *tp = (ThreadPool_t *)void_tp;
    ThreadPool_work_t *work;
    // continue to get work and work until on destroy
    while (!tp->on_destroy)
    {
        // get the work
        work = ThreadPool_get_work(tp);
        // do the work
        if (work != NULL)
        {
            work->func(work->arg);
            // delete the work after done
            delete work;
        }
    }
    // on destory, exit the thread
    pthread_exit(NULL);
    return NULL;
}