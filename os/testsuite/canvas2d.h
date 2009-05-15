/*
 * Copyright 2008, 2009 Google Inc.
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

#ifndef GOOGLE_ES_LIBCANVAS_CANVAS2D_H_INCLUDED
#define GOOGLE_ES_LIBCANVAS_CANVAS2D_H_INCLUDED

#include <string.h>
#include <es.h>
#include <es/any.h>
#include <es/color.h>
#include <es/handle.h>
#include <es/interlocked.h>
#include <es/list.h>
#include <es/ref.h>
#include <es/base/IFile.h>
#include <es/base/IInterfaceStore.h>
#include <es/base/IProcess.h>
#include <es/base/IStream.h>
#include <w3c/html5.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <cairo.h>

#ifdef __cplusplus
}
#endif

es::CurrentProcess* System();

class CanvasPattern : public es::CanvasPattern
{
    Ref ref;
    es::Monitor* monitor;
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
            delete[] imageData;
        }
        if (monitor)
        {
            monitor->release();
        }
    }

    void apply(const void* context)
    {
        Synchronized<es::Monitor*> method(monitor);

        ASSERT(context);
        cairo_t* cr = static_cast<cairo_t*>(const_cast<void*>(context)); // [check]
        cairo_set_source(cr, pattern);
    }

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::CanvasPattern::iid()) == 0)
        {
            objectPtr = static_cast<es::CanvasPattern*>(this);
        }
        else if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::CanvasPattern*>(this);
        }
        else
        {
            return 0;
        }
        objectPtr->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        int count = ref.addRef();
        return count;
    }

    unsigned int release()
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

class CanvasGradient : public es::CanvasGradient
{
    Ref ref;
    es::Monitor* monitor;
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
        Synchronized<es::Monitor*> method(monitor);

        ASSERT(context);
        cairo_t* cr = static_cast<cairo_t*>(const_cast<void*>(context)); // [check]
        cairo_set_source(cr, pattern);
    }

    void addColorStop(float offset, const char* color);

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::CanvasGradient::iid()) == 0)
        {
            objectPtr = static_cast<es::CanvasGradient*>(this);
        }
        else if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::CanvasGradient*>(this);
        }
        else
        {
            return 0;
        }
        objectPtr->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        int count = ref.addRef();
        return count;
    }

    unsigned int release()
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

class Canvas : public es::CanvasRenderingContext2D
{
    Ref ref;
    es::Monitor* monitor;
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
        friend class Canvas;
        Link<ContextState> link;

        float globalAlpha;
        u32 colorStyles[STYLE_MAX];
        CanvasGradient* gradientStyles[STYLE_MAX];
        CanvasPattern* patternStyles[STYLE_MAX];

        void setColorStyle(int whichStyle, u32 color)
        {
            colorStyles[whichStyle] = color;
            gradientStyles[whichStyle] = 0;
            patternStyles[whichStyle] = 0;
        }

        void setGradientStyle(int whichStyle, CanvasGradient* gradient)
        {
            gradientStyles[whichStyle] = gradient;
            patternStyles[whichStyle] = 0;
        }

        void setPatternStyle(int whichStyle, CanvasPattern* pattern)
        {
            gradientStyles[whichStyle] = 0;
            patternStyles[whichStyle] = pattern;
        }

    public:
        ContextState() :
            globalAlpha(1.0f)
        {
            for (int i = 0; i < STYLE_MAX; ++i)
            {
                colorStyles[i] = 0xff000000;
                gradientStyles[i] = 0;
                patternStyles[i] = 0;
            }
        }

        ContextState(const ContextState* other) :
            globalAlpha(other->globalAlpha)
        {
            for (int i = 0; i < STYLE_MAX; ++i)
            {
                colorStyles[i] = other->colorStyles[i];
                gradientStyles[i] = other->gradientStyles[i];
                patternStyles[i] = other->patternStyles[i];
            }
        }

        int setStyle(int whichStyle, Any style)
        {
            if (style.getType() == Any::TypeObject)
            {
                // TODO
            }

            if (style.getType() == Any::TypeString)
            {
                Rgb rgba;
                try
                {
                    rgba = Rgb(static_cast<const char*>(style));
                }
                catch (...)
                {
                    return -1;
                }
                setColorStyle(whichStyle, rgba);
                return 0;
            }
            return -1;
        }

        Any getStyle(int whichStyle, void* style, int styleLength)
        {
            if (es::CanvasGradient* gradient = gradientStyles[whichStyle])
            {
                gradient->addRef();
                return Any(gradient);
            }

            if (es::CanvasPattern* pattern = patternStyles[whichStyle])
            {
                pattern->addRef();
                return Any(pattern);
            }

            char color[22];
            Rgb value = colorStyles[whichStyle];
            if (value.getA() == 255)
            {
                sprintf(color, "#%02x%02x%02x", value.getR(), value.getG(), value.getB());
            }
            else
            {
                sprintf(color, "rgba(%d,%d,%d,%1.2f)",
                        value.getR(), value.getG(), value.getB(), value.getA() / 255.0f);
            }
            if (strlen(color) + 1 <= styleLength)
            {
                strcpy(static_cast<char*>(style), color);
                return Any(static_cast<const char*>(style));
            }
            return Any("");
        }
    };

    typedef ::List<ContextState, &ContextState::link> StyleStack;
    StyleStack styleStack;

    std::string textStyle;

    // style handling
    u32 lastStyle;
    bool dirtyStyle[STYLE_MAX];

    u8* allocateBitmapData(es::File* image, u32* imageWidth, u32* imageHeight);
    void applyStyle(u32 aWhichStyle);
    void dirtyAllStyles();

    ContextState* currentState()
    {
        return styleStack.getLast();
    }

    void setCairoColor(Rgb color)
    {
        double r = color.getR() / 255.0;
        double g = color.getG() / 255.0;
        double b = color.getB() / 255.0;
        double a = color.getA() / 255.0 * currentState()->globalAlpha;
        cairo_set_source_rgba(cr, r, g, b, a);
    }

public:
    Canvas(cairo_surface_t* surface, int screenWidth, int screenHeight);
    ~Canvas();

    u8* getData();

    //
    // ICanvasRenderingContext2D
    //
    es::HTMLCanvasElement* getCanvas() {}
    void save();
    void restore();
    void scale(float x, float y);
    void rotate(float angle);
    void translate(float x, float y);
    void transform(float m11, float m12, float m21, float m22, float dx, float dy) {}
    void setTransform(float m11, float m12, float m21, float m22, float dx, float dy) {}
    float getGlobalAlpha();
    void setGlobalAlpha(float globalAlpha);
    const char* getGlobalCompositeOperation(void* globalCompositeOperation, int globalCompositeOperationLength);
    void setGlobalCompositeOperation(const char* globalCompositeOperation);
    Any getStrokeStyle(void* strokeStyle, int strokeStyleLength);
    void setStrokeStyle(const Any strokeStyle);
    Any getFillStyle(void* fillStyle, int fillStyleLength);
    void setFillStyle(const Any fillStyle);
    es::CanvasGradient* createLinearGradient(float x0, float y0, float x1, float y1);
    es::CanvasGradient* createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1);
    es::CanvasPattern* createPattern(es::HTMLImageElement* image, const char* repetition) {}
    es::CanvasPattern* createPattern(es::HTMLCanvasElement* image, const char* repetition) {}

    float getLineWidth();
    void setLineWidth(float lineWidth);
    const char* getLineCap(void* lineCap, int lineCapLength);
    void setLineCap(const char* lineCap);
    const char* getLineJoin(void* lineJoin, int lineJoinLength);
    void setLineJoin(const char* lineJoin);
    float getMiterLimit();
    void setMiterLimit(float miterLimit);

    float getShadowOffsetX() {}
    void setShadowOffsetX(float shadowOffsetX) {}
    float getShadowOffsetY() {}
    void setShadowOffsetY(float shadowOffsetY) {}
    float getShadowBlur() {}
    void setShadowBlur(float shadowBlur) {}
    const char* getShadowColor(void* shadowColor, int shadowColorLength) {}
    void setShadowColor(const char* shadowColor) {}

    void clearRect(float x, float y, float width, float height);
    void fillRect(float x, float y, float width, float height);
    void strokeRect(float x, float y, float width, float height);
    void beginPath();
    void closePath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void quadraticCurveTo(float cpx, float cpy, float x, float y);
    void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y);
    void arcTo(float x1, float y1, float x2, float y2, float radius);
    void rect(float x, float y, float width, float height);
    void arc(float x, float y, float radius, float startAngle, float endAngle, bool anticlockwise);
    void fill();
    void stroke();
    void clip();

    bool isPointInPath(float x, float y) {}

    const char* getFont(void* font, int fontLength);
    void setFont(const char* font);
    const char* getTextAlign(void* textAlign, int textAlignLength) {}
    void setTextAlign(const char* textAlign) {}
    const char* getTextBaseline(void* textBaseline, int textBaselineLength) {}
    void setTextBaseline(const char* textBaseline) {}
    void fillText(const char* text, float x, float y);
    void fillText(const char* text, float x, float y, float maxWidth) {}
    void strokeText(const char* text, float x, float y) {}
    void strokeText(const char* text, float x, float y, float maxWidth) {}
    es::TextMetrics* measureText(const char* text) {}

    void drawImage(es::HTMLImageElement* image, float dx, float dy) {}
    void drawImage(es::HTMLImageElement* image, float dx, float dy, float dw, float dh) {}
    void drawImage(es::HTMLImageElement* image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh) {}
    void drawImage(es::HTMLCanvasElement* image, float dx, float dy) {}
    void drawImage(es::HTMLCanvasElement* image, float dx, float dy, float dw, float dh) {}
    void drawImage(es::HTMLCanvasElement* image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh) {}
    void drawImage(es::HTMLVideoElement* image, float dx, float dy) {}
    void drawImage(es::HTMLVideoElement* image, float dx, float dy, float dw, float dh) {}
    void drawImage(es::HTMLVideoElement* image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh) {}
    es::ImageData* createImageData(float sw, float sh) {}
    es::ImageData* getImageData(float sx, float sy, float sw, float sh) {}
    void putImageData(es::ImageData* imagedata, float dx, float dy) {}
    void putImageData(es::ImageData* imagedata, float dx, float dy, float dirtyX, float dirtyY, float dirtyWidth, float dirtyHeight) {}

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::CanvasRenderingContext2D::iid()) == 0)
        {
            objectPtr = static_cast<es::CanvasRenderingContext2D*>(this);
        }
        else if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::CanvasRenderingContext2D*>(this);
        }
        else
        {
            return 0;
        }
        static_cast<Object*>(objectPtr)->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        int count = ref.addRef();
        return count;
    }

    unsigned int release()
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

#endif // GOOGLE_ES_LIBCANVAS_CANVAS2D_H_INCLUDED
