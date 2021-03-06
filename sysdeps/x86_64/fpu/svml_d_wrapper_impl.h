/* Wrapper implementations of vector math functions.
   Copyright (C) 2014-2015 Free Software Foundation, Inc.
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

/* SSE2 ISA version as wrapper to scalar.  */
.macro WRAPPER_IMPL_SSE2 callee
        subq      $40, %rsp
        cfi_adjust_cfa_offset(40)
        movaps    %xmm0, (%rsp)
        call      \callee@PLT
        movsd     %xmm0, 16(%rsp)
        movsd     8(%rsp), %xmm0
        call      \callee@PLT
        movsd     16(%rsp), %xmm1
        movsd     %xmm0, 24(%rsp)
        unpcklpd  %xmm0, %xmm1
        movaps    %xmm1, %xmm0
        addq      $40, %rsp
        cfi_adjust_cfa_offset(-40)
        ret
.endm

/* 2 argument SSE2 ISA version as wrapper to scalar.  */
.macro WRAPPER_IMPL_SSE2_ff callee
        subq      $56, %rsp
        cfi_adjust_cfa_offset(56)
        movaps    %xmm0, (%rsp)
        movaps    %xmm1, 16(%rsp)
        call      \callee@PLT
        movsd     %xmm0, 32(%rsp)
        movsd     8(%rsp), %xmm0
        movsd     24(%rsp), %xmm1
        call      \callee@PLT
        movsd     32(%rsp), %xmm1
        movsd     %xmm0, 40(%rsp)
        unpcklpd  %xmm0, %xmm1
        movaps    %xmm1, %xmm0
        addq      $56, %rsp
        cfi_adjust_cfa_offset(-56)
        ret
.endm

/* 3 argument SSE2 ISA version as wrapper to scalar.  */
.macro WRAPPER_IMPL_SSE2_fFF callee
        pushq   %rbp
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%rbp, 0)
        pushq   %rbx
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%rbx, 0)
        movq    %rdi, %rbp
        movq    %rsi, %rbx
        subq    $40, %rsp
        cfi_adjust_cfa_offset(40)
        leaq    16(%rsp), %rsi
        leaq    24(%rsp), %rdi
        movaps  %xmm0, (%rsp)
        call    \callee@PLT
        leaq    16(%rsp), %rsi
        leaq    24(%rsp), %rdi
        movsd   24(%rsp), %xmm0
        movapd  (%rsp), %xmm1
        movsd   %xmm0, 0(%rbp)
        unpckhpd        %xmm1, %xmm1
        movsd   16(%rsp), %xmm0
        movsd   %xmm0, (%rbx)
        movapd  %xmm1, %xmm0
        call    \callee@PLT
        movsd   24(%rsp), %xmm0
        movsd   %xmm0, 8(%rbp)
        movsd   16(%rsp), %xmm0
        movsd   %xmm0, 8(%rbx)
        addq    $40, %rsp
        cfi_adjust_cfa_offset(-40)
        popq    %rbx
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%rbx)
        popq    %rbp
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%rbp)
        ret
.endm

/* AVX/AVX2 ISA version as wrapper to SSE ISA version.  */
.macro WRAPPER_IMPL_AVX callee
        pushq		%rbp
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%rbp, 0)
        movq		%rsp, %rbp
        cfi_def_cfa_register (%rbp)
        andq		$-32, %rsp
        subq		$32, %rsp
        vextractf128	$1, %ymm0, (%rsp)
        vzeroupper
        call		HIDDEN_JUMPTARGET(\callee)
        vmovapd		%xmm0, 16(%rsp)
        vmovaps		(%rsp), %xmm0
        call		HIDDEN_JUMPTARGET(\callee)
        vmovapd		%xmm0, %xmm1
        vmovapd		16(%rsp), %xmm0
        vinsertf128	$1, %xmm1, %ymm0, %ymm0
        movq		%rbp, %rsp
        cfi_def_cfa_register (%rsp)
        popq		%rbp
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%rbp)
        ret
.endm

