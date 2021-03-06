//
//  thread.cpp
//  cpp-pthread
//
//  Created by herbert koelman on 18/03/2016.
//  Copyright © 2016 urbix-software. All rights reserved.
//

#include "pthread/thread.hpp"
#include <unistd.h>
#include <cstdio>

namespace pthread {

  namespace this_thread {

    void sleep_for(const int millis){
      usleep(millis * 1000); //NOSONAR this wil be replaced by the much better C++11 implementation std::this_thread::sleep_for
    }

    pthread_t get_id(){
      return pthread_self();
    }
  }

  void thread::join () {

    if ( _thread != 0){

      if ( _thread == this_thread::get_id()){
        throw pthread_exception("join failed, join yourself would endup in deadlock.");
      }

      if ( _status == thread_status::not_a_thread ){
        throw pthread_exception("join failed, this is not a thread.");
      }

      int rc = pthread_join(_thread, NULL);
      if ( rc == 0 ){
        // thread was successfully joined, it's safe to assume that it's not a thread anymore.
        _status = thread_status::not_a_thread ;
        _thread = 0;
      } else {
        switch ( rc ) {
          case EDEADLK:
            throw thread_exception("EDEADLKpthread_join failed because of deadlock conditions.", rc );
          case EINVAL:
            throw thread_exception("EINVEL pthread_join failed not a joinable thread.", rc );
          case ESRCH:
            break; // thread has already ended.
          default:
            throw thread_exception("pthread_join returned an unexpected return code.", rc);
        }

      }
    }
  }

  int thread::cancel () {
    int rc = 0;

    if ( _status == thread_status::not_a_thread ){
      throw pthread_exception("cancel failed, this is not a thread.");
    }

    rc = pthread_cancel ( _thread );
    if(rc != 0){
      throw thread_exception("pthread_cancel failed.", rc );
    } else {
      _status = thread_status::not_a_thread;
    }

    return rc;
  }

  thread::thread(): _status(thread_status::not_a_thread), _thread(0){

    // intentional..
  }

  thread::thread (const runnable &work, const std::size_t stack_size ): thread(){ // ": thread()" calls the related anonymous constructor
    int rc = 0 ;

    /* Initialize and set thread detached attribute */
    rc = pthread_attr_init(&_attr);
    if ( rc != 0){
      throw thread_exception("pthread_attr_init failed.", rc );
    }

    rc = pthread_attr_setdetachstate(&_attr, PTHREAD_CREATE_JOINABLE);
    if ( rc != 0 ){
      throw thread_exception("pthread_attr_setdetachstate failed.", rc );
    }

    rc = pthread_attr_setstacksize(&_attr, stack_size);
    if ( (stack_size > 0) && (rc != 0) ){
      throw thread_exception("bad stacksize, check size passed to thread::thread; thread not started.", rc );
    }

    rc = pthread_create(&_thread, &_attr, thread_startup_runnable, (void *) &work);
    if ( rc != 0){
      throw thread_exception("pthread_create failed.", rc );
    } else {
      _status = thread_status::a_thread;
    }

  }

  /* move constructor */
  thread::thread(thread&& other){ //NOSONAR this a C++11 standard interface that we want to comply with.

    swap(other);
  }

  thread::~thread () {
      pthread_attr_destroy(&_attr);
  }

  /* move operator */
  thread& thread::operator=(thread&& other){ //NOSONAR this a C++11 standard interface that we want to comply with.

    swap(other);

    return *this;
  }

  void thread::swap(thread& other){
    std::swap(_thread, other._thread);
    std::swap(_status, other._status);
  }

  abstract_thread::abstract_thread(const std::size_t stack_size): _stack_size(stack_size), _thread(NULL){
  }

  abstract_thread::~abstract_thread(){

    if ( _thread != NULL ){
      delete _thread;
    }
  }

  void abstract_thread::start(){

    _thread = new pthread::thread(*this, _stack_size);
  }

  void abstract_thread::join() {
    _thread->join() ;
  };

  bool abstract_thread::joinable() const {
    return _thread != 0 ;
  };

#if __cplusplus < 201103L
  thread_group::thread_group(bool destructor_joins_first) throw():  _destructor_joins_first(destructor_joins_first){
#else
  thread_group::thread_group(bool destructor_joins_first) noexcept: _destructor_joins_first(destructor_joins_first){
#endif

  }

  thread_group::~thread_group(){

    while(! _threads.empty()){

#if __cplusplus < 201103L
      std::auto_ptr<pthread::abstract_thread> pat(_threads.front());
#else
      std::unique_ptr<pthread::abstract_thread> pat(_threads.front());
#endif

      _threads.pop_front();

      if ( _destructor_joins_first && pat->joinable() ){
        try {
          pat->join();
        } catch ( pthread_exception &err ){
          printf("thread_group destructor failed to join one thread. %s, (%d) %s.\n", err.what(), err.pthread_errno(), err.pthread_errmsg());
        } catch ( ... ){ //NOSONAR this was done on purpose to avoid crashes due to unhandled error conditions. This should never happen.
          printf("thread_group destructor received an unexpected exception when joining threads.");
        };
      }
    }
  }

  void thread_group::add(pthread::abstract_thread *thread){

    _threads.push_back(thread);
  }

  void thread_group::start(){
      for(auto iterator = _threads.begin(); iterator != _threads.end(); iterator++){
        (*iterator)->start();
      }
  }

  void thread_group::join(){
    for(auto iterator = _threads.begin(); iterator != _threads.end(); iterator++){
      if ( (*iterator)->joinable() ){
        (*iterator)->join();
      }
    }
  }

  unsigned long thread_group::size(){
    return _threads.size();
  }

  /**
   This function is a helper function. It has normal C linkage, and is
   the base for newly created Thread objects. It runs the
   run method on the thread object passed to it (as a void *).
   */
  void *thread_startup_runnable(void *runner) {

    try{
      static_cast<runnable *>(runner)->run();
    } catch ( ... ) { // NOSONAR threads cannot throw exceptions when ending, this prevents this from happening.
      printf("uncaugth excpetion in thread_startup_runnable(), check your runnable::run() implementation.");
    }
    return NULL ;
  }

} // namespace pthread
