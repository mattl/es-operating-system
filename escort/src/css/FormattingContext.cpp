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

#include "FormattingContext.h"

#include <assert.h>

#include "Box.h"
#include "CSSPropertyValueImp.h"
#include "CSSStyleDeclarationImp.h"
#include "StackingContext.h"
#include "ViewCSSImp.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

FormattingContext::FormattingContext() :
    breakable(false),
    isFirstLine(false),
    lineBox(0),
    x(0.0f),
    leftover(0.0f),
    prevChar(0),
    blankLeft(0.0f),
    blankRight(0.0f),
    clearance(0.0f),
    usedMargin(0.0f),
    positiveMargin(0.0f),
    negativeMargin(0.0f),
    previousMargin(NAN),
    baseline(0.0f),
    lineHeight(0.0f),
    atLineHead(true)
{
}

void FormattingContext::updateBlanks(Box* box)
{
    blankLeft += box->marginLeft;
    blankRight += box->marginRight;
}

void FormattingContext::restoreBlanks(Box* box)
{
    blankLeft -= box->marginLeft;
    blankRight -= box->marginRight;
}

float FormattingContext::getLeftoverForFloat(unsigned floatValue) const
{
    // cf. floats-rule3-outside-left-001 and floats-rule3-outside-right-001.
    switch (floatValue) {
    case CSSFloatValueImp::Left:
        if (!right.empty() && right.front()->edge < blankRight)
            return leftover + blankRight - right.front()->edge;
        break;
    case CSSFloatValueImp::Right:
        if (!left.empty() && left.front()->edge < blankLeft)
            return leftover + blankLeft - left.front()->edge;
        break;
    default:
        break;
    }
    return leftover;
}

float FormattingContext::getLeftEdge() const {
    if (left.empty())
        return 0.0f;
    return std::max(0.0f, left.back()->edge - blankLeft);
}

float FormattingContext::getRightEdge() const {
    if (right.empty())
        return 0.0f;
    return std::max(0.0f, right.front()->edge - blankRight);
}

float FormattingContext::getLeftRemainingHeight() const {
    if (left.empty())
        return 0.0f;
    return left.back()->remainingHeight;
}

float FormattingContext::getRightRemainingHeight() const {
    if (right.empty())
        return 0.0f;
    return right.front()->remainingHeight;
}

LineBox* FormattingContext::addLineBox(ViewCSSImp* view, BlockLevelBox* parentBox) {
    assert(!lineBox);
    assert(parentBox);
    baseline = lineHeight = 0.0f;
    isFirstLine = false;
    lineBox = new(std::nothrow) LineBox(parentBox->getStyle());
    if (lineBox) {
        parentBox->appendChild(lineBox);

        // Set marginLeft and marginRight to the current values. Note these
        // margins do not contain the widths of the floating boxes to be
        // added below.
        lineBox->marginLeft = getLeftEdge();
        lineBox->marginRight = getRightEdge();

        x = lineBox->marginLeft;
        leftover = parentBox->width - x - lineBox->marginRight;

        float indent = 0.0f;
        if (parentBox->getFirstChild() == lineBox &&
            (!parentBox->isAnonymous() || parentBox->getParentBox()->getFirstChild() == parentBox))
        {
            indent = parentBox->getStyle()->textIndent.getPx();
            x += indent;
            leftover -= indent;
            isFirstLine = true;
        }

        tryAddFloat(view);
        x = getLeftEdge() + indent;
        leftover = parentBox->width - x - getRightEdge();

        atLineHead = true;
    }
    return lineBox;
}

// if floatNodes is not empty, append float boxes as much as possible.
void FormattingContext::tryAddFloat(ViewCSSImp* view)
{
    while (!floatNodes.empty()) {
        BlockLevelBox* floatBox = view->getFloatBox(floatNodes.front());
        unsigned clear = floatBox->style->clear.getValue();
        if ((clear & CSSClearValueImp::Left) && !left.empty() ||
            (clear & CSSClearValueImp::Right) && !right.empty()) {
            break;
        }
        float w = floatBox->getEffectiveTotalWidth();
        if (leftover < w) {
            if (left.empty() && right.empty() && !lineBox->hasChildBoxes()) {
                addFloat(floatBox, w);
                floatNodes.pop_front();
            }
            break;
        }
        addFloat(floatBox, w);
        floatNodes.pop_front();
    }
}

