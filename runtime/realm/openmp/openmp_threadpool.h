/* Copyright 2020 Stanford University, NVIDIA Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// OpenMP (or similar) thread pool for Realm
#ifndef REALM_OPENMP_THREADPOOL_H
#define REALM_OPENMP_THREADPOOL_H

#include "realm/threads.h"

namespace Realm {

  class LoopSchedule {
  public:
    // sets the number of workers and initializes the barrier for usage
    //  by a work item
    void initialize(int _num_workers);

    // starts a static loop, blocking if the previous loop in the
    //  work item has any stragglers - returns true if there's work
    //  to do, false if not
    bool start_static(int64_t start, int64_t end,
		      int64_t incr, int64_t chunk,
		      int thread_id,
		      int64_t& span_start, int64_t& span_end);

    // continues a static loop - span_{start,end} must contain what
    //  they were given from the previous call to start/next_static
    bool next_static(int64_t& span_start, int64_t& span_end);

    // starts a dynamic loop, blocking if the previous loop in the
    //  work item has any stragglers - does not actually request any
    //  work - use next_dynamic for that
    void start_dynamic(int64_t start, int64_t end,
		       int64_t incr, int64_t chunk);

    // continues a dynamic loop
    bool next_dynamic(int64_t& span_start, int64_t& span_end,
		      int64_t& stride);

    // indicates this thread is done with the current loop - blocks
    //  if other threads haven't even entered the loop yet
    void end_loop(void);

  protected:
    int num_workers;
    // loop bounds and position are done with unsigned values to
    //  allow detection of overflow
    atomic<uint64_t> loop_pos, loop_limit;
    atomic<int64_t> loop_base, loop_incr, loop_chunk;
    atomic<int> loop_barrier;
  };

  class ThreadPool {
  public:
    ThreadPool(int _num_workers);
    ~ThreadPool(void);

    // associates the calling thread as the master of the threadpool
    void associate_as_master(void);

    // entry point for workers - does not return until thread pool is shut down
    void worker_entry(void);

    struct WorkItem {
      int prev_thread_id;
      int prev_num_threads;
      WorkItem *parent_work_item;
      atomic<int> remaining_workers;
      atomic<int> single_winner;  // worker currently assigned as the "single" one
      atomic<int> barrier_count;
      LoopSchedule schedule;
    };

    struct WorkerInfo {
      enum Status {
	WORKER_MASTER,
	WORKER_STARTING,
	WORKER_IDLE,
	WORKER_CLAIMED,
	WORKER_ACTIVE,
	WORKER_SHUTDOWN,
      };
      atomic<int> /*Status*/ status; // int allows CAS primitives
      ThreadPool *pool;
      int thread_id;  // in current team
      int num_threads; // in current team
      int app_num_threads;  // num threads requested by app
      void (*fnptr)(void *data);
      void *data;
      WorkItem *work_item;

      void push_work_item(WorkItem *new_work);
      WorkItem *pop_work_item(void);
    };
      
    // returns the WorkerInfo (if any) associated with the caller (which
    //  can be master or worker)
    static WorkerInfo *get_worker_info(void);

    // asks worker threads to shut down and waits for them to complete
    void shutdown(void);

    void claim_workers(int count, std::set<int>& worker_ids);

    void start_worker(int worker_id,
		      int thread_id, int num_threads,
		      void (*fnptr)(void *data), void *data,
		      WorkItem *work_item);

    int get_num_workers() const { return num_workers; }

  protected:
    int num_workers;
    std::vector<Thread *> worker_threads;
    std::vector<WorkerInfo> worker_infos;
  };

}; // namespace Realm

#include "realm/openmp/openmp_threadpool.inl"

#endif
