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

#include <es/exception.h>
#include <es/naming/IContext.h>
#include <es/synchronized.h>
#include <es/usage.h>
#include <algorithm>
#include <math.h>
#include <string>
#include "canvas.h"

namespace
{
    const char* skipSpace(const char* text)
    {
        while (*text && isspace(*text))
        {
            ++text;
        }
        return text;
    }

    const char* getWord(const char* text, char* word)
    {
        while (*text && !isspace(*text))
        {
            *word++ = *text++;
        }
        *word = '\0';
        return text;
    }
}

Canvas::Canvas(cairo_surface_t* surface, int screenWidth, int screenHeight) :
    surface(surface), screenWidth(screenWidth), screenHeight(screenHeight),
    updated(false)
{
    monitor = System()->createMonitor();

    surfaceWidth = cairo_image_surface_get_width(surface);
    surfaceHeight = cairo_image_surface_get_height(surface);
    surfaceFormat = cairo_image_surface_get_format(surface);

    cr = cairo_create(surface);
    ASSERT(cr);

    cairo_scale (cr, 1, 1); // the unit is 1 pixel.
    cairo_set_source_rgb (cr, 0, 0, 0);

    ContextState* state = new ContextState;
    styleStack.addLast(state);

    for (int i = 0; i < STYLE_MAX; i++)
    {
        state->colorStyles[i] = Rgb(0,0,0);
    }
    lastStyle = -1;
    dirtyAllStyles();

    cairo_set_source_rgba (cr, 255, 255, 255, 1); // white
    cairo_rectangle(cr, 0, 0, screenWidth, screenHeight);
    cairo_fill(cr);

    cairo_set_line_width(cr, 1.0);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_miter_limit(cr, 10.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);

    cairo_new_path(cr);
    updated = true;
}

Canvas::~Canvas()
{
    StyleStack::Iterator iter = styleStack.begin();
    ContextState* state;
    while (state = iter.next())
    {
        delete state;
    }

    if (monitor)
    {
        monitor->release();
    }
}

u8* Canvas::
getData()
{
    Synchronized<IMonitor*> method(monitor);
    if (!updated)
    {
        return NULL;
    }
    updated = false;
    return cairo_image_surface_get_data (surface);
}

void Canvas::
applyStyle(u32 aWhichStyle)
{
    if (lastStyle == aWhichStyle &&
        !dirtyStyle[aWhichStyle])
    {
        // nothing to do, this is already the set style
        return;
    }

    dirtyStyle[aWhichStyle] = false;
    lastStyle = aWhichStyle;

    ICanvasPattern* pattern = currentState()->patternStyles[aWhichStyle];
    if (pattern)
    {
        pattern->apply(cr);
        return;
    }

    if (currentState()->gradientStyles[aWhichStyle])
    {
        ICanvasGradient* grad = currentState()->gradientStyles[aWhichStyle];
        grad->apply(cr);
        return;
    }

    setCairoColor(currentState()->colorStyles[aWhichStyle]);
}

void Canvas::
dirtyAllStyles()
{
    for (int i = 0; i < STYLE_MAX; i++)
    {
        dirtyStyle[i] = true;
    }
}

int Canvas::
getStyle(u32 aWhichStyle, char* color, unsigned int len)
{
    Synchronized<IMonitor*> method(monitor);
    if (len < 8)
    {
        return 0;
    }

    Rgb value = currentState()->colorStyles[aWhichStyle];
    if (value.getA() == 255)
    {
        sprintf(color, "#%02x%02x%02x", value.getR(), value.getG(), value.getB());
    }
    else
    {
        char buf[22];
        sprintf(buf, "rgba(%d,%d,%d,%1.2f)",
            value.getR(), value.getG(), value.getB(), static_cast<float>(value.getA()) / 255.0);

        if (len < strlen(buf))
        {
            *color = 0;
            return 0;
        }
        memmove(color, buf, strlen(buf) + 1);
    }
    return strlen(color);
}

void Canvas::
setCairoColor(Rgb color)
{
    double r = static_cast<double>(color.getR()) / 255.0;
    double g = static_cast<double>(color.getG()) / 255.0;
    double b = static_cast<double>(color.getB()) / 255.0;
    double a = static_cast<double>(color.getA()) / 255.0 * currentState()->globalAlpha;

    cairo_set_source_rgba (cr, r, g, b, a);
}