float FormattingContext::adjustRemainingHeight(float h)
{
    float consumed = 0.0f;
    for (auto i = left.begin(); i != left.end();) {
        consumed = std::max(consumed, std::min(h, (*i)->remainingHeight));
        if (((*i)->remainingHeight -= h) <= 0.0f)
            i = left.erase(i);
        else
            ++i;    // consumed = h;
    }
    for (auto i = right.begin(); i != right.end();) {
        consumed = std::max(consumed, std::min(h, (*i)->remainingHeight));
        if (((*i)->remainingHeight -= h) <= 0.0f)
            i = right.erase(i);
        else
            ++i;
    }
    return consumed;
}

void FormattingContext::useMargin(BlockLevelBox* block)
{
    if (block->marginUsed)
        return;
    block->marginUsed = true;

    block->consumed = 0.0f;
    float m = getMargin();
    if (usedMargin < m) {
        block->consumed = adjustRemainingHeight(m - usedMargin);
        usedMargin = m;
    }
}

float FormattingContext::updateRemainingHeight(float h)
{
    h += getMargin() - usedMargin;
    clearMargin();
    return adjustRemainingHeight(h);
}

float FormattingContext::shiftDown(float width)
{
    float h = 0.0f;
    float w = 0.0f;
    do {
        float lh = getLeftRemainingHeight();
        float rh = getRightRemainingHeight();
        if (0.0f < lh && (lh < rh || rh <= 0.0f)) {
            // Shift down to left
            w = left.back()->getEffectiveTotalWidth();
            x -= w;
            leftover += w;
            if (left.size() == 1) {
                leftover += x;
                x = 0.0f;
            }
            h += lh;
        } else if (0.0f < rh && (rh < lh || lh <= 0.0f)) {
            // Shift down to right
            w = right.front()->getEffectiveTotalWidth();
            leftover += w;
            if (right.size() == 1)
                leftover = width - x;
            h += rh;
        } else if (0.0f < lh) {
            // Shift down to both
            float l = left.back()->getEffectiveTotalWidth();
            w = l + right.front()->getEffectiveTotalWidth();
            x -= l;
            leftover += w;
            if (left.size() == 1) {
                leftover += x;
                x = 0.0f;
            }
            if (right.size() == 1)
                leftover = width - x;
            h += lh;
        } else
            break;
    } while (w == 0.0f);
    return h;
}

// If there's a float, shiftDownLineBox() will expand leftover by extending
// marginTop in lineBox down to the bottom of the nearest float box.
bool FormattingContext::shiftDownLineBox(ViewCSSImp* view)
{
    assert(lineBox);
    if (float h = shiftDown(lineBox->getParentBox()->width)) {
        updateRemainingHeight(h);
        if (lineBox->hasClearance())
            lineBox->clearance += h;
        else
            lineBox->clearance = h;
        clearance += h;
        // Note updateRemainingHeight() could remove more than one floating box from this
        // context, and thus x and leftover have to be recalculated.
        lineBox->marginLeft = getLeftEdge();
        lineBox->marginRight = getRightEdge();
        x = lineBox->marginLeft;
        leftover = lineBox->getParentBox()->width - x - lineBox->marginRight;

        // If there are floating boxes in floatNodes, try adding those first;
        // cf. c414-flt-fit-001.
        tryAddFloat(view);
        x = getLeftEdge();
        leftover = lineBox->getParentBox()->width - x - getRightEdge();
        return true;
    }
    return false;  // no floats
}

bool FormattingContext::hasNewFloats() const
{
    if (!left.empty()) {
        BlockLevelBox* box = left.back();
        if (!box->inserted)
            return true;
    }
    if (!right.empty()) {
        BlockLevelBox* box = right.front();
        if (!box->inserted)
            return true;
    }
    return false;
}

