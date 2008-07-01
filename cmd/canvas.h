/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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
#include <es/color.h>
#include <es/classFactory.h>
#include <es/handle.h>
#include <es/interlocked.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/base/IClassStore.h>
#include <es/base/IFile.h>
#include <es/base/IInterfaceStore.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <es/util/ICanvasRenderingContext2D.h>

using namespace es;

#ifdef __cplusplus
extern "C" {
#endif

#include <cairo.h>

#ifdef __cplusplus
}
#endif

ICurrentProcess* System();

class CanvasPattern : public ICanvasPattern
{
    Ref ref;
    IMonitor* monitor;
    cairo_pattern_t* pattern;
    u8* imageData;

public:
    CanvasPattern(cairo_pattern_t* pattern, u8* imageData) :
        pattern(pattern), imageData(imageData)
    {
        monitor = System()->createMonitor();
    }

    ~CanvasPattern()
    {
        if (pattern)
        {
            cairo_pattern_destroy(pattern);
        }
        if (imageData)
        {
            delete [] imageData;
        }
        if (monitor)
        {
            monitor->release();
        }
    }

    void apply(const void* context)
    {
        Synchronized<IMonitor*> method(monitor);

        ASSERT(context);
        cairo_t* cr = static_cast<cairo_t*>(const_cast<void*>(context)); // [check]
        cairo_set_source(cr, pattern);
    }

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == ICanvasPattern::iid())
        {
            objectPtr = static_cast<ICanvasPattern*>(this);
        }
        else if (riid == IInterface::iid())
        {
            objectPtr = static_cast<ICanvasPattern*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
        return objectPtr;
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

class CanvasGradient : public ICanvasGradient
{
    Ref ref;
    IMonitor* monitor;
    cairo_pattern_t* pattern;

public:
    CanvasGradient(cairo_pattern_t* gradpat) : pattern(gradpat)
    {
        monitor = System()->createMonitor();
    }

    ~CanvasGradient()
    {
        if (pattern)
        {
            cairo_pattern_destroy(pattern);
        }

        if (monitor)
        {
            monitor->release();
        }
    }

    void apply(const void* context)
    {
        Synchronized<IMonitor*> method(monitor);

        ASSERT(context);
        cairo_t* cr = static_cast<cairo_t*>(const_cast<void*>(context)); // [check]
        cairo_set_source(cr, pattern);
    }

    void addColorStop(float offset, const char* color);

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == ICanvasGradient::iid())
        {
            objectPtr = static_cast<ICanvasGradient*>(this);
        }
        else if (riid == IInterface::iid())
        {
            objectPtr = static_cast<ICanvasGradient*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
        return objectPtr;
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
            for (int i = 0; i < STYLE_MAX; i++)
            {
                colorStyles[i] = 0xff000000;
                gradientStyles[i] = NULL;
                patternStyles[i] = NULL;
            }
        }

        ContextState(const ContextState* other)
            : globalAlpha(other->globalAlpha)
        {
            for (int i = 0; i < STYLE_MAX; i++)
            {
                colorStyles[i] = other->colorStyles[i];
                gradientStyles[i] = other->gradientStyles[i];
                patternStyles[i] = other->patternStyles[i];
            }
        }

        inline void setColorStyle(int whichStyle, u32 color)
        {
            colorStyles[whichStyle] = color;
            gradientStyles[whichStyle] = NULL;
            patternStyles[whichStyle] = NULL;
        }

        inline void setPatternStyle(int whichStyle, ICanvasPattern* pat)
        {
            gradientStyles[whichStyle] = NULL;
            patternStyles[whichStyle] = pat;
        }

        inline void setGradientStyle(int whichStyle, ICanvasGradient* grad)
        {
            gradientStyles[whichStyle] = grad;
            patternStyles[whichStyle] = NULL;
        }

        float globalAlpha;
        u32 colorStyles[STYLE_MAX];
        Link<ContextState>   link;
        ICanvasGradient* gradientStyles[STYLE_MAX];
        ICanvasPattern* patternStyles[STYLE_MAX];

    };

    typedef List<ContextState, &ContextState::link> StyleStack;
    StyleStack styleStack;

    std::string textStyle;

    // style handling
    u32 lastStyle;
    bool dirtyStyle[STYLE_MAX];

    u8* allocateBitmapData(IFile* image, u32* imageWidth, u32* imageHeight);
    void applyStyle(u32 aWhichStyle);
    void dirtyAllStyles();
    int getStyle(u32 aWhichStyle, char* color, unsigned int len);
    void setCairoColor(Rgb color);
    void setStyle(u32 aWhichStyle, const char* color);
    void setStyle(u32 aWhichStyle, ICanvasGradient* grad);
    void setStyle(u32 aWhichStyle, ICanvasPattern* pat);

    inline ContextState* currentState()
    {
        return styleStack.getLast();
    }

