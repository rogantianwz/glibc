/* futex operations for glibc-internal use.
   Copyright (C) 2014-2015 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#ifndef FUTEX_INTERNAL_H
#define FUTEX_INTERNAL_H

#include <errno.h>
#include <lowlevellock-futex.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <nptl/pthreadP.h>

/* This file defines futex operations used internally in glibc.  A futex
   consists of the so-called futex word in userspace, which is of type int
   and represents an application-specific condition, and kernel state
   associated with this particular futex word (e.g., wait queues).  The futex
   operations we provide are wrappers for the futex syscalls and add
   glibc-specific error checking of the syscall return value.  We abort on
   error codes that are caused by bugs in glibc or in the calling application,
   or when an error code is not known.  We return error codes that can arise
   in correct executions to the caller.  Each operation calls out exactly the
   return values that callers need to handle.

   The private flag must be either FUTEX_PRIVATE or FUTEX_SHARED.

   We expect callers to only use these operations if futexes are supported.

   Due to POSIX requirements on when synchronization data structures such
   as mutexes or semaphores can be destroyed and due to the futex design
   having separate fast/slow paths for wake-ups, we need to consider that
   futex_wake calls might effectively target a data structure that has been
   destroyed and reused for another object, or unmapped; thus, some
   errors or spurious wake-ups can happen in correct executions that would
   not be possible in a program using just a single futex whose lifetime
   does not end before the program terminates.  For background, see:
   https://sourceware.org/ml/libc-alpha/2014-04/msg00075.html
   https://lkml.org/lkml/2014/11/27/472

   We also expect a Linux kernel version of 2.6.22 or more recent (since this
   version, EINTR is not returned on spurious wake-ups anymore).  */

#define FUTEX_PRIVATE LLL_PRIVATE
#define FUTEX_SHARED  LLL_SHARED

/* Returns FUTEX_PRIVATE if pshared is zero and private futexes are supported;
   returns FUTEX_SHARED otherwise.  */
static __always_inline int
futex_private_if_supported (int pshared)
{
  if (pshared != 0)
    return FUTEX_SHARED;
#ifdef __ASSUME_PRIVATE_FUTEX
  return FUTEX_PRIVATE;
#else
  return THREAD_GETMEM (THREAD_SELF, header.private_futex)
      ^ FUTEX_PRIVATE_FLAG;
#endif
}


/* Atomically wrt other futex operations on the same futex, this blocks iff
   the value *FUTEX_WORD matches the expected value.  This is
   semantically equivalent to:
     l = <get lock associated with futex> (FUTEX_WORD);
     wait_flag = <get wait_flag associated with futex> (FUTEX_WORD);
     lock (l);
     val = atomic_load_relaxed (FUTEX_WORD);
     if (val != expected) { unlock (l); return EAGAIN; }
     atomic_store_relaxed (wait_flag, true);
     unlock (l);
     // Now block; can time out in futex_time_wait (see below)
     while (atomic_load_relaxed(wait_flag) && !<spurious wake-up>);

   Note that no guarantee of a happens-before relation between a woken
   futex_wait and a futex_wake is documented; however, this does not matter
   in practice because we have to consider spurious wake-ups (see below),
   and thus would not be able to reliably reason about which futex_wake woke
   us.

   Returns 0 if woken by a futex operation or spuriously.  (Note that due to
   the POSIX requirements mentioned above, we need to conservatively assume
   that unrelated futex_wake operations could wake this futex; it is easiest
   to just be prepared for spurious wake-ups.)
   Returns EAGAIN if the futex word did not match the expected value.
   Returns EINTR if waiting was interrupted by a signal.

   Note that some previous code in glibc assumed the underlying futex
   operation (e.g., syscall) to start with or include the equivalent of a
   seq_cst fence; this allows one to avoid an explicit seq_cst fence before
   a futex_wait call when synchronizing similar to Dekker synchronization.
   However, we make no such guarantee here.
   */
static __always_inline int
futex_wait (unsigned int *futex_word, unsigned int expected, int private)
{
  int err = lll_futex_timed_wait (futex_word, expected, NULL, private);
  switch (err)
    {
    case 0:
    case -EAGAIN:
    case -EINTR:
      return -err;

    case -ETIMEDOUT: /* Cannot have happened as we provided no timeout.  */
    case -EFAULT: /* Must have been caused by a glibc or application bug.  */
    case -EINVAL: /* Either due to wrong alignment or due to the timeout not
		     being normalized.  Must have been caused by a glibc or
		     application bug.  */
    case -ENOSYS: /* Must have been caused by a glibc bug.  */
    /* No other errors are documented at this time.  */
    default:
      abort ();
    }
}

/* Like futex_wait but cancelable.  */
static __always_inline int
futex_wait_cancelable (unsigned int *futex_word, unsigned int expected,
		       int private)
{
  int oldtype;
  oldtype = __pthread_enable_asynccancel ();
  int err = lll_futex_timed_wait (futex_word, expected, NULL, private);
  __pthread_disable_asynccancel (oldtype);
  switch (err)
    {
    case 0:
    case -EAGAIN:
    case -EINTR:
      return -err;

    case -ETIMEDOUT: /* Cannot have happened as we provided no timeout.  */
    case -EFAULT: /* Must have been caused by a glibc or application bug.  */
    case -EINVAL: /* Either due to wrong alignment or due to the timeout not
		     being normalized.  Must have been caused by a glibc or
		     application bug.  */
    case -ENOSYS: /* Must have been caused by a glibc bug.  */
    /* No other errors are documented at this time.  */
    default:
      abort ();
    }
}

