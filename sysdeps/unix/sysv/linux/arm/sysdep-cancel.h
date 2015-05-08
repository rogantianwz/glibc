/* Copyright (C) 2003-2015 Free Software Foundation, Inc.
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
   License along with the GNU C Library.  If not, see
   <http://www.gnu.org/licenses/>.  */

#include <sysdep.h>
#include <tls.h>
#ifndef __ASSEMBLER__
# include <nptl/pthreadP.h>
# include <sys/ucontext.h>
#endif

#if IS_IN (libc) || IS_IN (libpthread) || IS_IN (librt)

# if IS_IN (libc)
#  define JMP_SYSCALL_CANCEL  HIDDEN_JUMPTARGET(__syscall_cancel)
# else
#  define JMP_SYSCALL_CANCEL  __syscall_cancel(PLT)
# endif

/* NOTE: We do mark syscalls with unwind annotations, for the benefit of
   cancellation; but they're really only accurate at the point of the
   syscall.  The ARM unwind directives are not rich enough without adding
   a custom personality function.  */

# undef PSEUDO
# define PSEUDO(name, syscall_name, args)				\
	.text;								\
  ENTRY (name);								\
	SINGLE_THREAD_P;						\
	bne .Lpseudo_cancel;						\
	DOARGS_##args;							\
	cfi_remember_state;						\
	ldr	r7, =SYS_ify (syscall_name);				\
	swi	0x0;							\
	UNDOARGS_##args;						\
	cmn	r0, $4096;						\
	PSEUDO_RET;							\
	cfi_restore_state;						\
  .Lpseudo_cancel:							\
	.fnstart;	/* matched by the .fnend in UNDOARGS below.  */	\
	push	{r4, r5, lr};						\
	.save   {r4, r5, lr};						\
	PSEUDO_CANCEL_BEFORE;						\
	movw	r0, SYS_ify (syscall_name);				\
	PSEUDO_CANCEL_AFTER;						\
	pop	{r4, r5, pc};						\
	.fnend;								\
	cmn	r0, $4096

