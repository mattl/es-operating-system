#ifndef HTML5_CANVASRENDERINGCONTEXT2D_H_INCLUDED
#define HTML5_CANVASRENDERINGCONTEXT2D_H_INCLUDED

#include <es/any.h>
#include <w3c/html5.h>

#include "CanvasRenderingContext2D.h"
#include "CanvasStyle.h"        // [Custom]
#include "CanvasGradient.h"     // [Custom]
#include "CanvasPattern.h"      // [Custom]

class CanvasRenderingContext2D_Impl : public es::CanvasRenderingContext2D
{
    WebCore::CanvasRenderingContext2D* object;

public:
    CanvasRenderingContext2D_Impl(WebCore::CanvasRenderingContext2D* object)
        : object(object)
    {
    }
    ~CanvasRenderingContext2D_Impl()
    {
    }

    virtual es::HTMLCanvasElement* getCanvas()
    {
    }

    virtual void save()
    {
        object->save();
    }

    virtual void restore()
    {
        object->restore();
    }

    virtual void scale(float x, float y)
    {
        object->scale(x, y);
    }

    virtual void rotate(float angle)
    {
        object->rotate(angle);
    }

    virtual void translate(float x, float y)
    {
        object->translate(x, y);
    }

    virtual void transform(float m11, float m12, float m21, float m22, float dx, float dy)
    {
        object->transform(m11, m12, m21, m22, dx, dy);
    }

    virtual void setTransform(float m11, float m12, float m21, float m22, float dx, float dy)
    {
        object->setTransform(m11, m12, m21, m22, dx, dy);
    }

    virtual float getGlobalAlpha()
    {
        return object->globalAlpha();
    }

    virtual void setGlobalAlpha(float globalAlpha)
    {
        return object->setGlobalAlpha(globalAlpha);
    }

    virtual const char* getGlobalCompositeOperation(void* globalCompositeOperation, int globalCompositeOperationLength)
    {
    }

    virtual void setGlobalCompositeOperation(const char* globalCompositeOperation)
    {
        object->setGlobalCompositeOperation(globalCompositeOperation);
    }

    virtual Any getStrokeStyle(void* strokeStyle, int strokeStyleLength)
    {
    }

    virtual void setStrokeStyle(const Any strokeStyle)
    {
        // [Custom]
        WTF::PassRefPtr<WebCore::CanvasStyle> canvasStyle;
        switch (strokeStyle.getType())
        {
        case Any::TypeString:
            canvasStyle = WebCore::CanvasStyle::create(static_cast<const char*>(strokeStyle));
            break;
        }
        object->setStrokeStyle(canvasStyle);
    }

    virtual Any getFillStyle(void* fillStyle, int fillStyleLength)
    {
    }

    virtual void setFillStyle(const Any fillStyle)
    {
        // [Custom]
        WTF::PassRefPtr<WebCore::CanvasStyle> canvasStyle;
        switch (fillStyle.getType())
        {
        case Any::TypeString:
            canvasStyle = WebCore::CanvasStyle::create(static_cast<const char*>(fillStyle));
            break;
        }
        object->setFillStyle(canvasStyle);
    }

    virtual es::CanvasGradient* createLinearGradient(float x0, float y0, float x1, float y1)
    {
    }