void Canvas::
setStyle(u32 aWhichStyle, const char* color)
{
    Synchronized<IMonitor*> method(monitor);

    Rgb rgba;
    try
    {
        rgba = Rgb(color);
    }
    catch (...)
    {
        return; // [check]
    }
    currentState()->setColorStyle(aWhichStyle, rgba);
    dirtyStyle[aWhichStyle] = true;
}

void Canvas::
setStyle(u32 aWhichStyle, ICanvasGradient* grad)
{
    Synchronized<IMonitor*> method(monitor);
    currentState()->setGradientStyle(aWhichStyle, grad);
    dirtyStyle[aWhichStyle] = true;
}

void Canvas::
setStyle(u32 aWhichStyle, ICanvasPattern* pat)
{
    Synchronized<IMonitor*> method(monitor);
    currentState()->setPatternStyle(aWhichStyle, pat);
    dirtyStyle[aWhichStyle] = true;
}

void Canvas::
arc(float x, float y, float radius, float startAngle, float endAngle, bool anticlockwise)
{
    Synchronized<IMonitor*> method(monitor);

    if (anticlockwise)
    {
        cairo_arc_negative(cr, x, y, radius, startAngle, endAngle);
    }
    else
    {
        cairo_arc(cr, x, y, radius, startAngle, endAngle);
    }
}

void Canvas::
arcTo(float x1, float y1, float x2, float y2, float radius)
{
    // [check] not implemented.
}

void Canvas::
beginPath()
{
    Synchronized<IMonitor*> method(monitor);
    cairo_new_path(cr);
}

void Canvas::
bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_curve_to(cr, cp1x, cp1y, cp2x, cp2y, x, y);
}

void Canvas::
clearRect(float x, float y, float width, float height)
{
    Synchronized<IMonitor*> method(monitor);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_new_path(cr);
    cairo_rectangle(cr, x, y, width, height);
    cairo_fill(cr);
    cairo_restore(cr);

    updated = true;
}

void Canvas::
clip()
{
    Synchronized<IMonitor*> method(monitor);
    cairo_clip(cr);
}

void Canvas::
closePath()
{
    Synchronized<IMonitor*> method(monitor);
    cairo_close_path(cr);
}

ICanvasGradient* Canvas::
createLinearGradient(float x0, float y0, float x1, float y1)
{
    Synchronized<IMonitor*> method(monitor);

    cairo_pattern_t* gradpat = NULL;

    gradpat = cairo_pattern_create_linear ((double) x0, (double) y0, (double) x1, (double) y1);
    ICanvasGradient* grad = new CanvasGradient(gradpat);
    if (!grad)
    {
        cairo_pattern_destroy(gradpat);
        esReport("createLinearGradient() unable to allocate.\n"); // [check]
        return 0;
    }

    grad->addRef(); // [check] should increment the count?
    return grad;
}

ICanvasGradient* Canvas::
createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1)
{
    Synchronized<IMonitor*> method(monitor);

    cairo_pattern_t* gradpat = 0;
    gradpat = cairo_pattern_create_radial ((double) x0, (double) y0, (double) r0,
                                           (double) x1, (double) y1, (double) r1);
    ICanvasGradient* grad = new CanvasGradient(gradpat);
    if (!grad)
    {
        cairo_pattern_destroy(gradpat);
        esReport("createRadialGradient() allocation error.\n");
        return 0;
    }

    grad->addRef();
    return grad;
}

ICanvasPattern* Canvas::
createPattern(IFile* image, const char* repeat)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_extend_t extend;
    if (strcmp(repeat, "repeat") == 0)
    {
        extend = CAIRO_EXTEND_REPEAT;
    }
    else if (strcmp(repeat, "repeat-x") == 0)
    {
        extend = CAIRO_EXTEND_REPEAT;
    }
    else if (strcmp(repeat, "repeat-y") == 0)
    {
        extend = CAIRO_EXTEND_REPEAT;
    }
    else if (strcmp(repeat, "no-repeat") == 0)
    {
        extend = CAIRO_EXTEND_NONE;
    }
    else
    {
        esReport("createPattern(): error\n");
        return 0; // [check] should report the error.
    }

    cairo_surface_t* imgSurf = 0;
    u8* imgData = 0;
    u32 imgWidth;
    u32 imgHeight;

    imgData = allocateBitmapData(image, &imgWidth, &imgHeight);
    ASSERT(imgData);

    cairo_surface_t* imageSurface = cairo_image_surface_create_for_data(imgData, surfaceFormat, imgWidth, imgHeight, imgWidth * 4);
    if (!imageSurface)
    {
        delete [] imgData;
        esReport("createPattern() error\n");
        return 0;
    }

    cairo_pattern_t* cairopat = cairo_pattern_create_for_surface(imgSurf);
    cairo_surface_destroy(imgSurf);

    cairo_pattern_set_extend (cairopat, extend);

    ICanvasPattern* pat = new CanvasPattern(cairopat, imgData);
    if (!pat)
    {
        cairo_pattern_destroy(cairopat);
        delete [] imgData;
        esReport("createPattern(): error\n");
        return 0;
    }

    pat->addRef(); // [check]
    return pat;
}

