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

#ifndef ES_CSSBOX_H
#define ES_CSSBOX_H

#include <Object.h>
#include <org/w3c/dom/Text.h>

#include <algorithm>
#include <list>
#include <string>

#include <boost/intrusive_ptr.hpp>

#include "http/HTTPRequest.h"
#include "CSSStyleDeclarationImp.h"
#include "FormattingContext.h"

class FontGlyph;
class FontTexture;

namespace org { namespace w3c { namespace dom {

class Element;

namespace bootstrap {

class Box;
class BoxImage;
class FormattingContext;
class LineBox;
class Block;
class StackingContext;
class TableWrapperBox;
class ViewCSSImp;
class WindowImp;

class ContainingBlock : public ObjectMixin<ContainingBlock>
{
public:
    ContainingBlock() :
        width(0.0f),
        height(0.0f)
    {
    }
    virtual ~ContainingBlock() {}

    float width;
    float height;

    float getWidth() const {
        return width;
    }
    float getHeight() const {
        return height;
    }
};

class Box : public ContainingBlock
{
    friend class ViewCSSImp;
    friend class Block;
    friend class LineBox;
    friend class FormattingContext;
    friend class BoxImage;
    friend class StackingContext;

public:
    static const unsigned short BLOCK_LEVEL_BOX = 1;
    static const unsigned short LINE_BOX = 2;
    static const unsigned short INLINE_LEVEL_BOX = 3;

    // flags
    static const unsigned short NEED_SELECTOR_MATCHING = 1;
    static const unsigned short NEED_EXPANSION = 2;
    static const unsigned short NEED_CHILD_EXPANSION = 4;
    static const unsigned short NEED_REFLOW = 8;
    static const unsigned short NEED_REPAINT = 16;
    static const unsigned short NEED_CHILD_LAYOUT = 32;
    static const unsigned short NEED_SELECTOR_REMATCHING = 64;

protected:
    Node node;
    Box* parentBox;
    Box* firstChild;
    Box* lastChild;
    Box* previousSibling;
    Box* nextSibling;
    unsigned int childCount;

    float clearance;
    float marginTop;
    float marginRight;
    float marginBottom;
    float marginLeft;
    float paddingTop;
    float paddingRight;
    float paddingBottom;
    float paddingLeft;
    float borderTop;
    float borderRight;
    float borderBottom;
    float borderLeft;

    unsigned position;
    float offsetH;
    float offsetV;

    StackingContext* stackingContext;
    Box* nextBase;

    bool intrinsic;  // do not change width and height

    float x;  // in screen coord
    float y;  // in screen coord

    unsigned visibility;

    Block* clipBox;

    // background
    unsigned backgroundColor;
    BoxImage* backgroundImage;
    float backgroundLeft;
    float backgroundTop;
    unsigned backgroundStart;

    CSSStyleDeclarationPtr style;
    FormattingContext* formattingContext;

    unsigned short flags;

    WindowImp* childWindow;

    void renderBorderEdge(ViewCSSImp* view, int edge, unsigned borderStyle, unsigned color,
                          float a, float b, float c, float d,
                          float e, float f, float g, float h);
public:
    Box(Node node);
    virtual ~Box();

    virtual unsigned getBoxType() const = 0;

    virtual bool isAnonymous() const {
        return !node;
    }

    Node getNode() const {
        return node;
    }
    WindowImp* getChildWindow() const {
        return childWindow;
    }

    Node getTargetNode() const {
        const Box* box = this;
        do {
            if (box->node)
                return box->node;
        } while (box = box->parentBox);
        return 0;
    }

    Box* removeChild(Box* item);
    Box* insertBefore(Box* item, Box* after);
    Box* appendChild(Box* item);

    void removeChildren() {
        while (hasChildBoxes()) {
            Box* child = getFirstChild();
            removeChild(child);
            child->release_();
        }
    }

    Box* getParentBox() const {
        return parentBox;
    }
    bool hasChildBoxes() const {
        return firstChild;
    }
    Box* getFirstChild() const {
        return firstChild;
    }
    Box* getLastChild() const {
        return lastChild;
    }
    Box* getPreviousSibling() const {
        return previousSibling;
    }
    Box* getNextSibling() const {
        return nextSibling;
    }

