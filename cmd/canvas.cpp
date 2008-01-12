/*
 * Copyright (c) 2007
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

#include <es/exception.h>
#include <es/naming/IContext.h>
#include <es/synchronized.h>
#include <es/usage.h>
#include <algorithm>
#include <math.h>
#include <string>

#include "canvas.h"

ICurrentProcess* System();

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
        state->colorStyles[i] = NS_RGB(0,0,0);
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

u32 Canvas::
parser(const char* color)
{
    std::string s(color);
    int   r;
    int   g;
    int   b;
    float a;
    int ret;
    if (s.find("rgba(") == 0)
    {
        ret = sscanf(s.substr(4).c_str(), "(%d,%d,%d,%f)", &r, &g, &b, &a);
        if (ret == 4 &&
            0 <= r < 256 &&
            0 <= g < 256 &&
            0 <= b < 256 &&
            0.0 <= a <= 1.0)
        {
            float alpha = round(255.0 * a);
            if (alpha < 0)
            {
                alpha = 0.0;
            }
            else if (255.0 < alpha)
            {
                alpha = 255.0;
            }
            return NS_RGB(r, g, b, alpha);
        }
    }
    else if (s.find("rgb(") == 0)
    {
        ret = sscanf(s.substr(3).c_str(), "(%d,%d,%d)", &r, &g, &b);
        if (ret == 3 &&
            0 <= r < 256 &&
            0 <= g < 256 &&
            0 <= b < 256)
        {
            return NS_RGB(r, g, b);
        }
    }
    else if (s.find("#") == 0 && 7 <= s.length())
    {
        ret = sscanf(s.c_str(), "#%2x%2x%2x", &r, &g, &b);
        if (ret == 3 &&
            0 <= r < 256 &&
            0 <= g < 256 &&
            0 <= b < 256)
        {
            return NS_RGB(r, g, b);
        }
    }

    esThrow(EBUSY);
    return 0;
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

    u32 value = currentState()->colorStyles[aWhichStyle];
    if (NS_GET_A(value) == 255)
    {
        sprintf(color, "#%02x%02x%02x", NS_GET_R(value), NS_GET_G(value), NS_GET_B(value));
    }
    else
    {
        char buf[22];
        sprintf(buf, "rgba(%d,%d,%d,%1.2f)",
            NS_GET_R(value), NS_GET_G(value), NS_GET_B(value), static_cast<float>(NS_GET_A(value)) / 255.0);

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
setCairoColor(u32 color)
{
    double r = static_cast<double>(NS_GET_R(color)) / 255.0;
    double g = static_cast<double>(NS_GET_G(color)) / 255.0;
    double b = static_cast<double>(NS_GET_B(color)) / 255.0;
    double a = static_cast<double>(NS_GET_A(color)) / 255.0 * currentState()->globalAlpha;

    cairo_set_source_rgba (cr, r, g, b, a);
}

void Canvas::
setStyle(u32 aWhichStyle, const char* color)
{
    Synchronized<IMonitor*> method(monitor);
    u32 rgba;
    try
    {
        rgba = parser(color);
    }
    catch (...)
    {
        return; // [check]
    }
    currentState()->setColorStyle(aWhichStyle, rgba);
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

int Canvas::
getFillStyle(char* color, unsigned int len)
{
    return getStyle(STYLE_FILL, color, len);
}

float Canvas::
getGlobalAlpha()
{
    Synchronized<IMonitor*> method(monitor);
    return currentState()->globalAlpha;
}

float Canvas::
getMiterLimit()
{
    Synchronized<IMonitor*> method(monitor);
    return cairo_get_miter_limit(cr);
}

int Canvas::
getLineCap(char* capStyle, unsigned int len)
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
getLineJoin(char* joinStyle, unsigned int len)
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
getStrokeStyle(char* color, unsigned int len)
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

void Canvas::
setFillStyle(const char* color)
{
    setStyle(STYLE_FILL, color);
}

void Canvas::
setGlobalAlpha(float alpha)
{
    Synchronized<IMonitor*> method(monitor);
    currentState()->globalAlpha = alpha;
}

void Canvas::
setStrokeStyle(const char* color)
{
    setStyle(STYLE_STROKE, color);
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

void Canvas::
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
        return;

    cairo_set_line_cap (cr, cap);
}

void Canvas::
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
        return;

    cairo_set_line_join (cr, j);
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