void Canvas::
fill()
{
    Synchronized<IMonitor*> method(monitor);

    applyStyle(STYLE_FILL);
    cairo_fill_preserve(cr);
    updated = true;
}

void Canvas::
fillRect(float x, float y, float width, float height)
{
    Synchronized<IMonitor*> method(monitor);

    cairo_new_path (cr);
    cairo_rectangle (cr, x, y, width, height);

    applyStyle(STYLE_FILL);
    cairo_fill(cr);
    updated = true;
}


u8* Canvas::
allocateBitmapData(IFile* image, u32* imageWidth, u32* imageHeight)
{
    // [check] support only 24-bit bitmap format, now.
    u32 coreHeaderSize = 14;
    u32 infoHeaderSize = 40;

    Handle<IStream> stream = image->getStream();
    u32 size = stream->getSize();
    if (size < coreHeaderSize + infoHeaderSize)
    {
        return 0;
    }

    u8* buf = new u8[size];
    if (!buf)
    {
        return 0;
    }

    stream->read(buf, size);

    u8* p = buf;
    if ((*(char*)p++) != 'B' || (*(char*)p++) != 'M')
    {
        delete [] buf;
        return 0;
    }
    p += 4;
    p += 4;

    u32 offset = *((u32*) p);
    p += 4;
    p += 4;

    *imageWidth = *((u32*) p);
    p += 4;
    *imageHeight = *((u32*) p);
    p += 4;
    p += 2;

    int bitCount = *((u16*) p);
    p += 2;
    if (bitCount != 24)
    {
        // unsuppoted.
        delete [] buf;
        return 0;
    }

    u32 dataSize = *imageWidth * *imageHeight * 4;
    u8* data = new u8[dataSize]; // 32-bit bmp.

    u8* src = buf + offset;
    u8* dst = data;
    while (src < buf + size && dst < data + dataSize)
    {
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = *src++;
        *dst++ = 0xff;
    }
    ASSERT(src == buf + size);
    ASSERT(dst == data + dataSize);

    delete [] buf; // debug.
    return data;
}

void Canvas::
drawImage(IFile* image, float dx, float dy, float dw, float dh)
{
    Synchronized<IMonitor*> method(monitor);

    int width = static_cast<int>(dw);
    int height = static_cast<int>(dh);
    if (width <= 0 || height <= 0 || !image)
    {
        return;
    }

    u32 imageWidth;
    u32 imageHeight;
    u8* data = allocateBitmapData(image, &imageWidth, &imageHeight);
    ASSERT(data);

    cairo_surface_t* imageSurface = cairo_image_surface_create_for_data(data, surfaceFormat, width, height, width * 4);
    if (!imageSurface)
    {
        delete [] data;
        esReport("drawImage() error\n");
        return;
    }
    cairo_surface_reference(imageSurface); // [check]
    delete [] data;

    cairo_matrix_t flipVertical; // flip this image.
    cairo_matrix_init(&flipVertical, 1, 0, 0, -1, 0, imageHeight);

    float sx = 0.0;
    float sy = 0.0; // [check]
    float sw = static_cast<float>(imageWidth); // [check]
    float sh = static_cast<float>(imageHeight); // [check]

    cairo_matrix_t surfMat; // move and scale the image.
    cairo_matrix_init_translate(&surfMat, sx, sy);
    cairo_matrix_scale(&surfMat, sw/dw, sh/dh); // [check]

    cairo_matrix_t matrix;
    cairo_matrix_multiply(&matrix, &flipVertical, &surfMat);

    // imageSurface
    cairo_pattern_t* pat;
    pat = cairo_pattern_create_for_surface(imageSurface);
    ASSERT(cairo_pattern_status(pat) == CAIRO_STATUS_SUCCESS);
    cairo_pattern_set_matrix(pat, &matrix);

    cairo_save(cr);
    cairo_translate(cr, dx, dy);
    cairo_new_path(cr);
    cairo_rectangle(cr, 0, 0, dw, dh);
    cairo_set_source(cr, pat);
    cairo_clip(cr);
    cairo_paint_with_alpha(cr, currentState()->globalAlpha);
    cairo_restore(cr);

    /* [check] workaround?
    cairo_new_path(cr);
    cairo_rectangle(cr, 0, 0, 0, 0);
    cairo_fill(cr);
    */

    cairo_pattern_destroy(pat);
    cairo_surface_destroy(imageSurface);
    updated = true;
}