    float getX() const {
        return x;
    }
    float getY() const {
        return y;
    }

    float getMarginTop() const {
        return marginTop;
    }
    float getMarginRight() const {
        return marginRight;
    }
    float getMarginBottom() const {
        return marginBottom;
    }
    float getMarginLeft() const {
        return marginLeft;
    }

    float getBorderTop() const {
        return borderTop;
    }
    float getBorderRight() const {
        return borderRight;
    }
    float getBorderBottom() const {
        return borderBottom;
    }
    float getBorderLeft() const {
        return borderLeft;
    }

    bool hasMargins() const {  // have non-zero margins?
        return marginTop != 0.0f || marginRight != 0.0f || marginBottom != 0.0f || marginLeft != 0.0f;
    }

    bool hasClearance() const {
        return !isnan(clearance);
    }
    float getClearance() const {
        return hasClearance() ? clearance : 0.0f;
    }

    float getBlankLeft() const {
        return marginLeft + borderLeft + paddingLeft;
    }
    float getBlankRight() const {
        return marginRight + borderRight + paddingRight;
    }
    float getTotalWidth() const {
        return marginLeft + borderLeft + paddingLeft + width + paddingRight + borderRight + marginRight;
    }
    float getBorderWidth() const {
        return borderLeft + paddingLeft + width + paddingRight + borderRight;
    }
    float getPaddingWidth() const {
        return paddingLeft + width + paddingRight;
    }
    float getBlankTop() const {
        return marginTop + borderTop + paddingTop;
    }
    float getBlankBottom() const {
        return marginBottom + borderBottom + paddingBottom;
    }
    float getTotalHeight() const {
        return marginTop + borderTop + paddingTop + height + paddingBottom + borderBottom + marginBottom;
    }
    float getBorderHeight() const {
        return borderTop + paddingTop + height + paddingBottom + borderBottom;
    }
    float getPaddingHeight() const {
        return paddingTop + height + paddingBottom;
    }
    float getOutlineWidth() const;

    float getEffectiveTotalWidth() const;

    // for block level box
    float getBlockWidth() const {
        float w = marginLeft + getBorderWidth();
        if (0.0f < marginRight)
            w += marginRight;
        return w;
    }
    float getBlockHeight() const {
        float h = marginTop + getBorderHeight();
        if (0.0f < marginBottom)
            h += marginBottom;
        return h;
    }

    float getVerticalOffset() const {
        return offsetV;
    }

    void expandMargins(float t, float r, float b, float l) {
        marginTop += t;
        marginRight += r;
        marginBottom += b;
        marginLeft += l;
    }

    void expandBorders(float t, float r, float b, float l) {
        borderTop += t;
        borderRight += r;
        borderBottom += b;
        borderLeft += l;
    }

    virtual float shrinkTo();
    virtual void fit(float w) {}

    void toViewPort(float& x, float& y) const {
        const Box* box = this;
        do {
            box = box->towardViewPort(x, y);
        } while (box);
    }
    const Box* towardViewPort(float& x, float& y) const {
        x += offsetH + getBlankLeft();
        y += offsetV + getBlankTop();
        if (const Box* box = getParentBox())
            return box->towardViewPort(this, x, y);
        return 0;
    }
    virtual const Box* towardViewPort(const Box* child, float& x, float& y) const {
        return this;
    }

    CSSStyleDeclarationImp* getStyle() const {
        return style.get();
    }
    void setStyle(CSSStyleDeclarationImp* style);
    void restyle(ViewCSSImp* view, CSSStyleDeclarationImp* parentStyle = 0);
    void unresolveStyle();

    bool isStatic() const {
        return position == CSSPositionValueImp::Static;
    }
    bool isRelative() const {
        return position == CSSPositionValueImp::Relative;
    }
    bool isAbsolute() const {
        return position == CSSPositionValueImp::Absolute;
    }
    bool isFixed() const {
        return position == CSSPositionValueImp::Fixed;
    }
    bool isPositioned() const {
        return !isStatic();
    }
    void setPosition(unsigned value) {
        position = value;
    }

    virtual const ContainingBlock* getContainingBlock(ViewCSSImp* view) const;

