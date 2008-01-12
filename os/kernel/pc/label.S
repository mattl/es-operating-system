/*
 * Copyright (c) 2006
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
