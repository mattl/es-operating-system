/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef __i386__

//s32 applyS32(int argc, Param* argv, s32 (*function)())
        .text
        .globl  applyS32
        .globl  applyS64
        .globl  applyF32
        .globl  applyF64
        .globl  applyPTR
        .globl  _applyS32
        .globl  _applyS64
        .globl  _applyF32
        .globl  _applyF64
        .globl  _applyPTR
applyS32:
applyS64:
applyF32:
applyF64:
applyPTR:
_applyS32:
_applyS64:
_applyF32:
_applyF64:
_applyPTR:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %ecx   // argc

        and     %ecx, %ecx
        jz      3f
        mov     %ecx, %eax
        mov     $20, %edx
        mul     %edx
        movl    12(%ebp), %edx  // argv
        add     %edx, %eax
0:                              // while
        sub     $20, %eax
        mov     16(%eax), %edx
        cmpl    $5, %edx        // is REF?
        jne     1f
        pushl   %eax
        loop    0b
1:      andl    $1, %edx        // check parameter class
        jz      2f              // 32 bit value
        pushl   4(%eax)
2:      pushl   0(%eax)
        loop    0b
3:                              // break
        movl    16(%ebp), %edx
        call    *%edx

        movl    %ebp, %esp
        popl    %ebp
        ret

#endif  // __i386__