    FormattingContext* updateFormattingContext(FormattingContext* context);
    FormattingContext* restoreFormattingContext(FormattingContext* context);
    FormattingContext* establishFormattingContext();
    bool isFlowRoot() const {
        return formattingContext;
    }
    bool isFlowOf(const Box* floatRoot) const;

    bool isInFlow() const {
        // cf. 9.3 Positioning schemes
        return !isFloat() && !isAbsolutelyPositioned() && getParentBox();
    }

    virtual bool isAbsolutelyPositioned() const {
        return false;
    }
    virtual bool isFloat() const {
        return false;
    }

    void updatePadding();
    void updateBorderWidth();

    void resolveReplacedWidth(float intrinsicWidth, float intrinsicHeight);
    void applyReplacedMinMax(float w, float h);

    virtual void resolveOffset(float& x, float &y);
    virtual void resolveXY(ViewCSSImp* view, float left, float top, Block* clip) = 0;
    virtual bool layOut(ViewCSSImp* view, FormattingContext* context) {
        return true;
    }

    virtual float getMCW() const {
        return 0.0f;
    }

    bool isVisible() const {
        return visibility == CSSVisibilityValueImp::Visible;
    }

    virtual void render(ViewCSSImp* view, StackingContext* stackingContext) = 0;
    void renderBorder(ViewCSSImp* view, float left, float top,
                      CSSStyleDeclarationImp* style, unsigned backgroundColor, BoxImage* backgroundImage,
                      float ll, float lr, float rl, float rr, float tt, float tb, float bt, float bb,
                      Box* leftEdge, Box* rightEdge);
    void renderBorder(ViewCSSImp* view, float left, float top);
    void renderOutline(ViewCSSImp* view, float left, float top, float right, float bottom, float outlineWidth, unsigned outline, unsigned color);
    void renderOutline(ViewCSSImp* view, float left, float top);

    virtual void dump(std::string indent = "") = 0;

    bool needLayout() const {
        return flags & (NEED_REFLOW | NEED_CHILD_LAYOUT);
    }

    void setFlags(unsigned short f);
    void clearFlags(unsigned short f = 0xffff);
    unsigned short gatherFlags() const;

    bool isInside(int u, int v) const {
        // TODO: InlineBox needs to be treated differently.
        float l = x + marginLeft;
        if (u < l)
            return false;
        float t = y + marginTop;
        if (v < t)
            return false;
        float r = l + getBorderWidth();
        if (r <= u)
            return false;
        float b = t + getBorderHeight();
        return v < b;
    }

    Box* boxFromPoint(int x, int y) {
        for (Box* box = getFirstChild(); box; box = box->getNextSibling()) {
            if (Box* target = box->boxFromPoint(x, y))
                return target;
        }
        return isInside(x, y) ? this : 0;
    }

    static void renderVerticalScrollBar(float w, float h, float pos, float total);
    static void renderHorizontalScrollBar(float w, float h, float pos, float total);
    static void unionRect(float& l, float& t, float& w, float& h,
                          float ll, float tt, float ww, float hh) {
        if (ll < l + w && l < ll + ww && tt < t + h && t < tt + hh) {
            if (l < ll) {
                w -= (ll - l);
                l = ll;
            } else
                ww -= (l - ll);
            if (ww < w)
                w = ww;
            if (t < tt) {
                h -= (tt - t);
                t = tt;
            } else
                hh -= (t - tt);
            if (hh < h)
                h = hh;
            return;
        }
        w = h = 0.0f;
    }

    static Element getContainingElement(Node node);
};

typedef boost::intrusive_ptr<Box> BoxPtr;

class Block;
typedef boost::intrusive_ptr<Block> BlockPtr;

// paragraph
// ‘display’ of ‘block’, ‘list-item’, ‘table’, ‘table-*’ (i.e., all table boxes) or <template>.
class Block : public Box
{
    friend class ViewCSSImp;
    friend class FormattingContext;
    friend class TableWrapperBox;
    friend Box* Box::removeChild(Box* item);

    unsigned textAlign;

    // for a collapsed-through box
    bool marginUsed;
    float topBorderEdge;
    float consumed;

    // for a floating box
    bool inserted;  // set to true if inserted in a linebox.
    float edge;
    float remainingHeight;