/* 2 argument AVX/AVX2 ISA version as wrapper to SSE ISA version.  */
.macro WRAPPER_IMPL_AVX_ff callee
        pushq     %rbp
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%rbp, 0)
        movq      %rsp, %rbp
        cfi_def_cfa_register (%rbp)
        andq      $-32, %rsp
        subq      $64, %rsp
        vextractf128 $1, %ymm0, 16(%rsp)
        vextractf128 $1, %ymm1, (%rsp)
        vzeroupper
        call      HIDDEN_JUMPTARGET(\callee)
        vmovaps   %xmm0, 32(%rsp)
        vmovaps   16(%rsp), %xmm0
        vmovaps   (%rsp), %xmm1
        call      HIDDEN_JUMPTARGET(\callee)
        vmovaps   %xmm0, %xmm1
        vmovaps   32(%rsp), %xmm0
        vinsertf128 $1, %xmm1, %ymm0, %ymm0
        movq      %rbp, %rsp
        cfi_def_cfa_register (%rsp)
        popq      %rbp
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%rbp)
        ret
.endm

/* 3 argument AVX/AVX2 ISA version as wrapper to SSE ISA version.  */
.macro WRAPPER_IMPL_AVX_fFF callee
        pushq     %rbp
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%rbp, 0)
        movq      %rsp, %rbp
        cfi_def_cfa_register (%rbp)
        andq      $-32, %rsp
        pushq     %r13
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%r13, 0)
        pushq     %r14
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%r14, 0)
        subq      $48, %rsp
        movq      %rsi, %r14
        movq      %rdi, %r13
        vextractf128 $1, %ymm0, 32(%rsp)
        vzeroupper
        call      HIDDEN_JUMPTARGET(\callee)
        vmovaps   32(%rsp), %xmm0
        lea       (%rsp), %rdi
        lea       16(%rsp), %rsi
        call      HIDDEN_JUMPTARGET(\callee)
        vmovapd   (%rsp), %xmm0
        vmovapd   16(%rsp), %xmm1
        vmovapd   %xmm0, 16(%r13)
        vmovapd   %xmm1, 16(%r14)
        addq      $48, %rsp
        popq      %r14
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%r14)
        popq      %r13
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%r13)
        movq      %rbp, %rsp
        cfi_def_cfa_register (%rsp)
        popq      %rbp
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%rbp)
        ret
.endm

/* AVX512 ISA version as wrapper to AVX2 ISA version.  */
.macro WRAPPER_IMPL_AVX512 callee
        pushq	%rbp
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%rbp, 0)
        movq	%rsp, %rbp
        cfi_def_cfa_register (%rbp)
        andq	$-64, %rsp
        subq	$64, %rsp
/* Below is encoding for vmovaps %zmm0, (%rsp).  */
        .byte	0x62
        .byte	0xf1
        .byte	0x7c
        .byte	0x48
        .byte	0x29
        .byte	0x04
        .byte	0x24
/* Below is encoding for vmovapd (%rsp), %ymm0.  */
        .byte	0xc5
        .byte	0xfd
        .byte	0x28
        .byte	0x04
        .byte	0x24
        call	HIDDEN_JUMPTARGET(\callee)
/* Below is encoding for vmovapd 32(%rsp), %ymm0.  */
        .byte	0xc5
        .byte	0xfd
        .byte	0x28
        .byte	0x44
        .byte	0x24
        .byte	0x20
        call	HIDDEN_JUMPTARGET(\callee)
        movq	%rbp, %rsp
        cfi_def_cfa_register (%rsp)
        popq	%rbp
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%rbp)
        ret
.endm

/* 2 argument AVX512 ISA version as wrapper to AVX2 ISA version.  */
.macro WRAPPER_IMPL_AVX512_ff callee
        pushq	%rbp
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%rbp, 0)
        movq	%rsp, %rbp
        cfi_def_cfa_register (%rbp)
        andq	$-64, %rsp
        subq	$128, %rsp
/* Below is encoding for vmovaps %zmm0, (%rsp).  */
        .byte	0x62
        .byte	0xf1
        .byte	0x7c
        .byte	0x48
        .byte	0x29
        .byte	0x04
        .byte	0x24