/* Like futex_wait, but will eventually time out (i.e., stop being
   blocked) after the duration of time provided (i.e., RELTIME) has
   passed.  The caller must provide a normalized RELTIME.  RELTIME can also
   equal NULL, in which case this function behaves equivalent to futex_wait.

   Returns 0 if woken by a futex operation or spuriously.  (Note that due to
   the POSIX requirements mentioned above, we need to conservatively assume
   that unrelated futex_wake operations could wake this futex; it is easiest
   to just be prepared for spurious wake-ups.)
   Returns EAGAIN if the futex word did not match the expected value.
   Returns EINTR if waiting was interrupted by a signal.
   Returns ETIMEDOUT if the timeout expired.
   */
static __always_inline int
futex_reltimed_wait (unsigned int* futex_word, unsigned int expected,
		     const struct timespec* reltime, int private)
{
  int err = lll_futex_timed_wait (futex_word, expected, reltime, private);
  switch (err)
    {
    case 0:
    case -EAGAIN:
    case -EINTR:
    case -ETIMEDOUT:
      return -err;

    case -EFAULT: /* Must have been caused by a glibc or application bug.  */
    case -EINVAL: /* Either due to wrong alignment or due to the timeout not
		     being normalized.  Must have been caused by a glibc or
		     application bug.  */
    case -ENOSYS: /* Must have been caused by a glibc bug.  */
    /* No other errors are documented at this time.  */
    default:
      abort ();
    }
}

/* Like futex_reltimed_wait, but cancelable.  */
static __always_inline int
futex_reltimed_wait_cancelable (unsigned int* futex_word,
				unsigned int expected,
			        const struct timespec* reltime, int private)
{
  int oldtype;
  oldtype = __pthread_enable_asynccancel ();
  int err = lll_futex_timed_wait (futex_word, expected, reltime, private);
  __pthread_disable_asynccancel (oldtype);
  switch (err)
    {
    case 0:
    case -EAGAIN:
    case -EINTR:
    case -ETIMEDOUT:
      return -err;

    case -EFAULT: /* Must have been caused by a glibc or application bug.  */
    case -EINVAL: /* Either due to wrong alignment or due to the timeout not
		     being normalized.  Must have been caused by a glibc or
		     application bug.  */
    case -ENOSYS: /* Must have been caused by a glibc bug.  */
    /* No other errors are documented at this time.  */
    default:
      abort ();
    }
}

/* Like futex_reltimed_wait, but the provided timeout (ABSTIME) is an
   absolute point in time; a call will time out after this point in time.  */
static __always_inline int
futex_abstimed_wait (unsigned int* futex_word, unsigned int expected,
		     const struct timespec* abstime, int private)
{
  int err = lll_futex_abstimed_wait (futex_word, expected, abstime, private);
  switch (err)
    {
    case 0:
    case -EAGAIN:
    case -EINTR:
    case -ETIMEDOUT:
      return -err;

    case -EFAULT: /* Must have been caused by a glibc or application bug.  */
    case -EINVAL: /* Either due to wrong alignment or due to the timeout not
		     being normalized.  Must have been caused by a glibc or
		     application bug.  */
    case -ENOSYS: /* Must have been caused by a glibc bug.  */
    /* No other errors are documented at this time.  */
    default:
      abort ();
    }
}

/* Like futex_reltimed_wait, but cancelable.  */
static __always_inline int
futex_abstimed_wait_cancelable (unsigned int* futex_word,
				unsigned int expected,
			        const struct timespec* abstime, int private)
{
  int oldtype;
  oldtype = __pthread_enable_asynccancel ();
  int err = lll_futex_abstimed_wait (futex_word, expected, abstime, private);
  __pthread_disable_asynccancel (oldtype);
  switch (err)
    {
    case 0:
    case -EAGAIN:
    case -EINTR:
    case -ETIMEDOUT:
      return -err;

    case -EFAULT: /* Must have been caused by a glibc or application bug.  */
    case -EINVAL: /* Either due to wrong alignment or due to the timeout not
		     being normalized.  Must have been caused by a glibc or
		     application bug.  */
    case -ENOSYS: /* Must have been caused by a glibc bug.  */
    /* No other errors are documented at this time.  */
    default:
      abort ();
    }
}

/* Atomically wrt other futex operations on the same futex, this unblocks the
   specified number of processes, or all processes blocked on this futex if
   there are fewer than the specified number.  Semantically, this is
   equivalent to:
     l = <get lock associated with futex> (FUTEX_WORD);
     lock (l);
     for (res = 0; PROCESSES_TO_WAKE > 0; PROCESSES_TO_WAKE--, res++) {
       if (<no process blocked on futex>) break;
       wf = <get wait_flag of a process blocked on futex> (FUTEX_WORD);
       // No happens-before guarantee with woken futex_wait (see above)
       atomic_store_relaxed (wf, 0);
     }
     return res;

   Note that we need to support futex_wake calls to past futexes whose memory
   has potentially been reused due to POSIX' requirements on synchronization
   object destruction (see above); therefore, we must not report or abort
   on most errors.  */
static __always_inline void
futex_wake (unsigned int* futex_word, int processes_to_wake, int private)
{
  int res = lll_futex_wake (futex_word, processes_to_wake, private);
  /* No error.  Ignore the number of woken processes.  */
  if (res >= 0)
    return;
  switch (res)
    {
    case -EFAULT: /* Could have happened due to memory reuse.  */
    case -EINVAL: /* Could be either due to incorrect alignment (a bug in
		     glibc or in the application) or due to memory being
		     reused for a PI futex.  We cannot distinguish between the
		     two causes, and one of them is correct use, so we do not
		     act in this case.  */
      return;
    case -ENOSYS: /* Must have been caused by a glibc bug.  */
    /* No other errors are documented at this time.  */
    default:
      abort ();
    }
}

#endif  /* futex-internal.h */