ICanvasGradient* Canvas::
getFillGradient()
{
    Synchronized<IMonitor*> method(monitor);
    return currentState()->gradientStyles[STYLE_FILL];
}

ICanvasPattern* Canvas::
getFillPattern()
{
    Synchronized<IMonitor*> method(monitor);
    return currentState()->patternStyles[STYLE_FILL];
}

int Canvas::
getFillStyle(char* color, int len)
{
    return getStyle(STYLE_FILL, color, len);
}

float Canvas::
getGlobalAlpha()
{
    Synchronized<IMonitor*> method(monitor);
    return currentState()->globalAlpha;
}

int Canvas::
getGlobalCompositeOperation(char* operation, int len)
{
    Synchronized<IMonitor*> method(monitor);

    cairo_operator_t cairo_op = cairo_get_operator(cr);

#define CANVAS_OP_TO_CAIRO_OP(cvsop,cairoop) \
    if (cairo_op == CAIRO_OPERATOR_##cairoop) \
        strcpy(operation, cvsop);

    // XXX "darker" isn't really correct
    CANVAS_OP_TO_CAIRO_OP("clear", CLEAR)
    else CANVAS_OP_TO_CAIRO_OP("copy", SOURCE)
    else CANVAS_OP_TO_CAIRO_OP("darker", SATURATE)  // XXX
    else CANVAS_OP_TO_CAIRO_OP("destination-atop", DEST_ATOP)
    else CANVAS_OP_TO_CAIRO_OP("destination-in", DEST_IN)
    else CANVAS_OP_TO_CAIRO_OP("destination-out", DEST_OUT)
    else CANVAS_OP_TO_CAIRO_OP("destination-over", DEST_OVER)
    else CANVAS_OP_TO_CAIRO_OP("lighter", ADD)
    else CANVAS_OP_TO_CAIRO_OP("source-atop", ATOP)
    else CANVAS_OP_TO_CAIRO_OP("source-in", IN)
    else CANVAS_OP_TO_CAIRO_OP("source-out", OUT)
    else CANVAS_OP_TO_CAIRO_OP("source-over", OVER)
    else CANVAS_OP_TO_CAIRO_OP("xor", XOR)
    else return 0;

#undef CANVAS_OP_TO_CAIRO_OP

return strlen(operation);
}

float Canvas::
getMiterLimit()
{
    Synchronized<IMonitor*> method(monitor);
    return cairo_get_miter_limit(cr);
}

int Canvas::
getLineCap(char* capStyle, int len)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_line_cap_t cap = cairo_get_line_cap(cr);

    if (cap == CAIRO_LINE_CAP_BUTT && strlen("butt") + 1 < len)
        sprintf(capStyle, "butt");
    else if (cap == CAIRO_LINE_CAP_ROUND && strlen("round") + 1 < len)
        sprintf(capStyle, "round");
    else if (cap == CAIRO_LINE_CAP_SQUARE && strlen("square") + 1 < len)
        sprintf(capStyle, "square");
    else
        return 0;

    return strlen(capStyle);
}

int Canvas::
getLineJoin(char* joinStyle, int len)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_line_join_t j = cairo_get_line_join(cr);

    if (j == CAIRO_LINE_JOIN_ROUND && strlen("round") + 1 < len)
        sprintf(joinStyle, "round");
    else if (j == CAIRO_LINE_JOIN_BEVEL && strlen("bevel") + 1 < len)
        sprintf(joinStyle, "bevel");
    else if (j == CAIRO_LINE_JOIN_MITER && strlen("miter") + 1 < len)
        sprintf(joinStyle, "miter");
    else
        return 0;

    return strlen(joinStyle);
}

float Canvas::
getLineWidth()
{
    Synchronized<IMonitor*> method(monitor);
    return cairo_get_line_width(cr);
}

