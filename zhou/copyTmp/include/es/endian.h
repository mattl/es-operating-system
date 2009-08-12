/*
 * Copyright 2008 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

static __inline u16 bswap16(u16 x)
{
    return (x >> 8) | (x << 8);
}

#if defined(__i386__)

static __inline u32 bswap32(u32 x)
{
    __asm__("bswapl %0" : "=r" (x) : "0" (x));
    return x;
}

static __inline u64 bswap64(u64 x)
{
    union
    {
        struct
        {
            u32 a;
            u32 b;
        } s;
        u64 u;
    } v;
    v.u = x;
    asm("bswapl %0 ; bswapl %1 ; xchgl %0,%1"
        : "=r" (v.s.a), "=r" (v.s.b)
        : "0" (v.s.a), "1" (v.s.b));
    return v.u;
}

#elif defined(__x86_64__)

static __inline u32 bswap32(u32 x)
{
    __asm__("bswapl %0" : "=r" (x) : "0" (x));
    return x;
}

static __inline u64 bswap64(u64 x)
{
    __asm__("bswapq %0" : "=r" (x) : "0" (x));
    return x;
}

#else   // !defined(__x86_64__)

static __inline u32 bswap32(u32 x)
{
    return (x >> 24) & 0x000000ff |
           (x >> 8)  & 0x0000ff00 |
           (x << 8)  & 0x00ff0000 |
           (x << 24) & 0xff000000;
}

static __inline u64 bswap64(u64 x)
{
    return (x >> 48) & 0x00000000000000ffLL |
           (x >> 32) & 0x000000000000ff00LL |
           (x << 24) & 0x0000000000ff0000LL |
           (x << 8)  & 0x00000000ff000000LL |
           (x >> 8)  & 0x000000ff00000000LL |
           (x >> 24) & 0x0000ff0000000000LL |
           (x << 32) & 0x00ff000000000000LL |
           (x << 48) & 0xff00000000000000LL;
}

#endif  // !defined(__x86_64__)

#if defined(__es__) || defined(__linux__)

static __inline u64 htonll(u64 hostlonglong)
{
    return bswap64(hostlonglong);
}

static __inline u32 htonl(u32 hostlong)
{
    return bswap32(hostlong);
}

static __inline u16 htons(u16 hostshort)
{
    return bswap16(hostshort);
}

static __inline u64 ntohll(u64 netlonglong)
{
    return bswap64(netlonglong);
}

static __inline u32 ntohl(u32 netlong)
{
    return bswap32(netlong);
}

static __inline u16 ntohs(u16 netshort)
{
    return bswap16(netshort);
}

#else   // !__es__

extern u64 (htonll)(u64 hostlonglong);
extern u32 (htonl)(u32 hostlong);
extern u16 (htons)(u16 hostshort);
extern u64 (ntohll)(u64 netlonglong);
extern u32 (ntohl)(u32 netlong);
extern u16 (ntohs)(u16 netshort);

#endif  // __es__

#endif  // #ifndef NINTENDO_ES_ENDIAN_H_INCLUDED
