/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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
