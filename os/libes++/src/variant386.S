/*
 * Copyright 2008 Google Inc.
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

#define VSIZE   $12     // sizeof(Variant)
#define VTYPE   8       // offsetof(Variant, type)

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
#define VGUID   $13

#define VVARIANT    $0x80000000

        .text

// Variant apply(int argc, Variant* argv, Variant (*function)());
        .globl  _Z5applyiP7VariantPFS_vE
_Z5applyiP7VariantPFS_vE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    12(%ebp), %ecx  // argc

        andl    %ecx, %ecx
        jz      2f
        movl    %ecx, %eax
        movl    VSIZE, %edx
        mull    %edx
        movl    16(%ebp), %edx  // argv
        addl    %edx, %eax
0:                              // while
        subl    VSIZE, %eax
        movl    VTYPE(%eax), %edx

        testl   %edx, %edx
        jns     3f
        // push Variatn as it is
        andl    $0x7ffffff, %edx
        pushl   %edx
        pushl   4(%eax)
        pushl   0(%eax)

        loop    0b
        jmp     2f

3:
        cmpl    VS64, %edx
        je      1f
        cmpl    VU64, %edx
        je      1f
        cmpl    VDBL, %edx
        je      1f
        pushl   0(%eax)
        loop    0b
        jmp     2f
1:
        pushl   4(%eax)
        pushl   0(%eax)
        loop    0b
2:                              // break
        pushl   8(%ebp)         // pointer to the return value
        movl    20(%ebp), %edx
        call    *%edx

        movl    8(%ebp), %eax   // pointer to the return value
        movl    %ebp, %esp
        popl    %ebp
        ret

// Variant apply(int argc, Variant* argv, bool (*function)());
        .globl _Z5applyiP7VariantPFbvE
_Z5applyiP7VariantPFbvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VBOOL, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, uint8_t (*function)());
        .globl _Z5applyiP7VariantPFhvE
_Z5applyiP7VariantPFhvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VU8, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, int16_t (*function)());
        .globl _Z5applyiP7VariantPFsvE
_Z5applyiP7VariantPFsvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VS16, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, uint16_t (*function)());
        .globl _Z5applyiP7VariantPFtvE
_Z5applyiP7VariantPFtvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VU16, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, int32_t (*function)());
        .globl _Z5applyiP7VariantPFivE
_Z5applyiP7VariantPFivE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VS32, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, uint32_t (*function)());
        .globl _Z5applyiP7VariantPFjvE
_Z5applyiP7VariantPFjvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VU32, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, int64_t (*function)());
        .globl _Z5applyiP7VariantPFxvE
_Z5applyiP7VariantPFxvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VS64, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, uint64_t (*function)());
        .globl _Z5applyiP7VariantPFyvE
_Z5applyiP7VariantPFyvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VU64, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, float (*function)());
        .globl _Z5applyiP7VariantPFfvE
_Z5applyiP7VariantPFfvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VFLT, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, double (*function)());
        .globl _Z5applyiP7VariantPFdvE
_Z5applyiP7VariantPFdvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VDBL, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, const char* (*function)());
        .globl _Z5applyiP7VariantPFPKcvE
_Z5applyiP7VariantPFPKcvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VSTRING, VTYPE(%eax)
        jmp     apply

// Variant apply(int argc, Variant* argv, es::IInterface* (*function)());
        .globl _Z5applyiP7VariantPFPN2es10IInterfaceEvE
_Z5applyiP7VariantPFPN2es10IInterfaceEvE:
        pushl   %ebp
        movl    %esp, %ebp
        movl    8(%ebp), %eax   // pointer to the return value
        movl    VOBJECT, VTYPE(%eax)
        jmp     apply

apply:
        movl    12(%ebp), %ecx  // argc
        andl    %ecx, %ecx
        jz      2f
        movl    %ecx, %eax
        movl    VSIZE, %edx
        mull    %edx
        movl    16(%ebp), %edx  // argv
        addl    %edx, %eax
0:                              // while
        subl    VSIZE, %eax
        movl    VTYPE(%eax), %edx

        testl   %edx, %edx
        jns     3f
        // push Variatn as it is
        andl    $0x7ffffff, %edx
        pushl   %edx
        pushl   4(%eax)
        pushl   0(%eax)

        loop    0b
        jmp     2f

3:
        cmpl    VS64, %edx
        je      1f
        cmpl    VU64, %edx
        je      1f
        cmpl    VDBL, %edx
        je      1f
        pushl   0(%eax)
        loop    0b
        jmp     2f
1:
        pushl   4(%eax)
        pushl   0(%eax)
        loop    0b
2:                              // break
        movl    20(%ebp), %edx
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
        movl    %ebp, %esp
        popl    %ebp
        ret

// long long evaluate(const Variant& variant);
_Z8evaluateRK7Variant:
        .globl _Z8evaluateRK7Variant
        pushl   %ebp
        movl    %esp, %ebp
        movl    0x8(%ebp),%eax

        movl    VTYPE(%eax), %edx
        cmpl    VBOOL, %edx
        je      1f
        cmpl    VU8, %edx
        je      1f
        cmpl    VS16, %edx
        je      2f
        cmpl    VU16, %edx
        je      2f
        cmpl    VFLT, %edx
        je      3f
        cmpl    VDBL, %edx
        je      4f
        cmpl    VS64, %edx
        je      5f
        cmpl    VU64, %edx
        je      5f
        jmp     6f

1:      // return bool, uint8_t
        movzbl  (%eax), %eax
        jmp     return
2:      // return int16_t, uint16_t
        movzwl  (%eax), %eax
        jmp     return
3:      // return float
        flds    (%eax)
        jmp     return
4:      // return double
        fldl    (%eax)
        jmp     return
5:      // return int64_t, uint64_t
        movl    0x4(%eax), %edx
6:      // return int32_t, uint32_t, *
        movl    (%eax), %eax

return:
        movl    %ebp, %esp
        popl    %ebp
        ret

#endif  // __i386__
