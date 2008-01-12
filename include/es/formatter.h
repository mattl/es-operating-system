/*
 * Copyright (c) 2006, 2007
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

#ifndef NINTENDO_ES_FORMATTER_H_INCLUDED
#define NINTENDO_ES_FORMATTER_H_INCLUDED

#include <string>
#include <stdarg.h>
#include <es/base/IStream.h>

/**
 * This class provides methods to output a formatted string.
 */
class Formatter
{
    char    filler;
    int     width;
    int     precision;
    bool    leftJustified;
    bool    alt;            // show base
    bool    cap;            // uppercase
    char    sign;
    int     base;
    char    conversion;     // floating-point conversion type
    int   (*putc)(int, void*);
    void*   opt;

    int fillBlank(int count, char c);

    int printChar(int c);

    template <typename I>
    int printSigned(I s);

    template <typename I>
    int printUnsigned(I u);

    template <typename I>
    int printInteger(I u);

    template <typename U, int Bit, int MantDig, int MaxExp>
    int printFloat(U x);

    static const int MAXBUF = 32;   // 509 is ANSI minimum

    void reset()
    {
        filler = ' ';
        width = 0;
        precision = -1;

        leftJustified = false;
        alt = false;
        cap = false;
        sign = 0;
        base = 10;

        conversion = 0;
    }

    int getPrecision() const
    {
        return precision;
    }

    int setPrecision(int n)
    {
        precision = n;
        return precision;
    }

    int getWidth() const
    {
        return width;
    }

    int setWidth(int n)
    {
        width = n;
        return width;
    }

    char getFill() const
    {
        return filler;
    }

    char setFill(char c)
    {
        filler = c;
        return filler;
    }

    bool left()
    {
        leftJustified = true;
        return leftJustified;
    }

    bool right()
    {
        leftJustified = false;
        return !leftJustified;
    }

    bool showBase() const
    {
        return alt;
    }

    bool showBase(bool show)
    {
        alt = show;
        return alt;
    }

    bool uppercase() const
    {
        return cap;
    }

    bool uppercase(bool uppercase)
    {
        cap = uppercase;
        return !cap;
    }

    int bin()
    {
        return setBase(2);
    }

    int oct()
    {
        return setBase(8);
    }

    int dec()
    {
        return setBase(10);
    }

    int hex()
    {
        return setBase(16);
    }

    int getBase() const
    {
        return base;
    }

    int setBase(int n)
    {
        base = n;
        return base;
    }

    char showPos() const
    {
        return sign;
    }

    char showPos(char pos)
    {
        sign = pos;
        return sign;
    }

    // Floating-Point Output
    void general();
    void scientific();
    void fixed();

    template <typename U, int Bit>
    int digitlen(int& k, int& dd, U f, U r);

    template <typename U, int Bit>
    int significantlen(U f, U r);

    static int streamPutc(int c, void* opt)
    {
        IStream* stream(static_cast<IStream*>(opt));
        if (stream)
        {
            char ch(static_cast<char>(c));
            return stream->write(&ch, 1);
        }
        return 0;
    }

    static int stringPutc(int c, void* opt)
    {
        std::string* string(reinterpret_cast<std::string*>(opt));
        if (string)
        {
            char ch(static_cast<char>(c));
            string->append(1, ch);
            return 1;
        }
        return 0;
    }

public:
    /**
     * Constructs a new formatter with the specified output function.
     * The output stream of the formatted output is determined by the specified function.
     * @param putc the function to print a character.
     */
    Formatter(int (*putc)(int, void*), void* opt) throw();
    /**
     * Copy constructor.
     */
    Formatter(const Formatter& o) throw();
    /**
     * Constructs a new formatter with the specified output stream.
     * The formated strings are written to the specified stream.
     * @param stream the output stream.
     */
    Formatter(IStream* stream) throw();
    /**
     * Constructs a new formatter with the specified string.
     * The formated strings are written to the specified string.
     * @param string the string.
     */
    Formatter(std::string& string) throw();
    /**
     * Destructs this object.
     */
    ~Formatter();

    /**
     * Writes a formatted string to the output stream of this object
     * using the specified format string and arguments.
     * @param spec the format string.
     * @param args the arguments referenced by the format specifiers in the format string.
     */
    int format(const char* spec, va_list args)  __attribute__ ((format (printf, 2, 0)));

    /**
     * Writes a formatted string to the output stream of this object
     * using the specified format string and arguments.
     * @param spec the format string.
     * @param ... the arguments referenced by the format specifiers in the format string.
     */
    int format(const char* spec, ...) __attribute__ ((format (printf, 2, 3)))
    {
        va_list list;
        int count;

        va_start(list, spec);
        count = format(spec, list);
        va_end(list);
        return count;
    }

    int print(char c);
    int print(const char* string);
    int print(float x);
    int print(double x);

    int print(short n)
    {
        return print((long) n);
    }
    int print(unsigned short n)
    {
        return print((unsigned long) n);
    }

    int print(int n)
    {
        return print((long) n);
    }
    int print(unsigned int n)
    {
        return print((unsigned long) n);
    }

    int print(long n);
    int print(unsigned long n);
    int print(long long n);
    int print(unsigned long long n);
};

#endif  // NINTENDO_ES_FORMATTER_H_INCLUDED
