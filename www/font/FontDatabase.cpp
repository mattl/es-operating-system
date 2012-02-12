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

#include "css/CSSStyleDeclarationImp.h"

// TODO: support dynamic font configuration later...

namespace org { namespace w3c { namespace dom { namespace bootstrap {

struct FontInfo
{
    static const int MaxInfo = 4;

    const char16_t* family;
    unsigned generic;
    FontFileInfo fileInfo[MaxInfo];

    FontFileInfo* chooseFontFileInfo(unsigned style, unsigned weight) {
        FontFileInfo* chosen = fileInfo;
        unsigned score = 0;
        for (FontFileInfo* i = fileInfo; i < &fileInfo[MaxInfo]; ++i) {
            if (i->filename == 0)
                break;
            unsigned newScore = 0;
            if (i->style == style)
                newScore += 200;
            else if (style == CSSFontStyleValueImp::Italic && i->style == CSSFontStyleValueImp::Oblique)
                newScore += 100;
            if (i->weight == weight)
                newScore += 10;
            else if (weight <= 400) {
                if (weight == 400 && i->weight == 500)
                    newScore += 9;
                else if (i->weight < weight)
                    newScore += 9 - (weight - i->weight) / 100;
                else
                    newScore += (1000 - i->weight) / 100;
            } else if (500 <= weight) {
                if (weight == 500 && i->weight == 400)
                    newScore += 9;
                else if (weight < i->weight)
                    newScore += 9 - (i->weight - weight) / 100;
                else
                    newScore += i->weight / 100;
            }
            if (score < newScore) {
                chosen = i;
                score = newScore;
            }
        }
        return chosen;
    }
};

namespace {

FontInfo fontDatabase[] = {
    {
        u"LiberationSans",
        CSSFontFamilyValueImp::SansSerif,
        {
            { LIBERATON_TTF "/LiberationSans-BoldItalic.ttf", 700, CSSFontStyleValueImp::Italic },
            { LIBERATON_TTF "/LiberationSans-Italic.ttf", 400, CSSFontStyleValueImp::Italic },
            { LIBERATON_TTF "/LiberationSans-Regular.ttf", 400 },
            { LIBERATON_TTF "/LiberationSans-Bold.ttf", 700 },
        }
    },
    {
        u"LiberationSerif",
        CSSFontFamilyValueImp::Serif,
        {
            { LIBERATON_TTF "/LiberationSerif-BoldItalic.ttf", 700, CSSFontStyleValueImp::Italic },
            { LIBERATON_TTF "/LiberationSerif-Italic.ttf", 400, CSSFontStyleValueImp::Italic },
            { LIBERATON_TTF "/LiberationSerif-Regular.ttf", 400 },
            { LIBERATON_TTF "/LiberationSerif-Bold.ttf", 700 },
        }
    },
    {
        u"LiberationMono",
        CSSFontFamilyValueImp::Monospace,
        {
            { LIBERATON_TTF "/LiberationMono-BoldItalic.ttf", 700, CSSFontStyleValueImp::Italic },
            { LIBERATON_TTF "/LiberationMono-Italic.ttf", 400, CSSFontStyleValueImp::Italic },
            { LIBERATON_TTF "/LiberationMono-Regular.ttf", 400 },
            { LIBERATON_TTF "/LiberationMono-Bold.ttf", 700 },
        }
    },
#ifdef HAVE_IPA_PGOTHIC
    {
        u"IPAPGothic",
        CSSFontFamilyValueImp::SansSerif,
        {
            { HAVE_IPA_PGOTHIC, 400 }
        }
    },
#endif
#ifdef HAVE_IPA_PMINCHO
    {
        u"IPAPMincho",
        CSSFontFamilyValueImp::Serif,
        {
            { HAVE_IPA_PMINCHO, 400 }
        }
    },
#endif
#ifdef HAVE_IPA_GOTHIC
    {
        u"IPAGothic",
        CSSFontFamilyValueImp::Monospace,
        {
            { HAVE_IPA_GOTHIC, 400 }
        }
    },
#endif
#ifdef HAVE_IPA_MINCHO
    {
        u"IPAMincho",
        CSSFontFamilyValueImp::Monospace,
        {
            { HAVE_IPA_MINCHO, 400 }
        }
    },
#endif
#ifdef HAVE_AEGEAN
    {
        u"Aegean",
        CSSFontFamilyValueImp::SansSerif,
        {
            { HAVE_AEGEAN, 400 },
        }
    },
#endif
#ifdef HAVE_AHEM
    {
        u"Ahem",
        CSSFontFamilyValueImp::Monospace,
        {
            { HAVE_AHEM, 400 }
        }
    },
#endif
    // Test fonts for CSS 2.1 test suite
    {
        u"Ahem!",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_Ahem!.TTF", 400 }
        }
    },
    {
        u"MissingNormal",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_MissingNormal.TTF", 400 },
        }
    },
    {
        u"SmallCaps",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_SmallCaps.TTF", 400 },
        }
    },
    {
        u"MissingItalicOblique",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_MissingItalicOblique.TTF", 400 },
        }
    },
    {
        u"White Space",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_WhiteSpace.TTF", 400 },
        }
    },
    {
        u"cursive",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_cursive.TTF", 400 }
        }
    },
    {
        u"default",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_default.TTF", 400 }
        }
    },
    {
        u"fantasy",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_fantasy.TTF", 400 }
        }
    },
    {
        u"inherit",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_inherit.TTF", 400 }
        }
    },
    {
        u"initial",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_initial.TTF", 400 }
        }
    },
    {
        u"monospace",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_monospace.TTF", 400 }
        }
    },
    {
        u"serif",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_serif.TTF", 400 }
        }
    },
    {
        u"sans-serif",
        CSSFontFamilyValueImp::Monospace,
        {
            { TEST_FONTS "/AhemExtra/AHEM_sans-serif.TTF", 400 }
        }
    },
    {
        u"CSSTest ASCII",
        CSSFontFamilyValueImp::Serif,
        {
            { TEST_FONTS "/CSSTest/csstest-ascii.ttf", 400 },
        }
    },
    {
        u"CSSTest Basic",
        CSSFontFamilyValueImp::Serif,
        {
            { TEST_FONTS "/CSSTest/csstest-basic-bold.ttf", 700 },
            { TEST_FONTS "/CSSTest/csstest-basic-bolditalic.ttf", 700, CSSFontStyleValueImp::Italic },
            { TEST_FONTS "/CSSTest/csstest-basic-italic.ttf", 400, CSSFontStyleValueImp::Italic },
            { TEST_FONTS "/CSSTest/csstest-basic-regular.ttf", 400 },
        }
    },
    {
        u"CSSTest Fallback",
        CSSFontFamilyValueImp::Serif,
        {
            { TEST_FONTS "/CSSTest/csstest-fallback.ttf", 400 },
        }
    },
    {
        u"small-caps 1in CSSTest FamilyName Funky",
        CSSFontFamilyValueImp::Serif,
        {
            { TEST_FONTS "/CSSTest/csstest-familyname-funkyA.ttf", 400 },
        }
    },
    {
        u"x-large CSSTest FamilyName Funky",
        CSSFontFamilyValueImp::Serif,
        {
            { TEST_FONTS "/CSSTest/csstest-familyname-funkyB.ttf", 400 },
        }
    },
    {
        u"12px CSSTest FamilyName Funky",
        CSSFontFamilyValueImp::Serif,
        {
            { TEST_FONTS "/CSSTest/csstest-familyname-funkyC.ttf", 400 },
        }
    },
    {
        u"CSSTest FamilyName",
        CSSFontFamilyValueImp::Serif,
        {
            { TEST_FONTS "/CSSTest/csstest-familyname.ttf", 400 },
            { TEST_FONTS "/CSSTest/csstest-familyname-bold.ttf", 700 },
        }
    },
    {
        u"CSSTest Verify",
        CSSFontFamilyValueImp::Serif,
        {
            { TEST_FONTS "/CSSTest/csstest-verify.ttf", 400 },
        }
    },
};

