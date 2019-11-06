/* Copyright Anton Gerdelan <antongdl@protonmail.com>. 2019
Design:
  7 threads + main thread ( 1 per logical CPU on a typical 4-core machine )
  threads persist until worker_pool_free is called
  threads consume jobs from queue until no jobs exist in queue
  threads halt when finished a job until worker_pool_update() is called to clear finished jobs ( allows on_finished callback on main thread )

Tests: define WORKER_POOL_UNIT_TEST to include a main() running a test program in threads.c
*/

#pragma once
#include <stdbool.h>

typedef struct job_description_t {
  // function with the work to do in the thread
  // int is the worker index used to compute the job
  // void* is job_function_args
  void ( *job_function_ptr )( int, void* );
  // any memory the functions can read/write
  // warning: don't use memory on a stack that's likely to close before the thread ends
  // warning: don't access this memory until the thread is done with it
  // note: to have the thread write into a block of memory use eg a char**
  void* job_function_args;
  // callback called on the main thread to notify program that job has finished.
  // void* is job_function_args
  void ( *on_finished_cb )( const char*, void* );
  // name of the job for display in debug
  char name[16];
} job_description_t;

// initialises reasources and starts all worker threads
void worker_pool_init();

// signals all threads to stop, waits until current jobs done, cleans up threads.
void worker_pool_free();

// returns false if queue was full
bool worker_pool_push_job( job_description_t job );

// returns true if a job existed to be popped from the queue
bool worker_pool_pop_job( job_description_t* job );

// checks for finished jobs
void worker_pool_update();