int Canvas::
getStrokeStyle(char* color, int len)
{
    return getStyle(STYLE_STROKE, color, len);
}

void Canvas::
lineTo(float x, float y)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_line_to (cr, x, y);
}

void Canvas::
moveTo(float x, float y)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_move_to (cr, x, y);
}

void Canvas::
quadraticCurveTo(float cpx, float cpy, float x, float y)
{
    Synchronized<IMonitor*> method(monitor);

    double cx, cy;
    cairo_get_current_point(cr, &cx, &cy);
    cairo_curve_to(cr,
                   (cx + cpx * 2.0) / 3.0,
                   (cy + cpy * 2.0) / 3.0,
                   (cpx * 2.0 + x) / 3.0,
                   (cpy * 2.0 + y) / 3.0,
                   x,
                   y);
}

void Canvas::
rect(float x, float y, float width, float height)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_rectangle(cr, x, y, width, height);
}

void Canvas::
restore()
{
    Synchronized<IMonitor*> method(monitor);

    styleStack.removeLast();
    lastStyle = -1;
    dirtyAllStyles();

    cairo_restore(cr);
}

void  Canvas::
rotate(float angle)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_rotate(cr, angle);
}

void Canvas::
save()
{
    Synchronized<IMonitor*> method(monitor);
    cairo_save(cr);

    ContextState* state = new ContextState(currentState());
    styleStack.addLast(state);
}

void Canvas::
scale(float scaleW, float scaleH)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_scale(cr, scaleW, scaleH);
}

int Canvas::
setFillStyle(const char* color)
{
    setStyle(STYLE_FILL, color);
    return 0;
}

void Canvas::
setFillGradient(ICanvasGradient* gradient)
{
    ASSERT(gradient);
    setStyle(STYLE_FILL, gradient);
}

void Canvas::
setFillPattern(ICanvasPattern* pattern)
{
    ASSERT(pattern);
    setStyle(STYLE_FILL, pattern);
}

void Canvas::
setGlobalAlpha(float alpha)
{
    Synchronized<IMonitor*> method(monitor);
    currentState()->globalAlpha = alpha;
}

int Canvas::
setGlobalCompositeOperation(const char* operation)
{
    Synchronized<IMonitor*> method(monitor);

    cairo_operator_t cairo_op;

#define CANVAS_OP_TO_CAIRO_OP(cvsop,cairoop) \
   if (strcmp(operation, cvsop) == 0) cairo_op = CAIRO_OPERATOR_##cairoop;

    // XXX "darker" isn't really correct
    CANVAS_OP_TO_CAIRO_OP("clear", CLEAR)
    else CANVAS_OP_TO_CAIRO_OP("copy", SOURCE)
    else CANVAS_OP_TO_CAIRO_OP("darker", SATURATE)  // XXX
    else CANVAS_OP_TO_CAIRO_OP("destination-atop", DEST_ATOP)
    else CANVAS_OP_TO_CAIRO_OP("destination-in", DEST_IN)
    else CANVAS_OP_TO_CAIRO_OP("destination-out", DEST_OUT)
    else CANVAS_OP_TO_CAIRO_OP("destination-over", DEST_OVER)
    else CANVAS_OP_TO_CAIRO_OP("lighter", ADD)
    else CANVAS_OP_TO_CAIRO_OP("source-atop", ATOP)
    else CANVAS_OP_TO_CAIRO_OP("source-in", IN)
    else CANVAS_OP_TO_CAIRO_OP("source-out", OUT)
    else CANVAS_OP_TO_CAIRO_OP("source-over", OVER)
    else CANVAS_OP_TO_CAIRO_OP("xor", XOR)
    // not part of spec, kept here for compat
    else CANVAS_OP_TO_CAIRO_OP("over", OVER)
    else
    {
        esReport("setGlobalCompositeOperation(): invalie operation.(%s)\n", operation);
        return -1;
    }

    cairo_set_operator(cr, cairo_op);
    return 0;
}

int Canvas::
setStrokeStyle(const char* color)
{
    setStyle(STYLE_STROKE, color);
    return 0;
}

void Canvas::
setMiterLimit(float limit)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_set_miter_limit(cr, limit);
}

void Canvas::
setLineWidth(float width)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_set_line_width (cr, width);
}

