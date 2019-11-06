/* Copyright Anton Gerdelan <antongdl@protonmail.com>. 2019 */

#include "threads.h"
#include <assert.h>
#include <pthread.h> // could move into platform file for Windows support
#include <unistd.h>  // not portable TODO
#include <stdio.h>
#include <string.h>
#define MAX_JOBS_IN_QUEUE 64
#define MAX_WORKERS 7
#define WORKER_SLEEP_MICRO_S 100000 // 0.1s

typedef struct worker_t {
  int own_idx;
  pthread_t thread;

  job_description_t job;

  pthread_mutex_t mutex;
  bool shutdown;
  bool finished_job;
} worker_t;

typedef struct job_queue_t {
  pthread_mutex_t mutex;
  job_description_t data[MAX_JOBS_IN_QUEUE];
  int n;
  int start_idx;
  bool shutting_down;
} job_queue_t;

typedef struct worker_pool_t {
  job_queue_t job_queue;
  worker_t workers[MAX_WORKERS];
} worker_pool_t;

static worker_pool_t _g_pool;

static void* _worker_thread_sr( void* arg ) {
  int worker_idx = *( (int*)arg );

  while ( 1 ) {
    // check if threads are shutting down and break loop
    bool shutdown = false;
    bool finished = false;
    pthread_mutex_lock( &_g_pool.workers[worker_idx].mutex );
    shutdown = _g_pool.workers[worker_idx].shutdown;
    finished = _g_pool.workers[worker_idx].finished_job;
    pthread_mutex_unlock( &_g_pool.workers[worker_idx].mutex );
    if ( shutdown ) { break; }
    // wait for main thread to fire acknowledgement callback
    if ( finished ) {
      usleep( WORKER_SLEEP_MICRO_S );
      continue;
    }

    // look for job
    memset( &_g_pool.workers[worker_idx].job, 0, sizeof( job_description_t ) );
    bool got_job = worker_pool_pop_job( &_g_pool.workers[worker_idx].job );
    if ( got_job ) {
      _g_pool.workers[worker_idx].job.job_function_ptr( worker_idx, _g_pool.workers[worker_idx].job.job_function_args );

      pthread_mutex_lock( &_g_pool.workers[worker_idx].mutex );
      _g_pool.workers[worker_idx].finished_job = true;
      pthread_mutex_unlock( &_g_pool.workers[worker_idx].mutex );
      continue;
    }

    // TODO(Anton) wake on signal that a job has entered the queue
    // otherwise sleep
    usleep( WORKER_SLEEP_MICRO_S );
  }
  return NULL;
}

void worker_pool_init() {
  int ret = pthread_mutex_init( &_g_pool.job_queue.mutex, NULL );
  assert( 0 == ret );

  for ( int i = 0; i < MAX_WORKERS; i++ ) {
    _g_pool.workers[i].own_idx = i;
    int ret                    = pthread_create( &_g_pool.workers[i].thread, NULL, _worker_thread_sr, &_g_pool.workers[i].own_idx );
    assert( 0 == ret );
    ret = pthread_mutex_init( &_g_pool.workers[i].mutex, NULL );
    assert( 0 == ret );

    _g_pool.workers[i].own_idx = i;
  }
}

void worker_pool_free() {
  int ret = pthread_mutex_destroy( &_g_pool.job_queue.mutex );
  assert( 0 == ret );

  for ( int i = 0; i < MAX_WORKERS; i++ ) {
    pthread_mutex_lock( &_g_pool.workers[i].mutex );
    _g_pool.workers[i].shutdown = true;
    pthread_mutex_unlock( &_g_pool.workers[i].mutex );

    ret = pthread_join( _g_pool.workers[i].thread, NULL );
    assert( 0 == ret );

    ret = pthread_mutex_destroy( &_g_pool.workers[i].mutex );
    assert( 0 == ret );
  }
}

bool worker_pool_push_job( job_description_t job ) {
  printf( "pushing job `%s` to queue\n", job.name );
  pthread_mutex_lock( &_g_pool.job_queue.mutex );
  {
    if ( _g_pool.job_queue.n >= MAX_JOBS_IN_QUEUE ) { return false; }
    int end_idx                     = ( _g_pool.job_queue.start_idx + _g_pool.job_queue.n ) % MAX_JOBS_IN_QUEUE;
    _g_pool.job_queue.data[end_idx] = job;
    _g_pool.job_queue.n++;
    printf( "jobs queued = %i\n", _g_pool.job_queue.n );
  }
  pthread_mutex_unlock( &_g_pool.job_queue.mutex );
  return true;
}

bool worker_pool_pop_job( job_description_t* job ) {
  assert( job );

  bool got_job = false;

  // fetch job off queue
  pthread_mutex_lock( &_g_pool.job_queue.mutex );
  if ( _g_pool.job_queue.n > 0 ) {
    *job                        = _g_pool.job_queue.data[_g_pool.job_queue.start_idx];
    _g_pool.job_queue.start_idx = ( _g_pool.job_queue.start_idx + 1 ) % MAX_JOBS_IN_QUEUE;
    _g_pool.job_queue.n--;
    got_job = true;
    printf( "job queue is now reduced to %i\n", _g_pool.job_queue.n );
  }
  pthread_mutex_unlock( &_g_pool.job_queue.mutex );

  return got_job;
}

void worker_pool_update() {
  for ( int i = 0; i < MAX_WORKERS; i++ ) {
    bool finished = false;
    pthread_mutex_lock( &_g_pool.workers[i].mutex );
    if ( _g_pool.workers[i].finished_job ) {
      finished                        = true;
      _g_pool.workers[i].finished_job = false; // let thread get another job
    }
    pthread_mutex_unlock( &_g_pool.workers[i].mutex );
    if ( finished ) { _g_pool.workers[i].job.on_finished_cb( _g_pool.workers[i].job.name, _g_pool.workers[i].job.job_function_args ); }
  }
}

#ifdef WORKER_POOL_UNIT_TEST
// unit tests app for threads.c
void test_job_function_ptr( int worker_idx ) {
  printf( "starting job on worker_idx %i\n", worker_idx );
  sleep( 10 );
}

void test_on_finished_cb( const char* name ) { printf( "job `%s` is done -- can access memory it wrote\n", name ); }

int main() {
  worker_pool_init();

  for ( int i = 0; i < 20; i++ ) {
    job_description_t job;
    job.job_function_ptr = test_job_function_ptr;
    job.on_finished_cb   = test_on_finished_cb;
    snprintf( job.name, 16, "job %i", i );
    if ( !worker_pool_push_job( job ) ) { printf( "WARNING: could not queue job %i - queue full\n", i ); }
    sleep( 1 );
    worker_pool_update();
  }

  printf( "freeing worker pool...\n" );
  worker_pool_free();
  printf( "~~Fin!~~\n" );
  return 0;
}
#endif
