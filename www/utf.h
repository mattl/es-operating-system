/*
 * Copyright 2010, 2011 Esrille Inc.
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

#ifndef ES_UTF_H_INCLUDED
#define ES_UTF_H_INCLUDED

#include <cstring>
#include <string>
#include <ostream>

char* utf8to32(const char* utf8, char32_t* utf32);
char* utf32to8(char32_t utf32, char* utf8);
size_t utf32to8len(char32_t utf32);
char16_t* utf16to32(const char16_t* utf16, char32_t* utf32);
char16_t* utf32to16(char32_t utf32, char16_t* utf16);
char32_t utftolower(char32_t utf32);
char32_t utftoupper(char32_t utf32);
int utf16cmp(const char16_t* a, const char16_t* b);
int utf16icmp(const char16_t* a, const char16_t* b);
int utf16ncmp(const char16_t* a, const char16_t* b, size_t len);
int utf16nicmp(const char16_t* a, const char16_t* b, size_t len);
char16_t* utf16cpy(char16_t* a, const char16_t* b);
char16_t* utf16ncpy(char16_t* a, const char16_t* b, size_t len);
size_t utf16len(const char16_t* s);

char* utf16cpy8(char* a, const char16_t* b);
char* utf16ncpy8(char* a, const char16_t* b, size_t len);

std::string utfconv(const std::u16string& s);
std::u16string utfconv(const std::string& s);

void hexdump(const void* ptr, size_t len);

inline int isAlnum(int c)
{
    if ('0' <= c && c <= '9')
        return '0';
    if ('A' <= c && c <= 'Z')
        return 'A';
    if ('a' <= c && c <= 'z')
        return 'a';
    return 0;
}

inline int isAlpha(int c)
{
    if ('A' <= c && c <= 'Z')
        return 'A';
    if ('a' <= c && c <= 'z')
        return 'a';
    return 0;
}

inline int isControl(int c)
{
    return (0x00 <= c && c <= 0x1f);
}

inline int isDigit(int c)
{
    return ('0' <= c && c <= '9') ? '0' : 0;
}

inline int isHexDigit(int c)
{
    if ('0' <= c && c <= '9')
        return '0';
    if ('A' <= c && c <= 'F')
        return 'A' - 10;
    if ('a' <= c && c <= 'f')
        return 'a' - 10;
    return 0;
}

inline int isSpace(int c)
{
    switch (c) {
    case '\t':
    case '\n':
    case '\f':
    case '\r':
    case ' ':
        return true;
    default:
        return false;
    }
}

inline int isUpper(int c)
{
    if ('A' <= c && c <= 'Z')
        return 'A';
    return 0;
}

inline char16_t toLower(char16_t c)
{
    if ('A' <= c && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}

inline void toLower(std::u16string& s)
{
    for (auto i = s.begin(); i != s.end(); ++i)
        *i = toLower(*i);
}

inline void stripLeadingAndTrailingWhitespace(std::u16string& s)
{
    size_t pos = 0;
    while (isSpace(s[pos]))
        ++pos;
    if (0 < pos)
        s.erase(0, pos);
    if (0 < s.length()) {
        pos = s.length() - 1;
        if (isSpace(s[pos])) {
            while (0 < pos) {
                --pos;
                if (!isSpace(s[pos])) {
                    ++pos;
                    break;
                }
            }
            s.erase(pos);
        }
    }
}

inline std::ostream& operator<<(std::ostream& stream, const std::u16string& string)
{
    const char16_t* s = string.c_str();
    while (s && *s) {
        char32_t utf32;
        s = utf16to32(s, &utf32);
        if (s) {
            char utf8[5];
            if (char* end = utf32to8(utf32, utf8)) {
                *end = '\0';
                stream << utf8;
            }
        }
    }
    return stream;
}

inline std::u16string espaceString(const std::u16string string)
{
    std::u16string s;
    for (std::u16string::const_iterator i = string.begin(); i < string.end(); ++i) {
        switch (*i) {
        case '\\':
            s += u"\\\\";
            break;
        case '"':
            s += u"\\\"";
            break;
        case '/':
            s += u"\\/";
            break;
        case '\t':
            s += u"\\t";
            break;
        case '\n':
            s += u"\\n";
            break;
        case '\f':
            s += u"\\f";
            break;
        default:
            s += *i;
            break;
        }
    }
    return s;
}

#endif  // #ifndef ES_UTF_H_INCLUDED