void FormattingContext::appendInlineBox(ViewCSSImp* view, InlineLevelBox* inlineBox, CSSStyleDeclarationImp* activeStyle)
{
    assert(lineBox);
    baseline = lineBox->baseline;
    lineHeight = lineBox->height;

    if (lineBox->baseline == 0.0f && lineBox->height == 0.0f) {
        BlockLevelBox* parentBox = dynamic_cast<BlockLevelBox*>(lineBox->getParentBox());
        assert(parentBox);
        if (parentBox->defaultBaseline == 0.0f && parentBox->defaultLineHeight == 0.0f) {
            CSSStyleDeclarationImp* parentStyle = parentBox->getStyle();
            assert(parentStyle);
            FontTexture* font = view->selectFont(parentStyle);
            assert(font);
            float point = view->getPointFromPx(parentStyle->fontSize.getPx());
            parentBox->defaultLineHeight = parentStyle->lineHeight.getPx();
            float leading = parentBox->defaultLineHeight - font->getLineHeight(point);
            parentBox->defaultBaseline = (leading / 2.0f) + font->getAscender(point);
        }
        lineBox->height = parentBox->defaultLineHeight;
        lineBox->baseline = parentBox->defaultBaseline;
    }

    assert(activeStyle);
    float offset;
    if (activeStyle->display.isInlineLevel())
        offset = activeStyle->verticalAlign.getOffset(view, activeStyle, lineBox, inlineBox);
    else {
        float leading = inlineBox->getLeading() / 2.0f;
        offset = lineBox->baseline - (leading + inlineBox->getBaseline());
    }
    float descender = lineBox->height - lineBox->baseline;
    if (offset < 0.0f) {
        lineBox->baseline -= offset;
        offset = 0.0f;
    }
    if (0.0f < descender && descender < offset + inlineBox->getLeading() + inlineBox->height - lineBox->baseline)
        descender = 0.0f;

    if (0.0f < inlineBox->getLeading() + inlineBox->height)
        lineBox->height = std::max(lineBox->baseline + descender, offset + inlineBox->getLeading() + inlineBox->height);
    lineBox->height = std::max(lineBox->height, activeStyle->lineHeight.getPx());
    lineBox->height = std::max(lineBox->height, lineBox->getStyle()->lineHeight.getPx());

    lineBox->width += inlineBox->getTotalWidth();

    lineBox->appendChild(inlineBox);
    activeStyle->getStackingContext()->addBox(inlineBox, lineBox->getParentBox());

    atLineHead = false;
}

void FormattingContext::dontWrap()
{
    assert(lineBox);
    if (InlineLevelBox* box = dynamic_cast<InlineLevelBox*>(lineBox->getLastChild())) {
        box->wrap = box->data.length();
        box->wrapWidth = box->width;
    }
}

// Complete the current lineBox by adding float boxes if any.
// Then update remainingHeight.
void FormattingContext::nextLine(ViewCSSImp* view, BlockLevelBox* parentBox, bool linefeed)
{
    assert(lineBox);
    assert(lineBox == parentBox->lastChild);

    if (linefeed)
        dontWrap();

    if (InlineLevelBox* inlineLevelBox = dynamic_cast<InlineLevelBox*>(lineBox->getLastChild())) {
        float w = inlineLevelBox->atEndOfLine();
        if (w < 0.0f) {
            if (inlineLevelBox->width <= 0.0f) {
                lineBox->baseline = baseline;
                lineBox->height = lineHeight;
            }
            lineBox->width += w;
            leftover -= w;
            tryAddFloat(view);
        }
    }

    for (auto i = left.rbegin(); i != left.rend(); ++i) {
        BlockLevelBox* floatBox = *i;
        if (!floatBox->inserted) {
            floatBox->inserted = true;
            lineBox->insertBefore(floatBox, lineBox->getFirstChild());
            floatList.push_back(floatBox);
            parentBox->updateMCW(floatBox->mcw);
        }
    }
    for (auto i = right.begin(); i != right.end(); ++i) {
        BlockLevelBox* floatBox = *i;
        if (!floatBox->inserted) {
            floatBox->inserted = true;
            // We would need a gap before the 1st right floating box to be added.
            if (!lineBox->rightBox)
                lineBox->rightBox = floatBox;
            lineBox->appendChild(floatBox);
            floatList.push_back(floatBox);
            parentBox->updateMCW(floatBox->mcw);
        }
    }
    float height = lineBox->getClearance() + lineBox->getTotalHeight();
    if (height != 0.0f)
        updateRemainingHeight(height);

    if (isFirstLine) {
        float indent = parentBox->getStyle()->textIndent.getPx();
        for (auto i = lineBox->getFirstChild(); i; i = i->getNextSibling()) {
            if (i->isAbsolutelyPositioned() || i->isFloat())
                continue;
            i->marginLeft += indent;
            break;
        }
        lineBox->width += indent;
        isFirstLine = false;
    }

    lineBox = 0;
    x = leftover = 0.0f;
    atLineHead = true;
    breakable = false;
}

void FormattingContext::addFloat(BlockLevelBox* floatBox, float totalWidth)
{
    if (floatBox->style->float_.getValue() == CSSFloatValueImp::Left) {
        if (left.empty())
            floatBox->edge = blankLeft + totalWidth;
        else
            floatBox->edge = left.back()->edge + totalWidth;
        left.push_back(floatBox);
        x += totalWidth;
    } else {
        if (right.empty())
            floatBox->edge = blankRight + totalWidth;
        else
            floatBox->edge = right.front()->edge + totalWidth;
        right.push_front(floatBox);
    }
    leftover -= totalWidth;
}