public:
    Canvas(cairo_surface_t* surface, int screenWidth, int screenHeight);
    ~Canvas();

    u8* getData();

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
    ICanvasGradient* createLinearGradient(float x0, float y0, float x1, float y1);
    ICanvasGradient* createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1);
    ICanvasPattern* createPattern(IFile* image, const char* repeat);
    void drawImage(IFile* image, float dx, float dy, float dw, float dh);
    void fill();
    void fillRect(float x, float y, float width, float height);
    ICanvasGradient* getFillGradient();
    ICanvasPattern* getFillPattern();
    int getFillStyle(char* color, int len);
    float getGlobalAlpha();
    int getGlobalCompositeOperation(char* operation, int len);
    float getMiterLimit();
    int getLineCap(char* capStyle, int len);
    int getLineJoin(char* joinStyle, int len);
    float getLineWidth();
    int getStrokeStyle(char* color, int len);
    void lineTo(float x, float y);
    void moveTo(float x, float y);
    void quadraticCurveTo(float cpx, float cpy, float x, float y);
    void rect(float x, float y, float width, float height);
    void restore();
    void rotate(float angle);
    void save();
    void scale(float scaleW, float scaleH);
    int setFillStyle(const char* color);
    void setFillGradient(ICanvasGradient* gradient);
    void setFillPattern(ICanvasPattern* pattern);
    void setGlobalAlpha(float alpha);
    int setGlobalCompositeOperation(const char* operation);
    void setMiterLimit(float limit);
    void setLineWidth(float width);
    int setLineCap(const char* capStyle);
    int setLineJoin(const char* joinStyle);
    int setStrokeStyle(const char* color);
    void stroke();
    void strokeRect(float x, float y, float width, float height);
    void translate(float tx, float ty);

    // unsupported.
    void setShadowBlur(float blur)
    {
    }
    int setShadowColor(const char* color)
    {
        return -1;
    }
    void setShadowOffsetX(float x)
    {
    }
    void setShadowOffsetY(float y)
    {
    }
    float getShadowBlur()
    {
        return 0.0;
    }
    int getShadowColor(char* color, int len)
    {
        if (0 < len && !color)
        {
            *color = 0;
        }
        return 0;
    }
    float getShadowOffsetX()
    {
        return 0.0;
    }
    float getShadowOffsetY()
    {
        return 0.0;
    }

    // drawString enhancement
    int getMozTextStyle(char* textStyle, int textStyleLength);
    int setMozTextStyle(const char* textStyle);
    void mozDrawText(const char* textToDraw);
    float mozMeasureText(const char* textToMeasure);
    void mozPathText(const char* textToPath);
    void mozTextAlongPath(const char* textToDraw, bool stroke);

    void* queryInterface(const Guid& riid)
    {
        void* objectPtr;
        if (riid == ICanvasRenderingContext2D::iid())
        {
            objectPtr = static_cast<ICanvasRenderingContext2D*>(this);
        }
        else if (riid == IInterface::iid())
        {
            objectPtr = static_cast<ICanvasRenderingContext2D*>(this);
        }
        else
        {
            return NULL;
        }
        static_cast<IInterface*>(objectPtr)->addRef();
        return objectPtr;
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

    friend class CanvasGradient;
};

#endif // NINTENDO_ES_LIBCANVAS_CANVAS_H_INCLUDED
