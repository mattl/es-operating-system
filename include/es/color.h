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

/*
 * cf. CSS3 Color Module
 * http://www.w3.org/TR/css3-color/
 */

#ifndef NINTENDO_ES_COLOR_H_INCLUDED
#define NINTENDO_ES_COLOR_H_INCLUDED

#include <errno.h>
#include <es/types.h>
#include <es/exception.h>

class Rgb
{
    u32 rgba;

    bool assign(const char* color);

public:
    Rgb(u32 rgba = 0xff000000) :
        rgba(rgba)
    {
    }

    Rgb(const char* color) :
        rgba(0xff000000)
    {
        if (!assign(color))
        {
            throw SystemException<EINVAL>();
        }
    }

    Rgb(u8 r, u8 g, u8 b, u8 a = 255u)
    {
        rgba = (a << 24) | (b << 16) | (g << 8) | r;
    }

    u8 getR()
    {
        return (u8) (rgba & 0xff);
    }

    u8 getG() const
    {
        return (u8) ((rgba >> 8) & 0xff);
    }

    u8 getB() const
    {
        return (u8) ((rgba >> 16) & 0xff);
    }

    u8 getA() const
    {
        return (u8) ((rgba >> 24) & 0xff);
    }

    Rgb& operator=(const char* color)
    {
        assign(color);
        return *this;
    }

    operator u32() const
    {
        return rgba;
    }
};

#endif // NINTENDO_ES_COLOR_H_INCLUDED
