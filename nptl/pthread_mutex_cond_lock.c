#include <pthreadP.h>

#define LLL_MUTEX_LOCK(mutex) \
  lll_cond_lock ((mutex)->__data.__lock, PTHREAD_MUTEX_PSHARED (mutex))

/* Not actually elided so far. Needed? */
#define LLL_MUTEX_LOCK_ELISION(mutex)  \
  ({ lll_cond_lock ((mutex)->__data.__lock, PTHREAD_MUTEX_PSHARED (mutex)); 0; })

#define LLL_MUTEX_TRYLOCK(mutex) \
  lll_cond_trylock ((mutex)->__data.__lock)
#define LLL_MUTEX_TRYLOCK_ELISION(mutex) LLL_MUTEX_TRYLOCK(mutex)

#define LLL_ROBUST_MUTEX_LOCK(mutex, id) \
  lll_robust_cond_lock ((mutex)->__data.__lock, id, \
			PTHREAD_ROBUST_MUTEX_PSHARED (mutex))
#define __pthread_mutex_lock __pthread_mutex_cond_lock
#define __pthread_mutex_lock_full __pthread_mutex_cond_lock_full
#define NO_INCR

#include <nptl/pthread_mutex_lock.c>