float FormattingContext::clear(unsigned value)
{
    if (!value)
        return 0.0f;
    float h = NAN;
    if (value & 1) {  // clear left
        for (auto i = left.begin(); i != left.end(); ++i) {
            float w = (*i)->getEffectiveTotalWidth();
            x -= w;
            leftover += w;
            if (isnan(h))
                h = (*i)->remainingHeight;
            else
                h = std::max(h, (*i)->remainingHeight);
        }
    }
    if (value & 2) {  // clear right
        for (auto i = right.begin(); i != right.end(); ++i) {
            float w = (*i)->getEffectiveTotalWidth();
            leftover += w;
            if (isnan(h))
                h = (*i)->remainingHeight;
            else
                h = std::max(h, (*i)->remainingHeight);
        }
    }
    if (isnan(h))
        h = 0.0f;
    else {
        // Note there could be a floating box whose remainingHeight is zero.
        adjustRemainingHeight(h);
        h += usedMargin;
        clearMargin();
    }
    assert(!(value & 1) || left.empty());
    assert(!(value & 2) || right.empty());
    return h;
}

float FormattingContext::collapseMargins(float margin)
{
    if (0.0f <= margin) {
        previousMargin = positiveMargin;
        positiveMargin = std::max(positiveMargin, margin);
    } else {
        previousMargin = negativeMargin;
        negativeMargin = std::min(negativeMargin, margin);
    }
    return positiveMargin + negativeMargin;
}

float FormattingContext::undoCollapseMargins()
{
    if (!isnan(previousMargin)) {
        if (0.0f <= previousMargin)
            positiveMargin = previousMargin;
        else
            negativeMargin = previousMargin;
        previousMargin = NAN;
    }
    return positiveMargin + negativeMargin;
}

float FormattingContext::fixMargin()
{
    return updateRemainingHeight(0.0f);
}

// TODO: Remove this function.
void FormattingContext::adjustRemainingFloatingBoxes(float topBorderEdge)
{
    floatList.clear();
}

bool FormattingContext::isFirstCharacter(const std::u16string& text)
{
    bool result = true;
    size_t pos = 0;
    char32_t ch = nextChar(text, pos);
    if (!ch)
        return false;
    InlineLevelBox* box = dynamic_cast<InlineLevelBox*>(lineBox->getLastChild());
    while (box && box->hasWrapBox()) {
        std::u16string wrapText = box->getWrapText();
        size_t wrapLength = wrapText.length();
        if (0 < wrapLength) {
            wrapText += ch; // TODO: check this works with surrogate pairs.
            TextIterator ti;
            ti.setText(wrapText.c_str(), wrapText.length());
            if (!ti.next() || *ti == wrapLength)
                return true;

            char32_t last;
            pos = 0;
            while (pos < wrapLength)
                last = nextChar(wrapText, pos);
            return u_ispunct(last);
        }
        result = false;
        box = dynamic_cast<InlineLevelBox*>(box->getPreviousSibling());
    }
    return result;
}

InlineLevelBox* FormattingContext::getWrapBox(const std::u16string& text)
{
    InlineLevelBox* wrapBox = 0;
    InlineLevelBox* box = dynamic_cast<InlineLevelBox*>(lineBox->getLastChild());
    size_t pos = 0;
    char32_t ch = nextChar(text, pos);
    while (box && box->hasWrapBox()) {
        std::u16string wrapText = box->getWrapText();
        size_t wrapLength = wrapText.length();
        if (ch)
            wrapText += ch; // TODO: check this works with surrogate pairs.
        TextIterator ti;
        ti.setText(wrapText.c_str(), wrapText.length());
        if (!ti.next() || *ti == wrapLength)
            break;
        wrapBox = box;
        if (0 < box->getWrap())
            break;
        box = dynamic_cast<InlineLevelBox*>(box->getPreviousSibling());
        pos = 0;
        if (char32_t n = nextChar(wrapBox->getData(), pos))
            ch = n;
    }
    if (!wrapBox)
        return 0;
    if (0 < wrapBox->getWrap()) {
        box = dynamic_cast<InlineLevelBox*>(wrapBox->getNextSibling());
        wrapBox = wrapBox->split();
        wrapBox->nextSibling = box;
    } else
        box = wrapBox;
    while (box) {
        InlineLevelBox* next = dynamic_cast<InlineLevelBox*>(box->getNextSibling());
        box->getParentBox()->removeChild(box);
        box->nextSibling = next;
        box = next;
    }
    return wrapBox;
}

}}}}  // org::w3c::dom::bootstrap