int Canvas::
setLineCap(const char* capStyle)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_line_cap_t cap;
    if (strcmp(capStyle, "butt") == 0)
        cap = CAIRO_LINE_CAP_BUTT;
    else if (strcmp(capStyle, "round") == 0)
        cap = CAIRO_LINE_CAP_ROUND;
    else if (strcmp(capStyle, "square") == 0)
        cap = CAIRO_LINE_CAP_SQUARE;
    else
        return -1;

    cairo_set_line_cap (cr, cap);
    return 0;
}

int Canvas::
setLineJoin(const char* joinStyle)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_line_join_t j;

    if (strcmp(joinStyle, "round") == 0)
        j = CAIRO_LINE_JOIN_ROUND;
    else if (strcmp(joinStyle, "bevel") == 0)
        j = CAIRO_LINE_JOIN_BEVEL;
    else if (strcmp(joinStyle, "miter") == 0)
        j = CAIRO_LINE_JOIN_MITER;
    else
        return -1;

    cairo_set_line_join (cr, j);
    return 0;
}

void Canvas::
stroke()
{
    Synchronized<IMonitor*> method(monitor);
    applyStyle(STYLE_STROKE);
    cairo_stroke (cr);
    updated = true;
}

void Canvas::
strokeRect(float x, float y, float width, float height)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_new_path(cr);
    cairo_rectangle(cr, x, y, width, height);

    applyStyle(STYLE_STROKE);
    cairo_stroke(cr);
    updated = true;
}

void Canvas::
translate(float tx, float ty)
{
    Synchronized<IMonitor*> method(monitor);
    cairo_translate(cr, tx, ty);
}

int Canvas::
getTextStyle(char* style, int len)
{
    if (textStyle.size() < len)
    {
        len = textStyle.size() + 1;
    }
    strncpy(style, textStyle.c_str(), len);
    style[len - 1] = '\0';
    return len;
}

int Canvas::
setTextStyle(const char* style)
{
    Synchronized<IMonitor*> method(monitor);

    textStyle = style;

    const char* family;
    char word[256];
    double size = 20.0;

    cairo_font_slant_t slant = CAIRO_FONT_SLANT_NORMAL;
    cairo_font_weight_t weight = CAIRO_FONT_WEIGHT_NORMAL;

    do
    {
        family = style = skipSpace(style);
        style = getWord(style, word);
        if (isdigit(*word))
        {
            char* unit;
            size = strtod(word, &unit);
            // XXX Check unit
        }
        else if (strcasecmp(word, "italic") == 0)
        {
            slant = CAIRO_FONT_SLANT_ITALIC;
        }
        else if (strcasecmp(word, "oblique") == 0)
        {
            slant = CAIRO_FONT_SLANT_OBLIQUE;
        }
        else if (strcasecmp(word, "bold") == 0)
        {
            weight = CAIRO_FONT_WEIGHT_BOLD;
        }
        else if (strcasecmp(word, "normal") == 0)
        {
            slant = CAIRO_FONT_SLANT_NORMAL;
            weight = CAIRO_FONT_WEIGHT_NORMAL;
        }
        else
        {
            break;
        }
    } while (*style);

    cairo_select_font_face (cr, family, slant, weight);
    cairo_set_font_size (cr, size);
    return 0;
}

void Canvas::
drawText(const char* textToDraw)
{
    Synchronized<IMonitor*> method(monitor);

    applyStyle(STYLE_FILL);
    cairo_show_text(cr, textToDraw);

    updated = true;
}

float Canvas::
measureText(const char* textToMeasure)
{
    Synchronized<IMonitor*> method(monitor);

    cairo_text_extents_t te;
    cairo_text_extents(cr, textToMeasure, &te);
    return te.width;
}

void Canvas::
pathText(const char* textToPath)
{
    Synchronized<IMonitor*> method(monitor);

    cairo_text_path(cr, textToPath);
}

void Canvas::
textAlongPath(const char* textToDraw, bool stroke)
{
}


//
// CanvasGradient
//

void CanvasGradient::
addColorStop(float offset, const char* color)
{
    Synchronized<IMonitor*> method(monitor);

    if (offset < 0.0 || 1.0 < offset)
    {
        return; // [check] should report the error.
    }

    Rgb rgba;
    try
    {
        rgba = Rgb(color);
    }
    catch (...)
    {
        return; // [check] should report the error.
    }

    cairo_pattern_add_color_stop_rgba(pattern, (double) offset,
                                      rgba.getR() / 255.0,
                                      rgba.getG() / 255.0,
                                      rgba.getB() / 255.0,
                                      rgba.getA() / 255.0);
}
