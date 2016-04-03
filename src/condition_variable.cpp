  /*
 $Id: condition_variable.C 28 2007-08-06 22:29:28Z hkoelman $
 */

#include "pthread/condition_variable.hpp"

namespace pthread {
  
  void condition_variable::wait(mutex &mtx) {
    pthread_cond_wait ( &_condition, &mtx._mutex);
  }
  
  void condition_variable::wait(lock_guard<pthread::mutex> lck){
    wait(*(lck.mutex()));
  }
  
  cv_status condition_variable::wait_for (lock_guard<pthread::mutex> &lck, int millis ){
    return wait_for(*(lck.mutex()), millis);
  }
  
  /* Default millis is 0 */
  cv_status condition_variable::wait_for ( mutex &mtx, int millis ) {
    int rc = 0;
    cv_status status = no_timeout;
    
    if ( millis >= 0) milliseconds(millis);

    rc  = pthread_cond_timedwait ( &_condition, &mtx._mutex, &timeout );
    
    switch (rc){
        
      case ETIMEDOUT:
        status = timedout;
        break;
        
      case EINVAL:
        throw condition_variable_exception("The value specified by abstime is invalid.", rc);
        break;
        
      case EPERM:
        throw condition_variable_exception("The mutex was not owned by the current thread at the time of the call.", rc);
        break;
      default:
        status = no_timeout ;
        break;
    }
    
    return status;
  }
  
  void condition_variable::notify_one() __NOEXCEPT__ {
    pthread_cond_signal ( &_condition );
  }
  
  void condition_variable::notify_all () __NOEXCEPT__{
    pthread_cond_broadcast ( &_condition );
    
  }
  
  void condition_variable::milliseconds(int millis){
    timeval  now;
    
    if ( gettimeofday ( &now, NULL ) == 0){
      timeout.tv_sec = now.tv_sec;
      timeout.tv_nsec= now.tv_usec * 1000 ;
      
      auto seconds = millis / 1000;
      auto nanos   = (now.tv_usec * 1000) + ((millis % 1000) * 1000000) ;
      seconds     += nanos / 1000000000 ; // check if now + millis id not overflowing.
      nanos        = nanos % 1000000000 ;
      
      timeout.tv_sec  += seconds ;
      timeout.tv_nsec  = nanos;
    } else {
      throw condition_variable_exception("failed to get current time.");
    }
  }
  
  // constuctors & destructors --------------
  
  condition_variable::condition_variable () {
    int rc = pthread_cond_init ( &_condition, NULL );
    if ( rc != 0 ){
      throw condition_variable_exception("pthread_cond_init failed.", rc);
    }
  }
  
  condition_variable::~condition_variable () {
    if (pthread_cond_destroy(&_condition) != 0){
      throw pthread_exception("pthread condition variable destroy failed.", 0);
    }
  }
  
  // excpetions -------
  
#ifdef __IBMCPP__
  condition_variable_exception::condition_variable_exception( const string message, const int pthread_error): pthread_exception(message, pthread_error){
#else
  condition_variable_exception::condition_variable_exception( const string message, const int pthread_error): pthread_exception{message, pthread_error}{
#endif
  };
  
  
} // namespace pthread