    virtual es::CanvasGradient* createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1)
    {
    }

    virtual es::CanvasPattern* createPattern(es::HTMLImageElement* image, const char* repetition)
    {
    }

    virtual es::CanvasPattern* createPattern(es::HTMLCanvasElement* image, const char* repetition)
    {
    }

    virtual float getLineWidth()
    {
        return object->lineWidth();
    }

    virtual void setLineWidth(float lineWidth)
    {
        object->setLineWidth(lineWidth);
    }

    virtual const char* getLineCap(void* lineCap, int lineCapLength)
    {
    }

    virtual void setLineCap(const char* lineCap)
    {
        object->setLineCap(lineCap);
    }

    virtual const char* getLineJoin(void* lineJoin, int lineJoinLength)
    {
    }

    virtual void setLineJoin(const char* lineJoin)
    {
        object->setLineJoin(lineJoin);
    }

    virtual float getMiterLimit()
    {
        return object->miterLimit();
    }

    virtual void setMiterLimit(float miterLimit)
    {
        return object->setMiterLimit(miterLimit);
    }

    virtual float getShadowOffsetX()
    {
        return object->shadowOffsetX();
    }

    virtual void setShadowOffsetX(float shadowOffsetX)
    {
        object->setShadowOffsetX(shadowOffsetX);
    }

    virtual float getShadowOffsetY()
    {
        return object->shadowOffsetY();
    }

    virtual void setShadowOffsetY(float shadowOffsetY)
    {
        object->setShadowOffsetY(shadowOffsetY);
    }

    virtual float getShadowBlur()
    {
        return object->shadowBlur();
    }

    virtual void setShadowBlur(float shadowBlur)
    {
        object->setShadowBlur(shadowBlur);
    }

    virtual const char* getShadowColor(void* shadowColor, int shadowColorLength)
    {
    }

    virtual void setShadowColor(const char* shadowColor)
    {
        object->setShadowColor(shadowColor);
    }

    virtual void clearRect(float x, float y, float w, float h)
    {
        object->clearRect(x, y, w, h);
    }

    virtual void fillRect(float x, float y, float w, float h)
    {
        object->fillRect(x, y, w, h);
    }

    virtual void strokeRect(float x, float y, float w, float h)
    {
        object->strokeRect(x, y, w, h);
    }

    virtual void beginPath()
    {
        object->beginPath();
    }

    virtual void closePath()
    {
        object->closePath();
    }

    virtual void moveTo(float x, float y)
    {
        object->moveTo(x, y);
    }

    virtual void lineTo(float x, float y)
    {
        object->lineTo(x, y);
    }

    virtual void quadraticCurveTo(float cpx, float cpy, float x, float y)
    {
        object->quadraticCurveTo(cpx, cpy, x, y);
    }

    virtual void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y)
    {
        object->bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);
    }

    virtual void arcTo(float x1, float y1, float x2, float y2, float radius)
    {
        WebCore::ExceptionCode ec = 0;  // XXX arc shouldn't raise an exception.
        object->arcTo(x1, y1, x2, y2, radius, ec);
    }

    virtual void rect(float x, float y, float w, float h)
    {
        object->rect(x, y, w, h);
    }

    virtual void arc(float x, float y, float radius, float startAngle, float endAngle, bool anticlockwise)
    {
        WebCore::ExceptionCode ec = 0;  // XXX arc shouldn't raise an exception.
        object->arc(x, y, radius, startAngle, endAngle, anticlockwise, ec);
    }

    virtual void fill()
    {
        object->fill();
    }

    virtual void stroke()
    {
        object->stroke();
    }

    virtual void clip()
    {
        object->clip();
    }

    virtual bool isPointInPath(float x, float y)
    {
        object->isPointInPath(x, y);
    }

    virtual const char* getFont(void* font, int fontLength)
    {
    }

    virtual void setFont(const char* font)
    {
        object->setFont(font);
    }

    virtual const char* getTextAlign(void* textAlign, int textAlignLength)
    {
    }

    virtual void setTextAlign(const char* textAlign)
    {
        object->setTextAlign(textAlign);
    }

    virtual const char* getTextBaseline(void* textBaseline, int textBaselineLength)
    {
    }

    virtual void setTextBaseline(const char* textBaseline)
    {
        object->setTextBaseline(textBaseline);
    }

    virtual void fillText(const char* text, float x, float y)
    {
        object->fillText(text, x, y);
    }

    virtual void fillText(const char* text, float x, float y, float maxWidth)
    {
        object->fillText(text, x, y, maxWidth);
    }

    virtual void strokeText(const char* text, float x, float y)
    {
        object->strokeText(text, x, y);
    }

    virtual void strokeText(const char* text, float x, float y, float maxWidth)
    {
        object->strokeText(text, x, y, maxWidth);
    }

    virtual es::TextMetrics* measureText(const char* text)
    {
    }

    virtual void drawImage(es::HTMLImageElement* image, float dx, float dy)
    {
    }

    virtual void drawImage(es::HTMLImageElement* image, float dx, float dy, float dw, float dh)
    {
    }

    virtual void drawImage(es::HTMLImageElement* image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
    {
    }

    virtual void drawImage(es::HTMLCanvasElement* image, float dx, float dy)
    {
    }

    virtual void drawImage(es::HTMLCanvasElement* image, float dx, float dy, float dw, float dh)
    {
    }

    virtual void drawImage(es::HTMLCanvasElement* image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
    {
    }

    virtual void drawImage(es::HTMLVideoElement* image, float dx, float dy)
    {
    }

    virtual void drawImage(es::HTMLVideoElement* image, float dx, float dy, float dw, float dh)
    {
    }

    virtual void drawImage(es::HTMLVideoElement* image, float sx, float sy, float sw, float sh, float dx, float dy, float dw, float dh)
    {
    }

    virtual es::ImageData* createImageData(float sw, float sh)
    {
    }

    virtual es::ImageData* getImageData(float sx, float sy, float sw, float sh)
    {
    }

    virtual void putImageData(es::ImageData* imagedata, float dx, float dy)
    {
    }

    virtual void putImageData(es::ImageData* imagedata, float dx, float dy, float dirtyX, float dirtyY, float dirtyWidth, float dirtyHeight)
    {
    }

    virtual Object* queryInterface(const char* qualifiedName)
    {
    }

    virtual unsigned int addRef()
    {
    }

    virtual unsigned int release()
    {
    }
};

#endif  // HTML5_CANVASRENDERINGCONTEXT2D_H_INCLUDED