    // for an absolutely positioned box
    Retained<ContainingBlock> absoluteBlock;

    // A list of nodes to be formatted in line boxes; note if a block-level box contains
    // block-level boxes, 'inlines' must be empty.
    std::list<Node> inlines;
    Element floatingFirstLetter;
    std::map<Node, BlockPtr> blockMap;  // inline blocks, floating boxes, absolutely positioned boxes, etc. held by line boxes
    TableWrapperBox* anonymousTable;  // for ViewCSSImp::layOutBlockBoxes

    // The default baseline and line-height for the line boxes.
    float defaultBaseline;
    float defaultLineHeight;

    // for automatic table layout
    float mcw;

    // for reflow
    SavedFormattingContext savedFormattingContext;

    void getPsuedoStyles(ViewCSSImp* view, FormattingContext* context, CSSStyleDeclarationImp* style,
                         CSSStyleDeclarationPtr& firstLetterStyle, CSSStyleDeclarationPtr& firstLineStyle);
    void nextLine(ViewCSSImp* view, FormattingContext* context, CSSStyleDeclarationImp*& activeStyle,
                  CSSStyleDeclarationPtr& firstLetterStyle, CSSStyleDeclarationPtr& firstLineStyle,
                  CSSStyleDeclarationImp* style, bool linefeed, FontTexture*& font, float& point);
    size_t layOutFloatingFirstLetter(ViewCSSImp* view, FormattingContext* context, const std::u16string& data, CSSStyleDeclarationImp* firstLetterStyle);
    float measureText(ViewCSSImp* view, CSSStyleDeclarationImp* activeStyle,
                      const char16_t* text, size_t length, float point, bool isFirstCharacter,
                      FontGlyph*& glyph, std::u16string& transformed);
    bool layOutText(ViewCSSImp* view, Node text, FormattingContext* context,
                    std::u16string data, Element element, CSSStyleDeclarationImp* style);
    void layOutInlineBlock(ViewCSSImp* view, Node node, Block* inlineBlock, FormattingContext* context);
    void layOutFloat(ViewCSSImp* view, Node node, Block* floatBox, FormattingContext* context);
    void layOutAbsolute(ViewCSSImp* view, Node node, Block* absBox, FormattingContext* context);  // 1st pass
    bool layOutInline(ViewCSSImp* view, FormattingContext* context, float originalMargin = 0.0f);
    void layOutInlineBlocks(ViewCSSImp* view);
    void layOutChildren(ViewCSSImp* view, FormattingContext* context);

    bool layOutReplacedElement(ViewCSSImp* view, Box* replaced, Element element, CSSStyleDeclarationImp* style);

    void applyMinMaxHeight(FormattingContext* context);

    float getBaseline(const Box* box) const;

protected:
    // resolveAbsoluteWidth's return values
    enum {
        Left = 4u,
        Width = 2u,
        Right = 1u,
        Top = 32u,
        Height = 16u,
        Bottom = 8u
    };

    bool isCollapsedThrough() const;
    float collapseMarginTop(FormattingContext* context);
    bool undoCollapseMarginTop(FormattingContext* context, float before);
    void collapseMarginBottom(FormattingContext* context);
    void adjustCollapsedThroughMargins(FormattingContext* context);
    void moveUpCollapsedThroughMargins(FormattingContext* context);

public:
    Block(Node node = 0, CSSStyleDeclarationImp* style = 0);

    virtual unsigned getBoxType() const {
        return BLOCK_LEVEL_BOX;
    }
    void reset();

    void shrinkToFit();
    virtual float shrinkTo();
    virtual void fit(float w);

    virtual const Box* towardViewPort(const Box* child, float& x, float& y) const {
        if (const Box* box = child->getPreviousSibling()) {
            x -= box->getBlankLeft() + box->offsetH;
            y += box->height + box->getBlankBottom() - box->offsetV;
            return box;
        }
        return this;
    }

    float getBaseline() const;
    float getTopBorderEdge() const {
        return topBorderEdge;
    }

    unsigned getTextAlign() const {
        return textAlign;
    }

    bool hasInline() const {
        return !inlines.empty();
    }
    void insertInline(Node node) {
        inlines.push_back(node);
    }
    void spliceInline(Block* box) {
        inlines.splice(inlines.begin(), box->inlines);
    }

