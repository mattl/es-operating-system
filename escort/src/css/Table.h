/*
 * Copyright 2011, 2012 Esrille Inc.
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

#ifndef ES_CSSTABLE_H
#define ES_CSSTABLE_H

#include "css/Box.h"
#include "Box.h"

#include <vector>

namespace org { namespace w3c { namespace dom { namespace bootstrap {

class TableWrapperBox;

class CellBox : public Block
{
    friend class TableWrapperBox;

    bool fixedLayout;
    unsigned col;
    unsigned row;
    unsigned colSpan;
    unsigned rowSpan;
    unsigned verticalAlign;
    float intrinsicHeight;
    float columnWidth;

    float getBaseline(const Box* box) const;

public:
    CellBox(Element element = 0, CSSStyleDeclarationImp* style = 0);

    virtual void fit(float w);

    unsigned getColSpan() const {
        return colSpan;
    }
    void setColSpan(unsigned span) {
        colSpan = span;
    }
    unsigned getRowSpan() const {
        return rowSpan;
    }
    void setRowSpan(unsigned span) {
        rowSpan = span;
    }

    void setPosition(unsigned x, unsigned y) {
        col = x;
        row = y;
    }
    bool isSpanned(unsigned x, unsigned y) const {
        return col != x || row != y;
    }
    bool isLeftSpanned(unsigned x) const {
        return col != x;
    }
    bool isTopSpanned(unsigned y) const {
        return row != y;
    }
    bool isRightSpanned(unsigned x) const {
        return col + colSpan != x;
    }
    bool isBottomSpanned(unsigned y) const {
        return row + rowSpan != y;
    }

    void separateBorders(CSSStyleDeclarationPtr style, unsigned xWidth, unsigned yHeight);
    void collapseBorder(TableWrapperBox* wrapper);

    unsigned getVerticalAlign() const {
        return verticalAlign;
    }
    float getBaseline() const;

    bool isEmptyCell() const;

    float getColumnWidth() const {
        return columnWidth;
    }
    float adjustWidth();

    virtual float shrinkTo();
    virtual void resolveWidth(float w);
    virtual void render(ViewCSSImp* view, StackingContext* stackingContext);
    void renderNonInline(ViewCSSImp* view, StackingContext* stackingContext);
};

typedef boost::intrusive_ptr<CellBox> CellBoxPtr;

class TableWrapperBox : public Block
{
    typedef BlockPtr Caption;

    typedef std::deque<CellBoxPtr> Row;
    typedef std::deque<Row> Grid;

    struct BorderValue
    {
        CSSBorderColorValueImp color;
        CSSBorderStyleValueImp style;
        float width;
    public:
        BorderValue() :
            width(0.0f)
        {}

        bool resolveBorderConflict(CSSBorderColorValueImp& c, CSSBorderStyleValueImp& s, CSSBorderWidthValueImp& w);
        void resolveBorderConflict(CSSStyleDeclarationPtr s, unsigned trbl);
        float getWidth() const {
            return width;
        }
        unsigned getStyle() const {
            unsigned value = style.getValue();
            switch (value) {
            case CSSBorderStyleValueImp::Inset:
                value = CSSBorderStyleValueImp::Ridge;
                break;
            case CSSBorderStyleValueImp::Outset:
                value = CSSBorderStyleValueImp::Groove;
                break;
            default:
                break;
            }
            return value;
        }
    };

    ViewCSSImp* view;
    Element table;

    std::deque<Caption> topCaptions;
    std::deque<Caption> bottomCaptions;

    std::deque<CSSStyleDeclarationPtr> rows;
    std::deque<CSSStyleDeclarationPtr> rowGroups;
    std::deque<CSSStyleDeclarationPtr> columns;
    std::deque<CSSStyleDeclarationPtr> columnGroups;

    Grid grid;
    unsigned xWidth;
    unsigned yHeight;

    Block* tableBox;
    std::vector<BorderValue> borderRows;
    std::vector<BorderValue> borderColumns;

    std::vector<float> widths;
    std::vector<float> fixedWidths;
    std::vector<float> percentages;
    std::vector<float> heights;
    std::vector<float> baselines;
    std::vector<BoxImage*> rowImages;
    std::vector<BoxImage*> rowGroupImages;
    std::vector<BoxImage*> columnImages;
    std::vector<BoxImage*> columnGroupImages;

    bool isAnonymousTable;
    bool isHtmlTable;

    // CSS table model
    bool inRow;
    unsigned xCurrent;
    unsigned yCurrent;
    CellBox* anonymousCell;
    TableWrapperBox* anonymousTable;
    Element pendingTheadElement;
    unsigned yTheadBegin;
    unsigned yTheadEnd;
    Element pendingTfootElement;
    unsigned yTfootBegin;
    unsigned yTfootEnd;

    Block* constructTablePart(Node node);

    unsigned appendRow();
    unsigned appendColumn();

    void processCol(Element col, CSSStyleDeclarationImp* colStyle, Element colgroup);
    void processColGroup(Element colgroup);
    void processRow(Element row);
    void processRowChild(Node node, CSSStyleDeclarationImp* rowStyle);
    void endRow();
    void processRowGroup(Element section);
    void processRowGroupChild(Node node, CSSStyleDeclarationImp* sectionStyle);
    void endRowGroup();
    void processHeader();
    void processFooter();
    void growDownwardGrowingCells();

    void resolveHorizontalBorderConflict(unsigned x, unsigned y, BorderValue* b, CellBox* top, CellBox* bottom);
    void resolveVerticalBorderConflict(unsigned x, unsigned y, BorderValue* b, CellBox* left, CellBox* right);
    bool resolveBorderConflict();
    void computeTableBorders();

    void formCSSTable();
    CellBox* processCell(Element current, Block* parentBox, CSSStyleDeclarationImp* style, CSSStyleDeclarationImp* rowStyle);

    void layOutFixed(ViewCSSImp* view, const ContainingBlock* containingBlock, bool collapsingModel);
    void layOutAuto(ViewCSSImp* view, const ContainingBlock* containingBlock);
    void layOutAutoColgroup(ViewCSSImp* view, const ContainingBlock* containingBlock);
    void layOutTableBox(ViewCSSImp* view, FormattingContext* context, const ContainingBlock* containingBlock, bool collapsingModel, bool fixedLayout);

    void renderBackground(ViewCSSImp* view, CSSStyleDeclarationImp* style, float x, float y, float left, float top, float right, float bottom, float width, float height, unsigned backgroundColor, BoxImage* backgroundImage);
    void renderLayers(ViewCSSImp* view);

public:
    TableWrapperBox(ViewCSSImp* view, Element element, CSSStyleDeclarationImp* style);
    ~TableWrapperBox();

    void constructBlocks();
    void clearGrid();
    void reconstructBlocks(ViewCSSImp* view);

    BorderValue* getRowBorderValue(unsigned x, unsigned y) {
        assert(x < xWidth);
        return &borderRows[xWidth * y + x];
    }
    BorderValue* getColumnBorderValue(unsigned x, unsigned y) {
        assert(x < xWidth + 1);
        return &borderColumns[(xWidth + 1) * y + x];
    }

    bool isTableBox(const Block* box) const {
        return tableBox && box == tableBox;
    }

    bool isAnonymousTableObject() const {
        return isAnonymousTable;
    }

    unsigned getColumnCount() const {
        return xWidth;
    }
    unsigned getRowCount() const {
        return yHeight;
    }

    bool processTableChild(Node node, CSSStyleDeclarationImp* style);

    float getBaseline() const;

    virtual void fit(float w);
    virtual bool layOut(ViewCSSImp* view, FormattingContext* context);
    virtual float shrinkTo();

    virtual void layOutAbsolute(ViewCSSImp* view);  // 2nd pass

    void renderTableBorders(ViewCSSImp* view);
    void renderTableOutlines(ViewCSSImp* view);

    virtual void dump(std::string indent = "");
};

typedef boost::intrusive_ptr<TableWrapperBox> TableWrapperBoxPtr;

}}}}  // org::w3c::dom::bootstrap

#endif  // ES_CSSTABLE_H
