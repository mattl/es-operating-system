/*
 * Copyright 2008, 2009 Google Inc.
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

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <es.h>
#include <es/types.h>
#include <es/formatter.h>



extern "C"
{
    size_t strnlen(const char* string, size_t count);
}

Formatter::
Formatter(int (*putc)(int, void*), void* opt) throw() :
    mode(Mode::C), putc(putc), opt(opt)
{
    reset();
}

Formatter::
Formatter(es::Stream* stream) throw() :
    mode(Mode::C), putc(streamPutc), opt(stream)
{
    stream->addRef();
    reset();
}

Formatter::
Formatter(std::string& string) throw() :
    mode(Mode::C), putc(stringPutc), opt(&string)
{
    reset();
}

Formatter::
Formatter(const Formatter& o) throw() :
    mode(Mode::C), putc(o.putc), opt(o.opt)
{
    reset();
}

Formatter::
~Formatter()
{
    if (putc == streamPutc)
    {
        es::Stream* stream(static_cast<es::Stream*>(opt));
        stream->release();
    }
}

int Formatter::
printChar(int c)
{
    putc(c, opt);
    return 1;
}

int Formatter::
fillBlank(int count, char c)
{
    int n;

    for (n = 0; n < count; ++n)
    {
        printChar(c);
    }
    return n;
}

int Formatter::
print(char c)
{
    char s[2] = { c, '\0' };
    return print(s);
}

int Formatter::
print(const char* string)
{
    int count = 0;
    int n;

    if (precision < 0)
    {
        precision = INT_MAX;
    }
    if (!leftJustified)
    {
        n = (int) strnlen(string, precision);
        count += fillBlank(width - n, filler);
    }
    for (n = 0; *string && n < precision; ++n)
    {
        count += printChar(*string++);
    }
    if (leftJustified)
    {
        count += fillBlank(width - n, filler);
    }
    reset();
    return count;
}

template <typename I>
int Formatter::
printSigned(I n)
{
    if (0 <= n)
    {
        return printInteger(n);
    }
    else
    {
        sign = '-';
        return printInteger(-n);
    }
}

template <typename I>
int Formatter::
printUnsigned(I u)
{
    return printInteger(u);
}

template <typename I>
int Formatter::
printInteger(I u)
{
    static const int BUFSIZE = 32;
    int count = 0;
    int n;
    char buf[BUFSIZE];
    const char* prefix = 0;
    char* p;

    if (precision <= 0)
    {
        precision = 1;
    }

    if (u && alt)
    {
        if (base == 8)
        {
            prefix = "0";
        }
        else if (base == 16)
        {
            prefix = cap ? "0X" : "0x";
        }
    }

    p = &buf[BUFSIZE];
    do {
        if (cap)
            *--p = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[u % base];
        else
            *--p = "0123456789abcdefghijklmnopqrstuvwxyz"[u % base];
    } while ((u /= base) != 0);
    n = &buf[BUFSIZE] - p;

    width -= n;
    if (sign)
    {
        --width;
    }
    if (prefix)
    {
        width -= strlen(prefix);
    }
    n = precision - n;
    if (0 < n)
    {
        width -= n;
    }

    if (filler != '0' && !leftJustified)
    {
        count += fillBlank(width, filler);
    }

    if (sign != 0)
    {
        count += printChar(sign);
    }

    if (prefix)
    {
        while (*prefix)
        {
            count += printChar(*prefix++);
        }
    }

    count += fillBlank(n, '0');
    if (filler == '0')
    {
        count += fillBlank(width, '0');
    }

    while (p != &buf[BUFSIZE])
    {
        count += printChar(*p++);
    }

    if (filler != '0' && leftJustified)
    {
        count += fillBlank(width, filler);
    }

    return count;
}

int Formatter::
print(long n)
{
    int count = printSigned(n);
    reset();
    return count;
}

int Formatter::
print(unsigned long n)
{
    int count = printUnsigned(n);
    reset();
    return count;
}

int Formatter::
print(long long n)
{
    int count = printSigned(n);
    reset();
    return count;
}

int Formatter::
print(unsigned long long n)
{
    int count = printUnsigned(n);
    reset();
    return count;
}

// Get number of significant digits plus '.' ane 'e-dd'.
// Adjust precision to the number of digits to print.
template <typename U, int Bit>
int Formatter::
digitlen(int& k, int& dd, U f, U r)
{
    int  n;
    bool fixed = false;

    dd = -16383;
    switch (conversion)
    {
    case 'g':
        if (precision < 0)
        {
            precision = 6;
        }
        else if (precision == 0)
        {
            precision = 1;
        }
        if (mode == Mode::ECMAScript)
        {
            fixed = (-6 <= k && k < precision);
        }
        else
        {
            fixed = (-4 <= k && k < precision);
        }
        if (fixed)
        {
            // fixed
            if (!alt)
            {
                int sig = significantlen<U, Bit>(f, r);
                if (sig < precision)
                {
                    precision = sig;
                }
            }

            if (0 <= k)
            {
                n = 1 + k;
                if (precision < n)
                {
                    precision = n;
                }
                if (k - precision < -1)
                {
                    if (n < precision)
                    {
                        n += (precision - n);    // 000
                    }
                    ++n;    // .
                }
            }
            else
            {
                precision += -k;
                n = precision + 1;
            }
        }
        else
        {
            // scientific
            if (!alt)
            {
                int sig = significantlen<U, Bit>(f, r);
                if (sig < precision)
                {
                    precision = sig;
                }
            }

            n = (k < 0) ? -k : k;
            if (n < 100)
            {
                n = 2;  // dd
            }
            else if (n < 1000)
            {
                n = 3;  // ddd
            }
            else
            {
                n = 4;  // dddd
            }
            n += 2;     // e+
            n += precision;
            if (1 < precision)
            {
                ++n;
            }
            dd = k;
            k = 0;      // Adjust k
        }
        break;
    case 'f':
        if (precision < 0)
        {
            precision = 6;
        }

        n = 1;
        if (0 < k)
        {
            n += k;
        }
        if (precision != 0 || alt)
        {
            n += 1 + precision; // .ddd
        }

        ++precision;
        if (0 < k)
        {
            precision += k;
        }

        break;
    case 'e':
        if (precision < 0)
        {
            if (mode == Mode::ECMAScript)
            {
                precision = significantlen<U, Bit>(f, r) - 1;
            }
            else
            {
                precision = 6;
            }
        }

        n = (k < 0) ? -k : k;
        if (n < 100)
        {
            n = 2;      // dd
        }
        else if (n < 1000)
        {
            n = 3;      // ddd
        }
        else
        {
            n = 4;      // dddd
        }
        n += 3;         // d e+
        if (precision != 0 || alt)
        {
            n += 1 + precision; // .ddd
        }
        ++precision;
        dd = k;
        k = 0;          // Adjust k
        break;
    default:
        if (precision <= 0)
        {
            precision = significantlen<U, Bit>(f, r);
        }
        if (0 <= k)
        {
            n = 1 + k;
            if (precision < n)
            {
                precision = n;
            }
            if (k - precision < -1)
            {
                if (n < precision)
                {
                    n += (precision - n);    // 000
                }
                ++n;    // .
            }
        }
        else
        {
            precision += -k;
            n = precision + 1;
        }
        break;
    }

    if (sign != 0)
    {
        ++n;
    }

    return n;
}

template <typename U, int Bit>
int Formatter::
significantlen(U f, U r)
{
    U scale, high, low, d0, d1;
    int count = 0;

    high = f + r;
    low = f - r;
    scale = 1;
    r = f;
    do {
        ++count;
        r *= 10;
        scale *= 10;
        r &= (~((U) 0) >> 4);
        d0 = f - r / scale;
        d1 = d0 + (((U) 1) << (Bit - 4)) / scale;
        if (low < d0 || d1 < high)
        {
            break;
        }
    } while (r);
    return count;
}

//          float   double
// Bit      32      64          CHAR_BIT * sizeof(U)
// MantDig  24      53          MANT_DIG
// MaxExp   127     1023        MAX_EXP
template <typename U, int Bit, int MantDig, int MaxExp>
int Formatter::
printFloat(U x)
{
    int count = 0;
    int n, dd;
    int s, e, k = -1;
    U f, r, scale, high, low, d0, d1;
    int c1, c2;

    s = (int) (x >> (Bit - 1));
    e = (int) (((x >> (MantDig - 1)) & (((U) 1 << (Bit - MantDig)) - 1)) - MaxExp);
    f = x & (((U) 1 << (MantDig - 1)) - 1);

    if (s)
    {
        sign = '-';
    }

    if (e != -MaxExp)
    {
        f |= ((U) 1 << (MantDig - 1));
        r = ((U) 1) << (Bit - MantDig - 4 - 1);
    }
    else if (f == 0)
    {
        e = 0;
        r = 0;
    }
    else
    {
        // Normalize
        while (!(f & ((U) 1 << (MantDig - 1))))
        {
            f <<= 1;
            --e;
        }
        ++e;
        r = ((U) 1) << (Bit - MantDig + (-MaxExp - e) - 4 - 2);
    }

    if (e == MaxExp + 1)
    {
        char seq[10];
        char* p = seq;
        int prec;

        if (sign != 0)
        {
            *p++ = sign;
            prec = 4;
        }
        else
        {
            prec = 3;
        }
        if (f == ((U) 1 << (MantDig - 1)))
        {
            if (mode == Mode::ECMAScript)
            {
                prec += 5;
                strcpy(p, "Infinity");
            }
            else
            {
                strcpy(p, cap ? "INF" : "inf");
            }
        }
        else
        {
            if (mode == Mode::ECMAScript)
            {
                strcpy(p, "NaN");
            }
            else
            {
                strcpy(p, cap ? "NAN" : "nan");
            }
        }
        setPrecision(prec);
        return count + print(seq);
    }

    // Find k and set up f. In f, we will store 28 bits of fraciton bits.
    if (f == 0)
    {
        k = 0;
    }
    else if (0 <= e)
    {
        f <<= Bit - MantDig;
        do {
            f /= 10;
            ++k;
            while (!(f & ((U) 1 << (Bit - 1))))
            {
                f <<= 1;
                --e;
            }
        } while (0 <= e);
        f >>= 4 - e - 1;
    }
    else
    {
        f <<= (Bit - MantDig) - 4;
        f &= (~((U) 0) >> 4);
        while (e < -4)
        {
            f *= 10;
            --k;
            while (f & ((U) 0xf << (Bit - 4)))
            {
                f >>= 1;
                e += 1;
            }
        }
        f >>= -e - 1;
        f &= (~((U) 0) >> 4);
    }

    n = digitlen<U, Bit>(k, dd, f, r);
    width -= n;

    if (filler != '0' && !leftJustified)
    {
        count += fillBlank(width, filler);
    }
    if (sign != 0)
    {
        count += printChar(sign);
    }
    if (filler == '0' && !leftJustified)
    {
        count += fillBlank(width, filler);
    }

    if (k < 0)
    {
        int j = 0;
        u32 d = (((f * 10) >> (Bit - 4)) < 5) ? 0 : 1;

        do {
            --precision;
            if (j-- == -1)
            {
                count += printChar('.');
            }
            if (0 < precision || k < j)
            {
                count += printChar('0');
            }
            else
            {
                count += printChar('0' + d);
            }
        } while (k < j && 0 < precision);
        k = j;
    }

    if (0 < precision)
    {
        ASSERT(f == 0 || ((U) 1 << (Bit - 4)) / 10 <= f);
        high = f + r;
        low = f - r;
        scale = 1;
        r = f;
        do {
            --precision;
            if (k-- == -1)
            {
                count += printChar('.');
            }

            r *= 10;
            u32 d = (u32) (r >> (Bit - 4));
            scale *= 10;
            r &= (~((U) 0) >> 4);
            d0 = f - r / scale;
            d1 = d0 + (((U) 1) << (Bit - 4)) / scale;
            c1 = (low < d0);
            c2 = (d1 < high);
            if (c1 && !c2)
            {
                count += printChar('0' + d);
                break;
            }
            else if (!c1 && c2)
            {
                count += printChar('0' + d + 1);
                break;
            }
            else if (c1 && c2)
            {
                if (f - d0 <= d1 - f)
                {
                    count += printChar('0' + d);
                }
                else
                {
                    count += printChar('0' + d + 1);
                }
                break;
            }

            if (0 < precision)
            {
                count += printChar('0' + d);
            }
            else
            {
                count += printChar('0' + d + ((((r * 10) >> (Bit - 4)) < 5) ? 0 : 1));
                break;
            }
        } while (r);
    }

    if (0 < precision)
    {
        do {
            if (k == -1)
            {
                count += printChar('.');
            }
            count += printChar('0');
            --k;
        } while (0 < --precision);
    }

    count += fillBlank(k, '0');
    if (k == -1 && alt && conversion != 'g')
    {
        count += printChar('.');
    }

    if (-16383 < dd)
    {
        count += printChar(cap ? 'E' : 'e');
        Formatter t(*this);
        t.dec();
        t.setFill('0');
        t.setPrecision(2);
        t.showPos('+');
        count += t.printSigned((long) dd);
    }

    if (filler != '0' && leftJustified)
    {
        count += fillBlank(width, filler);
    }

    return count;
}

int Formatter::
print(float x)
{
    int count = printFloat<u32, sizeof(u32) * 8, 24, 127>(*(u32*) &x);
    reset();
    return count;
}

int Formatter::
print(double x)
{
    int count = printFloat<u64, sizeof(u64) * 8, 53, 1023>(*(u64*) &x);
    reset();
    return count;
}

// spec: %[-+ #0]*[{integer}*](.[{integer}*]?)?0[hljztL]*[dibouxXfFeEgGaAcspn%]
int Formatter::
format(const char* spec, va_list args)
{
    int count;      // number of printed charaters
    size_t len;     // length modifiers
    int precision;
    int width;
    bool unsignedNumber = false;

    for (count = 0; *spec != 0; ++spec)
    {
        if (*spec != '%')
        {
            count += printChar(*spec);
            continue;
        }
        ++spec;

        // Flags
        reset();
        for (;;)
        {
            char c = *spec;
            switch (c)
            {
              case '-':
                left();
                break;
              case '+':
                showPos('+');
                break;
              case ' ':
                if (showPos() != '+')
                {
                    showPos(' ');
                }
                break;
              case '#':
                showBase(true);
                break;
              case '0':
                setFill('0');
                break;
              default:
                goto FieldWidth;
                break;
            }
            ++spec;
        }

        // Field width
FieldWidth:
        width = 0;
        if (isdigit(*spec))
        {
            do {
                width = 10 * width + (*spec++ - '0');
            } while (isdigit(*spec));
        }
        else if (*spec == '*')
        {
            width = va_arg(args, int);
            ++spec;
            if (width < 0)
            {
                left();
                width = -width;
            }
        }
        setWidth(width);

        // Precision
        precision = -1;
        if (*spec == '.')
        {
            ++spec;
            if (isdigit(*spec))
            {
                precision = 0;
                do {
                    precision = 10 * precision + (*spec++ - '0');
                } while(isdigit(*spec));
            }
            else if (*spec == '*')
            {
                precision = va_arg(args, int);
                ++spec;
                if (precision < 0)
                {
                    precision = -1; // default
                }
            }
        }
        setPrecision(precision);

        // Length modifier
        len = sizeof(int);
        for (;;)
        {
            char c = *spec;
            switch (c)
            {
              case 'h':
                len = sizeof(char);
                if (spec[1] == 'h')
                {
                    ++spec;
                    len = sizeof(short);
                }
                break;
              case 'l':
                len = sizeof(long);
                if (spec[1] == 'l')
                {
                    ++spec;
                    len = sizeof(long long);
                }
                break;
              case 'j':
                len = sizeof(long long);
                break;
              case 'z':
                len = sizeof(size_t);
                break;
              case 't':
                len = sizeof(ptrdiff_t);
                break;
              case 'L':
                len = sizeof(long double);
                break;
              default:
                goto ConversionSpecifier;
                break;
            }
            ++spec;
        }

        // Cconversion specifier
ConversionSpecifier:
        switch (*spec) {
        case 'd':
        case 'i':
            dec();
            switch (len)
            {
            case 1:
                count += print((long) (signed char) va_arg(args, int));
                break;
            case 2:
                count += print((short) va_arg(args, int));
                break;
            case 4:
                count += print(va_arg(args, int));
                break;
            case 8:
                count += print(va_arg(args, long long));
                break;
            }
            break;
        case 'b':
            bin();
            unsignedNumber = true;
            break;
        case 'o':
            oct();
            unsignedNumber = true;
            break;
        case 'u':
            dec();
            unsignedNumber = true;
            break;
        case 'p':
        case 'X':
            uppercase(true);
            // FALL THROUGH
        case 'x':
            hex();
            unsignedNumber = true;
            break;
        case 'c':
            count += print((char) va_arg(args, int));
            break;
        case 's':
            count += print(va_arg(args, const char*));
            break;
        case '\0':
            --spec;
            break;
        case 'n':
            switch (len)
            {
            case 1:
                *va_arg(args, signed char*) = (signed char) count;
                break;
            case 2:
                *va_arg(args, short*) = (short) count;
                break;
            case 4:
                *va_arg(args, int*) = count;
                break;
            case 8:
                 *va_arg(args, long long*) = count;
                break;
            }
            break;

        case 'F':
            uppercase(true);
            // FALL THROUGH
        case 'f':
            fixed();
            {
                double x = va_arg(args, double);
                count += printFloat<u64, sizeof(u64) * 8, 53, 1023>(*(u64*) &x);
            }
            break;

        case 'E':
            uppercase(true);
            // FALL THROUGH
        case 'e':
            scientific();
            {
                double x = va_arg(args, double);
                count += printFloat<u64, sizeof(u64) * 8, 53, 1023>(*(u64*) &x);
            }
            break;

        case 'G':
            uppercase(true);
            // FALL THROUGH
        case 'g':
            general();
            {
                double x = va_arg(args, double);
                count += printFloat<u64, sizeof(u64) * 8, 53, 1023>(*(u64*) &x);
            }
            break;

        case '%':
        default:
            count += print(*spec);
            break;
        }

        if (unsignedNumber)
        {
            switch (len)
            {
            case 1:
                count += print((unsigned long) (unsigned char) va_arg(args, unsigned));
                break;
            case 2:
                count += print((unsigned short) va_arg(args, unsigned));
                break;
            case 4:
                count += print(va_arg(args, unsigned));
                break;
            case 8:
                count += print(va_arg(args, unsigned long long));
                break;
            }
        }
        unsignedNumber = false;
    }

    reset();
    return count;
}