int fontDatabaseSize = 0;

FontInfo* chooseFontInfo(const std::u16string& family)
{
    for (FontInfo* i = fontDatabase; i < &fontDatabase[fontDatabaseSize]; ++i) {
        if (!compareIgnoreCase(family, i->family))
            return i;
    }
    return 0;
}

FontInfo* chooseFontInfo(unsigned generic)
{
    // TODO: remove this switch statement once more fonts are supported
    switch (generic) {
    case CSSFontFamilyValueImp::Cursive:
        generic = CSSFontFamilyValueImp::Serif;
        break;
    case CSSFontFamilyValueImp::Fantasy:
        generic = CSSFontFamilyValueImp::SansSerif;
        break;
    default:
        break;
    }
    for (FontInfo* i = fontDatabase; i < &fontDatabase[fontDatabaseSize]; ++i) {
        if (generic == i->generic)
            return i;
    }
    return fontDatabase;
}

}  // namespace

void FontFileInfo::disableTestFonts()
{
    enableTestFonts();
    FontInfo* info = chooseFontInfo(u"Ahem!");
    fontDatabaseSize = info - fontDatabase;
}

void FontFileInfo::enableTestFonts()
{
    fontDatabaseSize = sizeof fontDatabase / sizeof fontDatabase[0];
}

