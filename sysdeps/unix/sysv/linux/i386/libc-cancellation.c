/* i386 cancellation definitions.
   Copyright (C) 2015 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <sysdep.h>

#define INTERNAL_SYSCALL_MAIN_6_NCS(name, err, arg1, arg2, arg3,	\
				    arg4, arg5, arg6)			\
  struct libc_do_syscall_args _xv =					\
    {									\
      (int) (arg1),							\
      (int) (arg5),							\
      (int) (arg6)							\
    };									\
    asm volatile (							\
    "movl %1, %%eax\n\t"						\
    "call __libc_do_syscall"						\
    : "=a" (resultvar)							\
    : "0" (name), "c" (arg2), "d" (arg3), "S" (arg4), "D" (&_xv)	\
    : "memory", "cc")

#undef INTERNAL_SYSCALL_NCS
#define INTERNAL_SYSCALL_NCS(name, err, nr, arg1, arg2, arg3, arg4,	\
			     arg5, arg6)				\
  ({									\
    unsigned int resultvar;						\
    INTERNAL_SYSCALL_MAIN_6_NCS (name, err, arg1, arg2, arg3, arg4,	\
				 arg5, arg6);				\
    (int) resultvar;							\
  })

#include <nptl/libc-cancellation.c>
