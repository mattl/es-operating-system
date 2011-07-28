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

#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <es/color.h>

namespace
{
    struct Keyword
    {
        const char* name;
        u32         rgba;
    };

    Keyword keywords[] =
    {
        { "aqua",        0xffffff00 },
        { "black",       0xff000000 },
        { "blue",        0xffff0000 },
        { "fuchsia",     0xffff00ff },
        { "gray",        0xff808080 },
        { "green",       0xff008000 },
        { "lime",        0xff00ff00 },
        { "maroon",      0xff000080 },
        { "navy",        0xff800000 },
        { "olive",       0xff008080 },
        { "purple",      0xff800080 },
        { "red",         0xff0000ff },
        { "silver",      0xffc0c0c0 },
        { "teal",        0xff808000 },
        { "transparent", 0x00000000 },
        { "white",       0xffffffff },
        { "yellow",      0xff00ffff }
    };

    const size_t MaxKeywords = (sizeof keywords) / (sizeof keywords[0]);
    const size_t MaxKeywordLen = 12;
}

bool Rgb::assign(const char* color)
{
    if (*color == '#')
    {
        unsigned int r, g, b;

        if (sscanf(color, "#%2x%2x%2x", &r, &g, &b) == 3)
        {
            rgba = 0xff000000 | (b << 16) | (g << 8) | r;
            return true;
        }
        else if (sscanf(color, "#%1x%1x%1x", &r, &g, &b) == 3)
        {
            r |= (r << 4);
            g |= (g << 4);
            b |= (b << 4);
            rgba = 0xff000000 | (b << 16) | (g << 8) | r;
            return true;
       }
    }
    else if (strncasecmp(color, "rgb(", 4) == 0)
    {
        int r, g, b;
        float s, t, u;

        if (sscanf(color + 3, "(%d,%d,%d)", &r, &g, &b) == 3)
        {
            r = std::max(0, std::min(255, r));
            g = std::max(0, std::min(255, g));
            b = std::max(0, std::min(255, b));
            rgba = 0xff000000 | (b << 16) | (g << 8) | r;
            return true;
        }
        else if (sscanf(color + 3, "(%f%%,%f%%,%f%%)", &s, &t, &u) == 3)
        {
            s = std::max(0.0f, std::min(100.0f, s));
            t = std::max(0.0f, std::min(100.0f, t));
            u = std::max(0.0f, std::min(100.0f, u));
            r = static_cast<int>(255.0f * (s / 100.0f));
            g = static_cast<int>(255.0f * (t / 100.0f));
            b = static_cast<int>(255.0f * (u / 100.0f));
            rgba = 0xff000000 | (b << 16) | (g << 8) | r;
            return true;
        }
    }
    else if (strncasecmp(color, "rgba(", 5) == 0)
    {
        int r, g, b, a;
        float s, t, u, v;

        if (sscanf(color + 4, "(%d,%d,%d,%f)", &r, &g, &b, &v) == 4)
        {
            r = static_cast<int>(std::max(0, std::min(255, r)));
            g = static_cast<int>(std::max(0, std::min(255, g)));
            b = static_cast<int>(std::max(0, std::min(255, b)));
            v = std::max(0.0f, std::min(1.0f, v));
            a = static_cast<int>(255.0f * v);
            rgba = (a << 24) | (b << 16) | (g << 8) | r;
            return true;
        }
        else if (sscanf(color + 4, "(%f%%,%f%%,%f%%,%f)", &s, &t, &u, &v) == 4)
        {
            s = std::max(0.0f, std::min(100.0f, s));
            t = std::max(0.0f, std::min(100.0f, t));
            u = std::max(0.0f, std::min(100.0f, u));
            v = std::max(0.0f, std::min(1.0f, v));
            r = static_cast<int>(255.0f * (s / 100.0f));
            g = static_cast<int>(255.0f * (t / 100.0f));
            b = static_cast<int>(255.0f * (u / 100.0f));
            a = static_cast<int>(255.0f * v);
            rgba = (a << 24) | (b << 16) | (g << 8) | r;
            return true;
        }
    }
    else
    {
        int low = 0;
        int high = MaxKeywords - 1;
        while (low <= high)
        {
            int mid = static_cast<unsigned>(low + high) >> 1;

            int cond = strncasecmp(color, keywords[mid].name, MaxKeywordLen);
            if (cond < 0)
            {
                high = mid - 1;
            }
            else if (cond == 0)
            {
                rgba = keywords[mid].rgba;
                return true;
            }
            else if (0 < cond)
            {
                low = mid + 1;
            }
        }
    }
    return false;
}
