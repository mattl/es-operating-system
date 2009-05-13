/*
 * Copyright 2008, 2009 Google Inc.
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

// cf. http://www.codesourcery.com/cxx-abi/abi.html#mangling

#ifdef __i386__

#define VSIZE   $12     // sizeof(Any)
#define VTYPE   8       // offsetof(Any, type)

#define VBOOL   $1
#define VU8     $2
#define VS16    $3
#define VU16    $4
#define VS32    $5
#define VU32    $6
#define VS64    $7
#define VU64    $8
#define VFLT    $9
#define VDBL    $10
#define VSTRING $11
#define VOBJECT $12

#define VVARIANT    $0x80000000

        .text

// Any apply(int argc, Any* argv, Any (*function)());
        .globl  _Z5applyiP3AnyPFS_vE
_Z5applyiP3AnyPFS_vE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VVARIANT, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, bool (*function)());
        .globl _Z5applyiP3AnyPFbvE
_Z5applyiP3AnyPFbvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VBOOL, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, uint8_t (*function)());
        .globl _Z5applyiP3AnyPFhvE
_Z5applyiP3AnyPFhvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VU8, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, int16_t (*function)());
        .globl _Z5applyiP3AnyPFsvE
_Z5applyiP3AnyPFsvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VS16, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, uint16_t (*function)());
        .globl _Z5applyiP3AnyPFtvE
_Z5applyiP3AnyPFtvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VU16, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, int32_t (*function)());
        .globl _Z5applyiP3AnyPFivE
_Z5applyiP3AnyPFivE:
        .globl _Z5applyiP3AnyPFlvE
_Z5applyiP3AnyPFlvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VS32, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, uint32_t (*function)());
        .globl _Z5applyiP3AnyPFjvE
_Z5applyiP3AnyPFjvE:
        .globl _Z5applyiP3AnyPFmvE
_Z5applyiP3AnyPFmvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VU32, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, int64_t (*function)());
        .globl _Z5applyiP3AnyPFxvE
_Z5applyiP3AnyPFxvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VS64, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, uint64_t (*function)());
        .globl _Z5applyiP3AnyPFyvE
_Z5applyiP3AnyPFyvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VU64, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, float (*function)());
        .globl _Z5applyiP3AnyPFfvE
_Z5applyiP3AnyPFfvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VFLT, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, double (*function)());
        .globl _Z5applyiP3AnyPFdvE
_Z5applyiP3AnyPFdvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VDBL, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, const char* (*function)());
        .globl _Z5applyiP3AnyPFPKcvE
_Z5applyiP3AnyPFPKcvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VSTRING, VTYPE(%eax)
        jmp     apply

// Any apply(int argc, Any* argv, Object* (*function)());
        .globl _Z5applyiP3AnyPFP6ObjectvE
_Z5applyiP3AnyPFP6ObjectvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VOBJECT, VTYPE(%eax)
        jmp     apply

apply:
        movl    12(%ebp), %ecx  // argc
        andl    %ecx, %ecx
        jz      break
        leal    (%ecx, %ecx, 2), %eax   // %eax = 3 * %ecx
        sall    $2, %eax                // %eax = VSIZE(3 * 4) * %ecx
        addl    16(%ebp), %eax          // %eax = argv + VSIZE(12) * %ecx
while:
        subl    VSIZE, %eax
        movl    VTYPE(%eax), %edx
        testl   %edx, %edx
        jns     0f                      // Test VVARIANT flag
        // Push Any as it is
        andl    $0x7fffffff, %edx
        pushl   %edx
        pushl   4(%eax)
        pushl   0(%eax)
        loop    while
        jmp     break
0:
        cmpl    VOBJECT, %edx
        ja      4f
        jmp     *0f(,%edx,4)
        .section        .rodata
        .align 4
0:
        .long   4f      // any XXX
        .long   1f      // bool
        .long   1f      // u8
        .long   2f      // s16
        .long   2f      // u16
        .long   4f      // s32
        .long   4f      // u32
        .long   3f      // s64
        .long   3f      // u64
        .long   4f      // flt
        .long   3f      // dbl
        .long   4f      // string
        .long   4f      // object
        .text
1:
        movzbl  (%eax), %edx
        pushl   %edx
        loop    while
        jmp     break
2:
        movzwl  (%eax), %edx
        pushl   %edx
        loop    while
        jmp     break
3:
        pushl   4(%eax)
4:
        pushl   0(%eax)
        loop    while
break:

        movl    8(%ebp), %eax   // pointer to the return value
        movl    20(%ebp), %edx  // pointer to the function
        cmpl    VVARIANT, VTYPE(%eax)
        jne     0f

        // variant function
        pushl   %eax
        call    *%edx
        leave
        ret

0:      // non-variant function
        call    *%edx
        movl    8(%ebp), %ecx   // pointer to the return value
        movl    %eax, 0(%ecx)
        movl    %edx, 4(%ecx)
        movl    VTYPE(%ecx), %edx
        cmpl    VFLT, %edx
        jne     1f
        fstps   (%ecx)
        jmp     2f
1:
        cmpl    VDBL, %edx
        jne     2f
        fstpl   (%ecx)
2:
        movl    %ecx, %eax
        leave
        ret

// long long evaluate(const Any& variant);
_Z8evaluateRK3Any:
        .globl _Z8evaluateRK3Any
        pushl   %ebp
        movl    %esp, %ebp
        movl    0x8(%ebp),%eax
        movl    VTYPE(%eax), %edx
        cmpl    VOBJECT, %edx
        ja      6f
        jmp     *0f(,%edx,4)
        .section        .rodata
        .align 4
0:
        .long   6f      // any XXX
        .long   1f      // bool
        .long   1f      // u8
        .long   2f      // s16
        .long   2f      // u16
        .long   6f      // s32
        .long   6f      // u32
        .long   5f      // s64
        .long   5f      // u64
        .long   3f      // flt
        .long   4f      // dbl
        .long   6f      // string
        .long   6f      // object
        .text
1:      // return bool, uint8_t
        movzbl  (%eax), %eax
        leave
        ret
2:      // return int16_t, uint16_t
        movzwl  (%eax), %eax
        leave
        ret
3:      // return float
        flds    (%eax)
        leave
        ret
4:      // return double
        fldl    (%eax)
        leave
        ret
5:      // return int64_t, uint64_t
        movl    0x4(%eax), %edx
6:      // return int32_t, uint32_t, *
        movl    (%eax), %eax
        leave
        ret

#endif  // __i386__
