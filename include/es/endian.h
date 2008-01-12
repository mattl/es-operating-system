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

#ifndef NINTENDO_ES_ENDIAN_H_INCLUDED
#define NINTENDO_ES_ENDIAN_H_INCLUDED

#include <es/types.h>

namespace LittleEndian
{
    inline u32 byte(u8* p)
    {
        return p[0];
    }

    inline u32 word(u8* p)
    {
        return p[0] | (p[1] << 8);
    }

    inline u32 dword(u8* p)
    {
        return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
    }

    inline void xbyte(u8* p, u32 v)
    {
        p[0] = (u8) v;
    }

    inline void xword(u8* p, u32 v)
    {
        p[0] = (u8) v;
        p[1] = (u8) (v >> 8);
    }

    inline void xdword(u8* p, u32 v)
    {
        p[0] = (u8) v;
        p[1] = (u8) (v >> 8);
        p[2] = (u8) (v >> 16);
        p[3] = (u8) (v >> 24);
    }
};

namespace BigEndian
{
    inline u32 byte(u8* p)
    {
        return p[0];
    }

    inline u32 word(u8* p)
    {
        return p[1] | (p[0] << 8);
    }

    inline u32 dword(u8* p)
    {
        return p[3] | (p[2] << 8) | (p[1] << 16) | (p[0] << 24);
    }

    inline void xbyte(u8* p, u32 v)
    {
        p[0] = (u8) v;
    }

    inline void xword(u8* p, u32 v)
    {
        p[0] = (u8) (v >> 8);
        p[1] = (u8) v;
    }

    inline void xdword(u8* p, u32 v)
    {
        p[0] = (u8) (v >> 24);
        p[1] = (u8) (v >> 16);
        p[2] = (u8) (v >> 8);
        p[3] = (u8) v;
    }
};

#endif  // #ifndef NINTENDO_ES_ENDIAN_H_INCLUDED
