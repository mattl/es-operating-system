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
/*
 * These coded instructions, statements, and computer programs contain
 * software derived from firefox 2.0.0.6.
 *
 * http://releases.mozilla.org/pub/mozilla.org/firefox/releases/2.0.0.6/source/firefox-2.0.0.6-source.tar.bz2
 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 *   Vladimir Vukicevic <vladimir@pobox.com>
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef NINTENDO_ES_LIBCANVAS_CANVAS_H_INCLUDED
#define NINTENDO_ES_LIBCANVAS_CANVAS_H_INCLUDED

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include <es/classFactory.h>
#include <es/interlocked.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/base/IClassStore.h>
#include <es/base/IInterfaceStore.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <es/util/ICanvasRenderingContext2D.h>
#include <cairo.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <cairo.h>

#ifdef __cplusplus
}
#endif

class Canvas : public ICanvasRenderingContext2D
{
    Ref ref;
    IMonitor* monitor;
    Interlocked updated;

    cairo_surface_t* surface;
    cairo_t* cr;

    int surfaceWidth;  // surface width
    int surfaceHeight; // surface height
    cairo_format_t surfaceFormat;

    int screenWidth;
    int screenHeight;

    enum
    {
        STYLE_STROKE = 0,
        STYLE_FILL,
        STYLE_SHADOW,
        STYLE_MAX
    };

    // state stack handling
    class ContextState
    {
    public:
        ContextState() : globalAlpha(1.0)
        {
        }

        ContextState(const ContextState* other)
            : globalAlpha(other->globalAlpha)
        {
            for (int i = 0; i < STYLE_MAX; i++)
            {
                colorStyles[i] = other->colorStyles[i];
            }
        }

        inline void setColorStyle(int whichStyle, u32 color)
        {
            colorStyles[whichStyle] = color;
        }

        float globalAlpha;
        u32 colorStyles[STYLE_MAX];
        Link<ContextState>   link;
    };

    typedef List<ContextState, &ContextState::link> StyleStack;
    StyleStack styleStack;

    // style handling
    u32 lastStyle;
    bool dirtyStyle[STYLE_MAX];

    void applyStyle(u32 aWhichStyle);
    void dirtyAllStyles();
    int getStyle(u32 aWhichStyle, char* color, unsigned int len);
    void setCairoColor(u32 color);
    void setStyle(u32 aWhichStyle, const char* color);

    inline ContextState* currentState()
    {
        return styleStack.getLast();
    }

    inline u32 NS_RGB(u8 r, u8 g, u8 b)
    {
        return ((255 << 24) | ((b)<<16) | ((g)<<8) | r);
    }

    inline u32 NS_RGB(u8 r, u8 g, u8 b, u8 a)
    {
        return (((a) << 24) | ((b)<<16) | ((g)<<8) | r);
    }

    inline u8 NS_GET_R(u32 rgba)
    {
        return (u8) (rgba & 0xff);
    }

    inline u8 NS_GET_G(u32 rgba)
    {
        return (u8) ((rgba >> 8) & 0xff);
    }

    inline u8 NS_GET_B(u32 rgba)
    {
        return (u8) ((rgba >> 16) & 0xff);
    }

    inline u8 NS_GET_A(u32 rgba)
    {
        return (u8) ((rgba >> 24) & 0xff);
    }

public:
    Canvas(cairo_surface_t* surface, int screenWidth, int screenHeight);
    ~Canvas();

    u8* getData();
    u32 parser(const char* color);

    //
    // ICanvasRenderingContext2D
    //
    void arc(float x, float y, float radius, float startAngle, float endAngle, bool anticlockwise);
    void arcTo(float x1, float y1, float x2, float y2, float radius);
    void beginPath();
    void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y);
    void clearRect(float x, float y, float width, float height);
    void closePath();
    void clip();
    void fill();
    void fillRect(float x, float y, float width, float height);
    int getFillStyle(char* color, unsigned int len);
    float getGlobalAlpha();
    float getMiterLimit();
    int getLineCap(char* capStyle, unsigned int len);
    int getLineJoin(char* joinStyle, unsigned int len);
    float getLineWidth();
    int getStrokeStyle(char* color, unsigned int len);
    void lineTo(float x, float y);
    void moveTo(float x, float y);
    void quadraticCurveTo(float cpx, float cpy, float x, float y);
    void rect(float x, float y, float width, float height);
    void restore();
    void rotate(float angle);
    void save();
    void scale(float scaleW, float scaleH);
    void setFillStyle(const char* color);
    void setGlobalAlpha(float alpha);
    void setMiterLimit(float limit);
    void setLineWidth(float width);
    void setLineCap(const char* capStyle);
    void setLineJoin(const char* joinStyle);
    void setStrokeStyle(const char* color);
    void stroke();
    void strokeRect(float x, float y, float width, float height);
    void translate(float tx, float ty);

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_ICanvasRenderingContext2D)
        {
            *objectPtr = static_cast<ICanvasRenderingContext2D*>(this);
        }
        else if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<ICanvasRenderingContext2D*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
    }

    unsigned int addRef(void)
    {
        int count = ref.addRef();
        return count;
    }

    unsigned int release(void)
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};

#endif // NINTENDO_ES_LIBCANVAS_CANVAS_H_INCLUDED
