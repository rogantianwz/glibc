/* Copyright (C) 2003-2015 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Martin Schwidefsky <schwidefsky@de.ibm.com>, 2003.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <sysdep.h>
#include <lowlevellock.h>
#include <pthread.h>
#include <pthreadP.h>
#include <sys/time.h>
#include <kernel-features.h>
#include <stdbool.h>


/* Try to acquire write lock for RWLOCK or return after specfied time.	*/
int
pthread_rwlock_timedwrlock (rwlock, abstime)
     pthread_rwlock_t *rwlock;
     const struct timespec *abstime;
{
  int result = 0;
  bool wake_readers = false;

  /* Make sure we are alone.  */
  lll_lock (rwlock->__data.__lock, rwlock->__data.__shared);

  while (1)
    {
      int err;

      /* Get the rwlock if there is no writer and no reader.  */
      if (rwlock->__data.__writer == 0 && rwlock->__data.__nr_readers == 0)
	{
	  /* Mark self as writer.  */
	  rwlock->__data.__writer = THREAD_GETMEM (THREAD_SELF, tid);
	  break;
	}

      /* Make sure we are not holding the rwlock as a writer.  This is
	 a deadlock situation we recognize and report.  */
      if (__builtin_expect (rwlock->__data.__writer
			    == THREAD_GETMEM (THREAD_SELF, tid), 0))
	{
	  result = EDEADLK;
	  break;
	}

      /* Make sure the passed in timeout value is valid.  Ideally this
	 test would be executed once.  But since it must not be
	 performed if we would not block at all simply moving the test
	 to the front is no option.  Replicating all the code is
	 costly while this test is not.  */
      if (__builtin_expect (abstime->tv_nsec >= 1000000000
                            || abstime->tv_nsec < 0, 0))
	{
	  result = EINVAL;
	  break;
	}

      /* Work around the fact that the kernel rejects negative timeout values
	 despite them being valid.  */
      if (__glibc_unlikely (abstime->tv_sec < 0))
	{
	  result = ETIMEDOUT;
	  break;
	}

#if (!defined __ASSUME_FUTEX_CLOCK_REALTIME \
     || !defined lll_futex_timed_wait_bitset)
      /* Get the current time.  So far we support only one clock.  */
      struct timeval tv;
      (void) __gettimeofday (&tv, NULL);

      /* Convert the absolute timeout value to a relative timeout.  */
      struct timespec rt;
      rt.tv_sec = abstime->tv_sec - tv.tv_sec;
      rt.tv_nsec = abstime->tv_nsec - tv.tv_usec * 1000;
      if (rt.tv_nsec < 0)
	{
	  rt.tv_nsec += 1000000000;
	  --rt.tv_sec;
	}
      /* Did we already time out?  */
      if (rt.tv_sec < 0)
	{
	  result = ETIMEDOUT;
	  break;
	}
#endif

      /* Remember that we are a writer.  */
      if (++rwlock->__data.__nr_writers_queued == 0)
	{
	  /* Overflow on number of queued writers.  */
	  --rwlock->__data.__nr_writers_queued;
	  result = EAGAIN;
	  break;
	}

      int waitval = rwlock->__data.__writer_wakeup;

      /* Free the lock.  */
      lll_unlock (rwlock->__data.__lock, rwlock->__data.__shared);

      /* Wait for the writer or reader(s) to finish.  */
#if (!defined __ASSUME_FUTEX_CLOCK_REALTIME \
     || !defined lll_futex_timed_wait_bitset)
      err = lll_futex_timed_wait (&rwlock->__data.__writer_wakeup,
				  waitval, &rt, rwlock->__data.__shared);
#else
      err = lll_futex_timed_wait_bitset (&rwlock->__data.__writer_wakeup,
					 waitval, abstime,
					 FUTEX_CLOCK_REALTIME,
					 rwlock->__data.__shared);
#endif

      /* Get the lock.  */
      lll_lock (rwlock->__data.__lock, rwlock->__data.__shared);

      /* To start over again, remove the thread from the writer list.  */
      --rwlock->__data.__nr_writers_queued;

      /* Did the futex call time out?  */
      if (err == -ETIMEDOUT)
	{
	  result = ETIMEDOUT;
	  /* If we prefer writers, it can have happened that readers blocked
	     for us to acquire the lock first.  If we have timed out, we need
	     to wake such readers if there are any, and if there is no writer
	     currently (otherwise, the writer will take care of wake-up).
	     Likewise, even if we prefer readers, we can be responsible for
	     wake-up (see pthread_rwlock_unlock) if no reader or writer has
	     acquired the lock.  We have timed out and thus not consumed a
	     futex wake-up; therefore, if there is no other blocked writer
	     that would consume the wake-up and thus take over responsibility,
	     we need to wake blocked readers.  */
	  if ((!PTHREAD_RWLOCK_PREFER_READER_P (rwlock)
	       || ((rwlock->__data.__nr_readers == 0)
		   && (rwlock->__data.__nr_writers_queued == 0)))
	      && (rwlock->__data.__nr_readers_queued > 0)
	      && (rwlock->__data.__writer == 0))
	    {
	      ++rwlock->__data.__readers_wakeup;
	      wake_readers = true;
	    }
	  break;
	}
    }

  /* We are done, free the lock.  */
  lll_unlock (rwlock->__data.__lock, rwlock->__data.__shared);

  /* Might be required after timeouts.  */
  if (wake_readers)
    lll_futex_wake (&rwlock->__data.__readers_wakeup, INT_MAX,
	rwlock->__data.__shared);

  return result;
}