# define PSEUDO_CANCEL_BEFORE						\
	.pad	#20;							\
	sub	sp, sp, #20;						\
	ldr	r5, [sp, #32];						\
	ldr	r4, [sp, #36];						\
	str	r3, [sp];						\
	mov	r3, r2;							\
	str	r5, [sp, #4];						\
	mov	r2, r1;							\
	str	r4, [sp, #8];						\
	mov	r1, r0

# define PSEUDO_CANCEL_AFTER						\
	bl	JMP_SYSCALL_CANCEL;					\
	add	sp, sp, #20

/* DOARGS pushes eight bytes on the stack for five arguments, twelve bytes for
   six arguments, and four bytes for fewer.  In order to preserve doubleword
   alignment, sometimes we must save an extra register.  */

# define RESTART_UNWIND				\
	.fnend;					\
	.fnstart;				\
	.save	{r7};				\
	.save	{lr}

# define DOCARGS_0				\
	.save {r7};				\
	push	{lr};				\
	cfi_adjust_cfa_offset (4);		\
	cfi_rel_offset (lr, 0);			\
	.save	{lr}
# define UNDOCARGS_0
# define RESTORE_LR_0				\
	pop	{lr};				\
	cfi_adjust_cfa_offset (-4);		\
	cfi_restore (lr)

# define DOCARGS_1				\
	.save	{r7};				\
	push	{r0, r1, lr};			\
	cfi_adjust_cfa_offset (12);		\
	cfi_rel_offset (lr, 8);			\
	.save	{lr};				\
	.pad	#8
# define UNDOCARGS_1				\
	ldr r0, [sp], #8;			\
	cfi_adjust_cfa_offset (-8);		\
	RESTART_UNWIND
# define RESTORE_LR_1				\
	RESTORE_LR_0

# define DOCARGS_2				\
	.save	{r7};				\
	push	{r0, r1, lr};			\
	cfi_adjust_cfa_offset (12);		\
	cfi_rel_offset (lr, 8);			\
	.save	{lr};				\
	.pad	#8
# define UNDOCARGS_2				\
	pop	{r0, r1};			\
	cfi_adjust_cfa_offset (-8);		\
	RESTART_UNWIND
# define RESTORE_LR_2				\
	RESTORE_LR_0

# define DOCARGS_3				\
	.save	{r7};				\
	push	{r0, r1, r2, r3, lr};		\
	cfi_adjust_cfa_offset (20);		\
	cfi_rel_offset (lr, 16);		\
	.save	{lr};				\
	.pad	#16
# define UNDOCARGS_3				\
	pop	{r0, r1, r2, r3};		\
	cfi_adjust_cfa_offset (-16);		\
	RESTART_UNWIND
# define RESTORE_LR_3				\
	RESTORE_LR_0

# define DOCARGS_4				\
	.save	{r7};				\
	push	{r0, r1, r2, r3, lr};		\
	cfi_adjust_cfa_offset (20);		\
	cfi_rel_offset (lr, 16);		\
	.save	{lr};				\
	.pad	#16
# define UNDOCARGS_4				\
	pop	{r0, r1, r2, r3};		\
	cfi_adjust_cfa_offset (-16);		\
	RESTART_UNWIND
# define RESTORE_LR_4				\
	RESTORE_LR_0

/* r4 is only stmfd'ed for correct stack alignment.  */
# define DOCARGS_5				\
	.save	{r4, r7};			\
	push	{r0, r1, r2, r3, r4, lr};	\
	cfi_adjust_cfa_offset (24);		\
	cfi_rel_offset (lr, 20);		\
	.save	{lr};				\
	.pad	#20
# define UNDOCARGS_5				\
	pop	{r0, r1, r2, r3};		\
	cfi_adjust_cfa_offset (-16);		\
	.fnend;					\
	.fnstart;				\
	.save	{r4, r7};			\
	.save	{lr};				\
	.pad	#4
# define RESTORE_LR_5				\
	pop	{r4, lr};			\
	cfi_adjust_cfa_offset (-8);		\
	/* r4 will be marked as restored later.  */ \
	cfi_restore (lr)

# define DOCARGS_6				\
	.save	{r4, r5, r7};			\
	push	{r0, r1, r2, r3, lr};		\
	cfi_adjust_cfa_offset (20);		\
	cfi_rel_offset (lr, 16);		\
	.save	{lr};				\
	.pad	#16
# define UNDOCARGS_6				\
	pop	{r0, r1, r2, r3};		\
	cfi_adjust_cfa_offset (-16);		\
	.fnend;					\
	.fnstart;				\
	.save	{r4, r5, r7};			\
	.save	{lr};
# define RESTORE_LR_6				\
	RESTORE_LR_0

# if IS_IN (libpthread)
#  define __local_multiple_threads __pthread_multiple_threads
# elif IS_IN (libc)
#  define __local_multiple_threads __libc_multiple_threads
# endif

# if IS_IN (libpthread) || IS_IN (libc)
#  ifndef __ASSEMBLER__
extern int __local_multiple_threads attribute_hidden;
#   define SINGLE_THREAD_P __builtin_expect (__local_multiple_threads == 0, 1)
#  else
#   define SINGLE_THREAD_P						\
	LDST_PCREL(ldr, ip, ip, __local_multiple_threads);		\
	teq ip, #0
#  endif
# else
/*  There is no __local_multiple_threads for librt, so use the TCB.  */
#  ifndef __ASSEMBLER__
#   define SINGLE_THREAD_P						\
  __builtin_expect (THREAD_GETMEM (THREAD_SELF,				\
				   header.multiple_threads) == 0, 1)
#  else
#   define SINGLE_THREAD_P						\
	push	{r0, lr};						\
	cfi_adjust_cfa_offset (8);					\
	cfi_rel_offset (lr, 4);						\
	GET_TLS (lr);							\
	NEGOFF_ADJ_BASE (r0, MULTIPLE_THREADS_OFFSET);			\
	ldr	ip, NEGOFF_OFF1 (r0, MULTIPLE_THREADS_OFFSET);		\
	pop	{r0, lr};						\
	cfi_adjust_cfa_offset (-8);					\
	cfi_restore (lr);						\
	teq	ip, #0
#  endif
# endif

#elif !defined __ASSEMBLER__

/* For rtld, et cetera.  */
# define SINGLE_THREAD_P 1
# define NO_CANCELLATION 1

#endif

#ifndef __ASSEMBLER__
# define RTLD_SINGLE_THREAD_P \
  __builtin_expect (THREAD_GETMEM (THREAD_SELF, \
				   header.multiple_threads) == 0, 1)

static inline
long int __pthread_get_ip (const struct ucontext *uc)
{
  return uc->uc_mcontext.arm_pc;
}
#endif
