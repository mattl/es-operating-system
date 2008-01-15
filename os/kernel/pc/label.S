/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

// cf. http://www.codesourcery.com/cxx-abi/abi.html#mangling

// int Label::set();
        .align  2
        .globl  _ZN5Label3setEv
_ZN5Label3setEv:
        movl    4(%esp), %eax
        movl    %ebp, 8(%eax)
        movl    %ebx, 12(%eax)
        movl    %esi, 16(%eax)
        movl    %edi, 20(%eax)
        leal    4(%esp), %ecx
        movl    %ecx, 0(%eax)
        movl    0(%esp), %ecx
        movl    %ecx, 4(%eax)
        movl    0(%ebp), %ecx
        movl    %ecx, 24(%eax)
        xorl    %eax, %eax      // return 0
        ret

// void Label::jump();
        .align  2
        .globl  _ZN5Label4jumpEv
_ZN5Label4jumpEv:
        movl    4(%esp), %ecx
        movl    4(%ecx), %edx
        movl    12(%ecx), %ebx
        movl    16(%ecx), %esi
        movl    20(%ecx), %edi
        movl    8(%ecx), %ebp
        movl    0(%ecx), %esp
        movl    $1, %eax        // return 1
        jmp     *%edx

// void Label::start();
        .align  2
        .globl  _ZN5Label5startEv
_ZN5Label5startEv:
        call    *%ebx
