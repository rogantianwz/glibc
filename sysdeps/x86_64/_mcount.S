/* Machine-specific calling sequence for `mcount' profiling function.  x86-64 version.
   Copyright (C) 2002-2015 Free Software Foundation, Inc.
   Contributed by Andreas Jaeger <aj@suse.de>.
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

/* Assembly stub to invoke _mcount().  Compiler generated code calls
   this stub after executing a function's prologue and without saving any
   registers.  It is therefore necessary to preserve %rcx, %rdx, %rsi, %rdi,
   %r8, %r9 as they may contain function arguments.  */

#include <sysdep.h>

	.globl C_SYMBOL_NAME(_mcount)
	.type C_SYMBOL_NAME(_mcount), @function
	.align ALIGNARG(4)
C_LABEL(_mcount)
	/* Allocate space for 7 registers.  */
	subq	$56,%rsp
	movq	%rax,(%rsp)
	movq	%rcx,8(%rsp)
	movq	%rdx,16(%rsp)
	movq	%rsi,24(%rsp)
	movq	%rdi,32(%rsp)
	movq	%r8,40(%rsp)
	movq	%r9,48(%rsp)

	/* Setup parameter for __mcount_internal.  */
	/* selfpc is the return address on the stack.  */
	movq	56(%rsp),%rsi
	/* Get frompc via the frame pointer.  */
	movq	8(%rbp),%rdi
#ifdef PIC
	call C_SYMBOL_NAME(__mcount_internal)@PLT
#else
	call C_SYMBOL_NAME(__mcount_internal)
#endif
	/* Pop the saved registers.  Please note that `mcount' has no
	   return value.  */
	movq	48(%rsp),%r9
	movq	40(%rsp),%r8
	movq	32(%rsp),%rdi
	movq	24(%rsp),%rsi
	movq	16(%rsp),%rdx
	movq	8(%rsp),%rcx
	movq	(%rsp),%rax
	addq	$56,%rsp
	ret

	ASM_SIZE_DIRECTIVE(C_SYMBOL_NAME(_mcount))

#undef mcount
weak_alias (_mcount, mcount)

	.globl C_SYMBOL_NAME(__fentry__)
	.type C_SYMBOL_NAME(__fentry__), @function
	.align ALIGNARG(4)
C_LABEL(__fentry__)
	/* Allocate space for 7 registers.  */
	subq	$64,%rsp
	movq	%rax,(%rsp)
	movq	%rcx,8(%rsp)
	movq	%rdx,16(%rsp)
	movq	%rsi,24(%rsp)
	movq	%rdi,32(%rsp)
	movq	%r8,40(%rsp)
	movq	%r9,48(%rsp)

	/* Setup parameter for __mcount_internal.  */
	/* selfpc is the return address on the stack.  */
	movq	64(%rsp),%rsi
	/* caller is the return address above it */
	movq	72(%rsp),%rdi
#ifdef PIC
	call C_SYMBOL_NAME(__mcount_internal)@PLT
#else
	call C_SYMBOL_NAME(__mcount_internal)
#endif
	/* Pop the saved registers.  Please note that `__fentry__' has no
	   return value.  */
	movq	48(%rsp),%r9
	movq	40(%rsp),%r8
	movq	32(%rsp),%rdi
	movq	24(%rsp),%rsi
	movq	16(%rsp),%rdx
	movq	8(%rsp),%rcx
	movq	(%rsp),%rax
	addq	$64,%rsp
	ret

	ASM_SIZE_DIRECTIVE(C_SYMBOL_NAME(__fentry__))