/* Below is encoding for vmovaps %zmm1, 64(%rsp).  */
        .byte	0x62
        .byte	0xf1
        .byte	0x7c
        .byte	0x48
        .byte	0x29
        .byte	0x4c
        .byte	0x24
/* Below is encoding for vmovapd (%rsp), %ymm0.  */
        .byte	0xc5
        .byte	0xfd
        .byte	0x28
        .byte	0x04
        .byte	0x24
/* Below is encoding for vmovapd 64(%rsp), %ymm1.  */
        .byte	0xc5
        .byte	0xfd
        .byte	0x28
        .byte	0x4c
        .byte	0x24
        .byte	0x40
        call	HIDDEN_JUMPTARGET(\callee)
/* Below is encoding for vmovapd 32(%rsp), %ymm0.  */
        .byte	0xc5
        .byte	0xfd
        .byte	0x28
        .byte	0x44
        .byte	0x24
        .byte	0x20
/* Below is encoding for vmovapd 96(%rsp), %ymm1.  */
        .byte	0xc5
        .byte	0xfd
        .byte	0x28
        .byte	0x4c
        .byte	0x24
        .byte	0x60
        call	HIDDEN_JUMPTARGET(\callee)
        movq	%rbp, %rsp
        cfi_def_cfa_register (%rsp)
        popq	%rbp
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%rbp)
        ret
.endm

/* 3 argument AVX512 ISA version as wrapper to AVX2 ISA version.  */
.macro WRAPPER_IMPL_AVX512_fFF callee
        pushq     %rbp
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%rbp, 0)
        movq      %rsp, %rbp
        cfi_def_cfa_register (%rbp)
        andq      $-64, %rsp
        pushq     %r12
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%r12, 0)
        pushq     %r13
        cfi_adjust_cfa_offset (8)
        cfi_rel_offset (%r13, 0)
        subq      $176, %rsp
        movq      %rsi, %r13
/* Below is encoding for vmovaps %zmm0, (%rsp).  */
        .byte	0x62
        .byte	0xf1
        .byte	0x7c
        .byte	0x48
        .byte	0x29
        .byte	0x04
        .byte	0x24
        movq    %rdi, %r12
/* Below is encoding for vmovapd (%rsp), %ymm0.  */
        .byte	0xc5
        .byte	0xfd
        .byte	0x28
        .byte	0x04
        .byte	0x24
        call      HIDDEN_JUMPTARGET(\callee)
/* Below is encoding for vmovapd 32(%rsp), %ymm0.  */
        .byte	0xc5
        .byte	0xfd
        .byte	0x28
        .byte	0x44
        .byte	0x24
        .byte	0x20
        lea       64(%rsp), %rdi
        lea       96(%rsp), %rsi
        call      HIDDEN_JUMPTARGET(\callee)
/* Below is encoding for vmovapd 64(%rsp), %ymm0.  */
        .byte	0xc5
        .byte	0xfd
        .byte	0x28
        .byte	0x44
        .byte	0x24
        .byte	0x40
/* Below is encoding for vmovapd   96(%rsp), %ymm1.  */
        .byte	0xc5
        .byte	0xfd
        .byte	0x28
        .byte	0x4c
        .byte	0x24
        .byte	0x60
/* Below is encoding for vmovapd   %ymm0, 32(%r12).  */
        .byte	0xc4
        .byte	0xc1
        .byte	0x7d
        .byte	0x29
        .byte	0x44
        .byte	0x24
        .byte	0x20
/* Below is encoding for vmovapd   %ymm1, 32(%r13).  */
        .byte	0xc4
        .byte	0xc1
        .byte	0x7d
        .byte	0x29
        .byte	0x4d
        .byte	0x20
        addq      $176, %rsp
        popq      %r13
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%r13)
        popq      %r12
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%r12)
        movq      %rbp, %rsp
        cfi_def_cfa_register (%rsp)
        popq      %rbp
        cfi_adjust_cfa_offset (-8)
        cfi_restore (%rbp)
        ret
.endm
