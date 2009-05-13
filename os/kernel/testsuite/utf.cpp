/*
 * Copyright 2008, 2009 Google Inc.
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

#include <stdio.h>
#include <es.h>
#include <es/utf.h>
#include "core.h"

#define TEST(exp)                           \
    (void) ((exp) ||                        \
            (esPanic(__FILE__, __LINE__, "\nFailed test " #exp), 0))

void utf16toutf8(const u16* utf16, char* utf8)
{
    do {
        u32 utf32;
        utf16 = utf16to32(utf16, &utf32);
        utf8 = utf32to8(utf32, utf8);
    } while (utf16 && *utf16);
    *utf8 = 0;
}

void utf8toutf16(const char* utf8, u16* utf16)
{
    do {
        u32 utf32;
        utf8 = utf8to32(utf8, &utf32);
        utf16 = utf32to16(utf32, utf16);
    } while (utf8 && *utf8);
    *utf16 = 0;
}

int main()
{
    Object* root = NULL;
    esInit(&root);

    wchar_t kanji[] = L"漢字";
    esDump(kanji, sizeof kanji);
    esReport("sizeof(wchar_t) = %u\n", sizeof(wchar_t));

    char utf[256];
    strcpy(utf, "漢字");
    esReport("%02x %02x %02x %02x %02x %02x %02x %02x %s\n",
             (u8) utf[0], (u8) utf[1], (u8) utf[2], (u8) utf[3],
             (u8) utf[4], (u8) utf[5], (u8) utf[6], (u8) utf[7],
             utf);

    u16 uni[256];
    utf8toutf16(utf, uni);
    // 0x6F22, 0x5B57
    esReport("%04x %04x\n", uni[0], uni[1]);
    TEST(uni[0] == 0x6F22);
    TEST(uni[1] == 0x5B57);

    char utf2[256];
    utf16toutf8(uni, utf2);
    esReport("%02x %02x %02x %02x %02x %02x %02x %02x %s\n",
             (u8) utf2[0], (u8) utf2[1], (u8) utf2[2], (u8) utf2[3],
             (u8) utf2[4], (u8) utf2[5], (u8) utf2[6], (u8) utf2[7],
             utf2);

    TEST(strcmp(utf, utf2) == 0);

    esReport("done.\n");
}
