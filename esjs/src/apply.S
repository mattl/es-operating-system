/*
 * Copyright (c) 2007
 * Nintendo Co., Ltd.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */


        .align  2
        .globl  applyS32
        .globl  applyS64
        .globl  applyF32
        .globl  applyF64
        .globl  applyPTR

applyS32:
applyS64:
applyF32:
applyF64:
applyPTR:

#ifdef __i386__
        // apply(argc: 8(%ebp), argv: 12(%ebp), function: 16(%ebp));
        push    %ebp
        mov     %esp, %ebp

        movl    8(%ebp), %ecx
        and     %ecx, %ecx
        jz      break

        mov     %ecx, %eax
        mov     $20, %edx
        mul     %edx
        movl    12(%ebp), %edx
        add     %edx, %eax

while:
        sub     $20, %eax
        mov     16(%eax), %edx

        cmpl    $5, %edx        // is REF?
        jne     0f
        pushl   %eax
        loop    while
0:      andl    $1, %edx        // check parameter class
        jz      1f              // 32 bit value
        pushl   4(%eax)
1:      pushl   0(%eax)
        loop    while

break:
        call    *16(%ebp)
        leave
        ret
#endif  // __i386__

#ifdef __x86_64__
        // apply(argc: rdi, argv: rsi, function: rdx);
        // INTEGER: %rdi, %rsi, %rdx, %rcx, %r8 and %r9
        // SSE: %xmm0 to %xmm7
        // %r10: int count
        // %r11: sse count
        // %r12: argc
        // %r13: temp
        // %r14: param
        // %r15: function
        // %xmm8: param
        push    %rbp
        mov     %rsp,%rbp

        mov     %rdi, %r12
        mov     %rsi, %rax
        mov     %rdx, %r15

        xor     %r10, %r10
        xor     %r11, %r11
        cmp     $0, %r12
        jmp     begin

while:
        add     $24, %rax
        dec     %r12
begin:
        jz      break

        // Check parameter class
        cmp     $2, 16(%rax)     // F32
        jz      f32
        cmp     $3, 16(%rax)     // F64
        jz      f64
        cmp     $5, 16(%rax)     // REF
        jz      ref

int:    // INTEGER class
        inc     %r10
        cmp     $1, %r10
        jne     1f
        mov     (%rax), %rdi
        jmp     while
1:      cmp     $2, %r10
        jne     2f
        mov     (%rax), %rsi
        jmp     while
2:      cmp     $3, %r10
        jne     3f
        mov     (%rax), %rdx
        jmp     while
3:      cmp     $4, %r10
        jne     4f
        mov     (%rax), %rcx
        jmp     while
4:      cmp     $5, %r10
        jne     5f
        mov     (%rax), %r8
        jmp     while
5:      cmp     $6, %r10
        jne     6f
        mov     (%rax), %r9
        jmp     while
6:      cmp     $7, %r10
        jne     7f
        mov     (%rax), %r14
        push    %r14
        jmp     while
7:      pop     %r14
        mov     (%rax), %r13
        push    %r13
        push    %r14
        jmp     while

f32:    // SSE class
        inc     %r11
        cmp     $1, %r11
        jne     1f
        movss   (%rax), %xmm0
        jmp     while
1:      cmp     $2, %r11
        jne     2f
        movss   (%rax), %xmm1
        jmp     while
2:      cmp     $3, %r11
        jne     3f
        movss   (%rax), %xmm2
        jmp     while
3:      cmp     $4, %r11
        jne     4f
        movss   (%rax), %xmm3
        jmp     while
4:      cmp     $5, %r11
        jne     5f
        movss   (%rax), %xmm4
        jmp     while
5:      cmp     $6, %r11
        jne     6f
        movss   (%rax), %xmm5
        jmp     while
6:      cmp     $7, %r11
        jne     7f
        movss   (%rax), %xmm6
        jmp     while
7:      movss   (%rax), %xmm7
        jmp     while

f64:
        inc     %r11
        cmp     $1, %r11
        jne     1f
        movsd   (%rax), %xmm0
        jmp     while
1:      cmp     $2, %r11
        jne     2f
        movsd   (%rax), %xmm1
        jmp     while
2:      cmp     $3, %r11
        jne     3f
        movsd   (%rax), %xmm2
        jmp     while
3:      cmp     $4, %r11
        jne     4f
        movsd   (%rax), %xmm3
        jmp     while
4:      cmp     $5, %r11
        jne     5f
        movsd   (%rax), %xmm4
        jmp     while
5:      cmp     $6, %r11
        jne     6f
        movsd   (%rax), %xmm5
        jmp     while
6:      cmp     $7, %r11
        jne     7f
        movsd   (%rax), %xmm6
        jmp     while
7:      movsd   (%rax), %xmm7
        jmp     while

ref:
        inc     %r10
        cmp     $1, %r10
        jne     1f
        mov     %rax, %rdi
        jmp     while
1:      cmp     $2, %r10
        jne     2f
        mov     %rax, %rsi
        jmp     while
2:      cmp     $3, %r10
        jne     3f
        mov     %rax, %rdx
        jmp     while
3:      cmp     $4, %r10
        jne     4f
        mov     %rax, %rcx
        jmp     while
4:      cmp     $5, %r10
        jne     5f
        mov     %rax, %r8
        jmp     while
5:      cmp     $6, %r10
        jne     6f
        mov     %rax, %r9
        jmp     while
6:      cmp     $7, %r10
        jne     7f
        mov     %rax, %r14
        push    %r14
        jmp     while
7:      pop     %r14
        mov     %rax, %r13
        push    %r13
        push    %r14
        jmp     while

break:
        call    *%r15
        leave
        ret

#endif  // __x86_64__