    void addBlock(const Node& node, Block* block) {
        blockMap[node] = block;
    }
    Block* findBlock(const Node& node) {
        Block* box = this;
        do {
            auto i = box->blockMap.find(node);
            if (i != box->blockMap.end())
                return i->second.get();
        } while (box->isAnonymous() && (box = dynamic_cast<Block*>(box->getParentBox())));
        return 0;
    }
    bool removeBlock(const Node& node) {
        Block* box = this;
        do {
            auto i = box->blockMap.find(node);
            if (i != box->blockMap.end()) {
                box->blockMap.erase(i);
                return true;
            }
        } while (box->isAnonymous() && (box = dynamic_cast<Block*>(box->getParentBox())));
        return false;
    }

    virtual bool isAbsolutelyPositioned() const;
    virtual bool isFloat() const;
    bool isFixed() const;
    bool isClipped() const {
        return !isAnonymous() && style && style->overflow.isClipped();
    }

    virtual const ContainingBlock* getContainingBlock(ViewCSSImp* view) const;
    void setContainingBlock(ViewCSSImp* view);

    unsigned resolveAbsoluteWidth(const ContainingBlock* containingBlock, float& left, float& right, float r = NAN);
    unsigned applyAbsoluteMinMaxWidth(const ContainingBlock* containingBlock, float& left, float& right, unsigned autoMask);
    unsigned resolveAbsoluteHeight(const ContainingBlock* containingBlock, float& top, float& bottom, float r = NAN);
    unsigned applyAbsoluteMinMaxHeight(const ContainingBlock* containingBlock, float& top, float& bottom, unsigned autoMask);

    bool hasAnonymousBox(Box* prev) const;
    // Gets the anonymous child box. Creates one if there's none even
    // if there's no children; if so, the existing texts are moved to the
    // new anonymous box.
    Block* getAnonymousBox(Box* prev);

    bool isCollapsableInside() const;
    bool isCollapsableOutside() const;

    void resolveBackground(ViewCSSImp* view);
    void resolveBackgroundPosition(ViewCSSImp* view, const ContainingBlock* containingBlock);
    void resolveMargin(ViewCSSImp* view, const ContainingBlock* containingBlock);
    virtual void resolveWidth(float w);
    void applyMinMaxWidth(float w);
    void resolveNormalWidth(float w, float r = NAN);
    void resolveFloatWidth(float w, float r = NAN);

    virtual float getMCW() const {
        return mcw;
    }

    virtual void resolveOffset(float& x, float &y);
    virtual void resolveXY(ViewCSSImp* view, float left, float top, Block* clip);
    virtual bool layOut(ViewCSSImp* view, FormattingContext* context);
    virtual void render(ViewCSSImp* view, StackingContext* stackingContext);
    virtual void dump(std::string indent = "");

    virtual void layOutAbsolute(ViewCSSImp* view);  // 2nd pass

    unsigned renderBegin(ViewCSSImp* view, bool noBorder = false);
    void renderEnd(ViewCSSImp* view, unsigned overflow, bool scrollBar = true);
    void renderNonInline(ViewCSSImp* view, StackingContext* stackingContext);
    void renderInline(ViewCSSImp* view, StackingContext* stackingContext);

    bool isTableBox() const;
    float updateMCW(float w) {
        mcw = std::max(w, mcw);
        return mcw;
    }
};

// line of text
// ‘display’ of ‘inline’, ‘inline-block’, ‘inline-table’ or ‘ruby’.
class LineBox : public Box
{
    friend class Block;
    friend class FormattingContext;

    float baseline;
    float underlinePosition;
    float underlineThickness;
    float lineThroughPosition;
    float lineThroughThickness;

    float leftGap;    // the gap between the first inline box and the last left floating box
    float rightGap;   // the gap between the last inline box and the 1st right floating box
    Block* rightBox;  // the 1st right floating box

public:
    LineBox(CSSStyleDeclarationImp* style);

    virtual unsigned getBoxType() const {
        return LINE_BOX;
    }