FontFileInfo* FontFileInfo::chooseFont(const CSSStyleDeclarationImp* style)
{
    if (fontDatabaseSize == 0)
        disableTestFonts();

    FontInfo* fontInfo = 0;
    for (auto i = style->fontFamily.getFamilyNames().begin();
         i != style->fontFamily.getFamilyNames().end();
         ++i)
    {
        if (fontInfo = chooseFontInfo(*i))
            break;
    }
    if (!fontInfo)
        fontInfo = chooseFontInfo(style->fontFamily.getGeneric());

    return fontInfo->chooseFontFileInfo(style->fontStyle.getStyle(), style->fontWeight.getWeight());
}

FontFileInfo* FontFileInfo::chooseAltFont(const CSSStyleDeclarationImp* style, FontTexture* current, char32_t u)
{
    if (fontDatabaseSize == 0)
        disableTestFonts();

    assert(current);
    FontManager* manager = current->getFace()->getManager();

    FontInfo* fontInfo = 0;
    bool skipped = false;
    for (auto i = style->fontFamily.getFamilyNames().begin();
         i != style->fontFamily.getFamilyNames().end();
         ++i)
    {
        if (fontInfo = chooseFontInfo(*i)) {
            // TODO: Deal with filesnames registered more than once.
            FontFileInfo* fontFileInfo = fontInfo->chooseFontFileInfo(style->fontStyle.getStyle(), style->fontWeight.getWeight());
            if (fontFileInfo->filename == current->getFace()->getFilename()) {
                skipped = true;
                continue;
            }
            if (skipped)
                break;
        }
    }
    if (fontInfo) {
        FontFileInfo* fontFileInfo = fontInfo->chooseFontFileInfo(style->fontStyle.getStyle(), style->fontWeight.getWeight());
        FontFace* face = manager->getFontFace(fontFileInfo->filename);
        if (face && face->hasGlyph(u))
            return fontFileInfo;
    }

    // Now we can choose any font that has the glyph for unicode 'u'.
    // TODO: Make this faster.
    unsigned generic = style->fontFamily.getGeneric();
    for (;;) {
        for (fontInfo = fontDatabase; fontInfo < &fontDatabase[fontDatabaseSize]; ++fontInfo) {
            FontFileInfo* fontFileInfo = fontInfo->chooseFontFileInfo(style->fontStyle.getStyle(), style->fontWeight.getWeight());
            if (fontFileInfo->filename == current->getFace()->getFilename())
                continue;
            if (generic) {
                if (fontInfo->generic != generic)
                    continue;
            } else if (fontInfo->generic == style->fontFamily.getGeneric())
                continue;
            FontFace* face = manager->getFontFace(fontFileInfo->filename);
            if (!face)
                continue;
            if (face->hasGlyph(u))
                return fontFileInfo;
        }
        if (!generic)
            break;
        generic = CSSFontFamilyValueImp::None;
    }
    return 0;
}

}}}}  // org::w3c::dom::bootstrap
