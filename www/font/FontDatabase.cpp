/*
 * Copyright 2010-2012 Esrille Inc.
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

#include "FontDatabase.h"
#include "FontManager.h"

namespace {

const char* fontList[] =
{
    // LiberationSans
    LIBERATON_TTF "/LiberationSans-BoldItalic.ttf",
    LIBERATON_TTF "/LiberationSans-Italic.ttf",
    LIBERATON_TTF "/LiberationSans-Regular.ttf",
    LIBERATON_TTF "/LiberationSans-Bold.ttf",
    // LiberationSerif
    LIBERATON_TTF "/LiberationSerif-BoldItalic.ttf",
    LIBERATON_TTF "/LiberationSerif-Italic.ttf",
    LIBERATON_TTF "/LiberationSerif-Regular.ttf",
    LIBERATON_TTF "/LiberationSerif-Bold.ttf",
    // LiberationMono
    LIBERATON_TTF "/LiberationMono-BoldItalic.ttf",
    LIBERATON_TTF "/LiberationMono-Italic.ttf",
    LIBERATON_TTF "/LiberationMono-Regular.ttf",
    LIBERATON_TTF "/LiberationMono-Bold.ttf",
#ifdef HAVE_IPA_PGOTHIC
    // IPAPGothic
    HAVE_IPA_PGOTHIC,
#endif
#ifdef HAVE_IPA_PMINCHO
    // IPAPMincho
    HAVE_IPA_PMINCHO,
#endif
#ifdef HAVE_IPA_GOTHIC
    // IPAGothic
    HAVE_IPA_GOTHIC,
#endif
#ifdef HAVE_IPA_MINCHO
    // IPAMincho
    HAVE_IPA_MINCHO,
#endif
#ifdef HAVE_AEGEAN
    // Aegean
    HAVE_AEGEAN,
#endif
#ifdef HAVE_AHEM
    // Ahem
    HAVE_AHEM,
#endif
};

// Test fonts for CSS 2.1 test suite
const char* testFontList[] =
{
    // Ahem!
    TEST_FONTS "/AhemExtra/AHEM_Ahem!.TTF",
    // MissingNormal
    TEST_FONTS "/AhemExtra/AHEM_MissingNormal.TTF",
    // SmallCaps
    TEST_FONTS "/AhemExtra/AHEM_SmallCaps.TTF",
    // MissingItalicOblique
    TEST_FONTS "/AhemExtra/AHEM_MissingItalicOblique.TTF",
    // White Space
    TEST_FONTS "/AhemExtra/AHEM_WhiteSpace.TTF",
    // cursive
    TEST_FONTS "/AhemExtra/AHEM_cursive.TTF",
    // default
    TEST_FONTS "/AhemExtra/AHEM_default.TTF",
    // fantasy
    TEST_FONTS "/AhemExtra/AHEM_fantasy.TTF",
    // inherit
    TEST_FONTS "/AhemExtra/AHEM_inherit.TTF",
    // initial
    TEST_FONTS "/AhemExtra/AHEM_initial.TTF",
    // monospace
    TEST_FONTS "/AhemExtra/AHEM_monospace.TTF",
    // serif
    TEST_FONTS "/AhemExtra/AHEM_serif.TTF",
    // sans-serif
    TEST_FONTS "/AhemExtra/AHEM_sans-serif.TTF",
    // CSSTest ASCII
    TEST_FONTS "/CSSTest/csstest-ascii.ttf",
    // CSSTest Basic
    TEST_FONTS "/CSSTest/csstest-basic-bold.ttf",
    TEST_FONTS "/CSSTest/csstest-basic-bolditalic.ttf",
    TEST_FONTS "/CSSTest/csstest-basic-italic.ttf",
    TEST_FONTS "/CSSTest/csstest-basic-regular.ttf",
    // CSSTest Fallback
    TEST_FONTS "/CSSTest/csstest-fallback.ttf",
    // small-caps 1in CSSTest FamilyName Funky
    TEST_FONTS "/CSSTest/csstest-familyname-funkyA.ttf",
    // x-large CSSTest FamilyName Funky
    TEST_FONTS "/CSSTest/csstest-familyname-funkyB.ttf",
    // 12px CSSTest FamilyName Funky
    TEST_FONTS "/CSSTest/csstest-familyname-funkyC.ttf",
    // CSSTest FamilyName
    TEST_FONTS "/CSSTest/csstest-familyname.ttf",
    TEST_FONTS "/CSSTest/csstest-familyname-bold.ttf",
    // CSSTest Verify
    TEST_FONTS "/CSSTest/csstest-verify.ttf",
};

}

void FontDatabase::loadBaseFonts(FontManager* manager)
{
    for (auto i = fontList; i < &fontList[sizeof fontList / sizeof fontList[0]]; ++i)
        manager->loadFont(*i);
}

void FontDatabase::loadTestFonts(FontManager* manager)
{
    for (auto i = testFontList; i < &testFontList[sizeof testFontList / sizeof testFontList[0]]; ++i)
        manager->loadFont(*i);
}