    virtual const Box* towardViewPort(const Box* child, float& x, float& y) const {
        for (const Box* box = child->getPreviousSibling(); box; box = box->getPreviousSibling()) {
            if (box->isAbsolutelyPositioned())
                continue;
            x += box->width + box->getBlankRight() - box->offsetH;
            y -= box->getBlankTop() + box->offsetV;
            return box;
        }
        return this;
    }

    float getBaseline() const {
        return baseline;
    }
    float getUnderlinePosition() const {
        return underlinePosition;
    }
    float getUnderlineThickness() const {
        return underlineThickness;
    }
    float getLineThroughPosition() const {
        return lineThroughPosition;
    }
    float getLineThroughThickness() const {
        return lineThroughThickness;
    }

    // Returns true if this box contains other than floating boxes or absolutely positioned boxes.
    bool hasInlineBox() const {
        return height != 0.0f;  // TODO: Check whether this is correct.
    }

    virtual void resolveXY(ViewCSSImp* view, float left, float top, Block* clip);
    virtual bool layOut(ViewCSSImp* view, FormattingContext* context);
    virtual void render(ViewCSSImp* view, StackingContext* stackingContext);
    virtual void dump(std::string indent);
    virtual float shrinkTo();
    virtual void fit(float w);
};

typedef boost::intrusive_ptr<LineBox> LineBoxPtr;

// words inside a line
class InlineBox : public Box
{
    friend class Block;
    friend class FormattingContext;

    FontTexture* font;
    float point;
    float baseline;
    float leading;
    std::u16string data;

    size_t wrap;
    float wrapWidth;

    int emptyInline;    // 0: none, 1: first, 2: last, 3: both, 4: empty

    void renderText(ViewCSSImp* view, const std::u16string& data, float point);
    void renderMultipleBackground(ViewCSSImp* view);
    void renderEmptyBox(ViewCSSImp* view, CSSStyleDeclarationImp* parentStyle);

public:
    InlineBox(Node node, CSSStyleDeclarationImp* style);

    virtual unsigned getBoxType() const {
        return INLINE_LEVEL_BOX;
    }

    virtual const Box* towardViewPort(const Box* child, float& x, float& y) const {
        // In the case of the inline-block, InlineBox holds a block-level box
        // as its only child.
        return this;
    }

    virtual bool isAnonymous() const {
        return !style || (font && !style->display.isInline()) || !style->display.isInlineLevel();
    }
    bool isInline() const {
        return style && style->display.isInline();
    }

    bool isEmptyInlineAtFirst(CSSStyleDeclarationImp* style, Element& element, Node& node);
    bool isEmptyInlineAtLast(CSSStyleDeclarationImp* style, Element& element, Node& node);

    void clearBlankLeft() {
        marginLeft = paddingLeft = borderLeft = 0.0f;
    }
    void clearBlankRight() {
        marginRight = paddingRight = borderRight = 0.0f;
    }

    size_t getWrap() const {
        return wrap;
    }
    bool hasWrapBox() const {
        // Check font to skip inline-block, etc.
        return font && (data.empty() || wrap < data.length());
    }
    std::u16string getWrapText() const {
        return data.substr(wrap);
    }
    InlineBox* split();

    // has non-zero margins, padding, or borders?
    bool hasHeight() const {
        return 0.0f < getBorderWidth() || 0.0f < getBorderHeight() || hasMargins();
    }

    float getLeading() const {
        return leading;
    }

    float atEndOfLine();

    FontTexture* getFont() const {
        return font;
    }
    float getPoint() const {
        return point;
    }

    float getBaseline() const {
        return baseline;
    }
    float getSub() const;
    float getSuper() const;

    void resolveWidth();
    virtual void resolveOffset(float& x, float &y);
    void setData(FontTexture* font, float point, const std::u16string& data, size_t wrap, float wrapWidth);
    const std::u16string& getData() const {
        return data;
    }
    virtual void resolveXY(ViewCSSImp* view, float left, float top, Block* clip);
    virtual void render(ViewCSSImp* view, StackingContext* stackingContext);
    void renderOutline(ViewCSSImp* view);

    virtual void dump(std::string indent);
};

typedef boost::intrusive_ptr<InlineBox> InlineBoxPtr;

}}}}  // org::w3c::dom::bootstrap

#endif  // ES_CSSBOX_H
