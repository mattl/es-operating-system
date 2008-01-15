/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <es/apply.h>

#ifdef __i386__

s32 applyS32(int argc, Param* argv, s32 (*function)())
{
     __asm__ __volatile__ (
        "and     %0, %0\n"
        "jz      break\n"
        "mov     %0, %%eax\n"
        "mov     $20, %%edx\n"
        "mul     %%edx\n"
        "movl    %1, %%edx\n"
        "add     %%edx, %%eax\n"
    "while:\n"
        "sub     $20, %%eax\n"
        "mov     16(%%eax), %%edx\n"
        "cmpl    $5, %%edx\n"        // is REF?
        "jne     0f\n"
        "pushl   %%eax\n"
        "loop    while\n"
    "0:  andl    $1, %%edx\n"       // check parameter class
        "jz      1f\n"              // 32 bit value
        "pushl   4(%%eax)\n"
    "1:  pushl   0(%%eax)\n"
        "loop    while\n"
    "break:\n"
        :: "c"(argc), "r"(argv) : "%eax", "%edx");

    return function();
}

#endif  // __i386__

#ifdef __x86_64__

s32 applyS32(int argc, Param* argv, s32 (*function)())
{
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

     __asm__ __volatile__ (
        "mov     %%rdi, %%r12\n"
        "mov     %%rsi, %%rax\n"
        "mov     %%rdx, %%r15\n"

        "xor     %%r10, %%r10\n"
        "xor     %%r11, %%r11\n"
        "cmp     $0, %%r12\n"
        "jmp     begin\n"

"while:\n"
        "add     $24, %%rax\n"
        "dec     %%r12\n"
"begin:\n"
        "jz      break\n"

        // Check parameter class
        "cmp     $2, 16(%%rax)\n"     // F32
        "jz      f32\n"
        "cmp     $3, 16(%%rax)\n"     // F64
        "jz      f64\n"
        "cmp     $5, 16(%%rax)\n"     // REF
        "jz      ref\n"

"int:\n"    // INTEGER class
        "inc     %%r10\n"
        "cmp     $1, %%r10\n"
        "jne     1f\n"
        "mov     (%%rax), %%rdi\n"
        "jmp     while\n"
"1:      cmp     $2, %%r10\n"
        "jne     2f\n"
        "mov     (%%rax), %%rsi\n"
        "jmp     while\n"
"2:      cmp     $3, %%r10\n"
        "jne     3f\n"
        "mov     (%%rax), %%rdx\n"
        "jmp     while\n"
"3:      cmp     $4, %%r10\n"
        "jne     4f\n"
        "mov     (%%rax), %%rcx\n"
        "jmp     while\n"
"4:      cmp     $5, %%r10\n"
        "jne     5f\n"
        "mov     (%%rax), %%r8\n"
        "jmp     while\n"
"5:      cmp     $6, %%r10\n"
        "jne     6f\n"
        "mov     (%%rax), %%r9\n"
        "jmp     while\n"
"6:      cmp     $7, %%r10\n"
        "jne     7f\n"
        "mov     (%%rax), %%r14\n"
        "push    %%r14\n"
        "jmp     while\n"
"7:      pop     %%r14\n"
        "mov     (%%rax), %%r13\n"
        "push    %%r13\n"
        "push    %%r14\n"
        "jmp     while\n"

"f32:\n"    // SSE class
        "inc     %%r11\n"
        "cmp     $1, %%r11\n"
        "jne     1f\n"
        "movss   (%%rax), %%xmm0\n"
        "jmp     while\n"
"1:      cmp     $2, %%r11\n"
        "jne     2f\n"
        "movss   (%%rax), %%xmm1\n"
        "jmp     while\n"
"2:      cmp     $3, %%r11\n"
        "jne     3f\n"
        "movss   (%%rax), %%xmm2\n"
        "jmp     while\n"
"3:      cmp     $4, %%r11\n"
        "jne     4f\n"
        "movss   (%%rax), %%xmm3\n"
        "jmp     while\n"
"4:      cmp     $5, %%r11\n"
        "jne     5f\n"
        "movss   (%%rax), %%xmm4\n"
        "jmp     while\n"
"5:      cmp     $6, %%r11\n"
        "jne     6f\n"
        "movss   (%%rax), %%xmm5\n"
        "jmp     while\n"
"6:      cmp     $7, %%r11\n"
        "jne     7f\n"
        "movss   (%%rax), %%xmm6\n"
        "jmp     while\n"
"7:      movss   (%%rax), %%xmm7\n"
        "jmp     while\n"

"f64:\n"
        "inc     %%r11\n"
        "cmp     $1, %%r11\n"
        "jne     1f\n"
        "movsd   (%%rax), %%xmm0\n"
        "jmp     while\n"
"1:      cmp     $2, %%r11\n"
        "jne     2f\n"
        "movsd   (%%rax), %%xmm1\n"
        "jmp     while\n"
"2:      cmp     $3, %%r11\n"
        "jne     3f\n"
        "movsd   (%%rax), %%xmm2\n"
        "jmp     while\n"
"3:      cmp     $4, %%r11\n"
        "jne     4f\n"
        "movsd   (%%rax), %%xmm3\n"
        "jmp     while\n"
"4:      cmp     $5, %%r11\n"
        "jne     5f\n"
        "movsd   (%%rax), %%xmm4\n"
        "jmp     while\n"
"5:      cmp     $6, %%r11\n"
        "jne     6f\n"
        "movsd   (%%rax), %%xmm5\n"
        "jmp     while\n"
"6:      cmp     $7, %%r11\n"
        "jne     7f\n"
        "movsd   (%%rax), %%xmm6\n"
        "jmp     while\n"
"7:      movsd   (%%rax), %%xmm7\n"
        "jmp     while\n"

"ref:\n"
        "inc     %%r10\n"
        "cmp     $1, %%r10\n"
        "jne     1f\n"
        "mov     %%rax, %%rdi\n"
        "jmp     while\n"
"1:      cmp     $2, %%r10\n"
        "jne     2f\n"
        "mov     %%rax, %%rsi\n"
        "jmp     while\n"
"2:      cmp     $3, %%r10\n"
        "jne     3f\n"
        "mov     %%rax, %%rdx\n"
        "jmp     while\n"
"3:      cmp     $4, %%r10\n"
        "jne     4f\n"
        "mov     %%rax, %%rcx\n"
        "jmp     while\n"
"4:      cmp     $5, %%r10\n"
        "jne     5f\n"
        "mov     %%rax, %%r8\n"
        "jmp     while\n"
"5:      cmp     $6, %%r10\n"
        "jne     6f\n"
        "mov     %%rax, %%r9\n"
        "jmp     while\n"
"6:      cmp     $7, %%r10\n"
        "jne     7f\n"
        "mov     %%rax, %%r14\n"
        "push    %%r14\n"
        "jmp     while\n"
"7:      pop     %%r14\n"
        "mov     %%rax, %%r13\n"
        "push    %%r13\n"
        "push    %%r14\n"
        "jmp     while\n"

"break:\n"
        :::);

    return function();
}

#endif  // __x86_64__

s64 applyS64(int argc, Param* argv, s64 (*function)()) __attribute__((alias("applyS32")));
f32 applyF32(int argc, Param* argv, f32 (*function)()) __attribute__((alias("applyS32")));
f64 applyF64(int argc, Param* argv, f64 (*function)()) __attribute__((alias("applyS32")));
void* applyPTR(int argc, Param* argv, const void* (*function)()) __attribute__((alias("applyS32")));
