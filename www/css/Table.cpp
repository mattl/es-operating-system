/*
 * Copyright 2011 Esrille Inc.
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

#include "Table.h"

#include <org/w3c/dom/html/HTMLTableElement.h>
#include <org/w3c/dom/html/HTMLTableCaptionElement.h>
#include <org/w3c/dom/html/HTMLTableCellElement.h>
#include <org/w3c/dom/html/HTMLTableColElement.h>
#include <org/w3c/dom/html/HTMLTableRowElement.h>
#include <org/w3c/dom/html/HTMLTableSectionElement.h>

#include "ViewCSSImp.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

namespace {

int isOneOf(const std::u16string& s, std::initializer_list<const char16_t*> list)
{
    int index = 0;
    for (auto i = list.begin(); i != list.end(); ++i, ++index) {
        if (s.compare(*i) == 0)
            return index;
    }
    return -1;
}

}  // namespace

void CellBox::fit(float w)
{
    if (getBlockWidth() == w)
        return;
    if (!isAnonymous() && !style->width.isAuto())
        return;
    width = w - getBlankLeft() - getBlankRight();
    for (Box* child = getFirstChild(); child; child = child->getNextSibling())
        child->fit(width);
}

void CellBox::separatedBorders(CSSStyleDeclarationPtr style, unsigned xWidth, unsigned yHeight)
{
    float hs = style->borderSpacing.getHorizontalSpacing();
    float vs = style->borderSpacing.getVerticalSpacing();
    marginTop = (row == 0) ? vs : (vs / 2.0f);
    marginRight = (col + 1 == xWidth) ? hs : (hs / 2.0f);
    marginBottom = (row + 1 == yHeight) ? vs : (vs / 2.0f);
    marginLeft = (col == 0) ? hs : (hs / 2.0f);
}

void CellBox::collapseBorder(TableWrapperBox* wrapper)
{
    borderTop = borderRight = borderBottom = borderLeft = 0.0f;
    marginTop = wrapper->getRowBorderValue(col, row)->getWidth() / 2.0f;
    marginRight = wrapper->getColumnBorderValue(col + 1, row)->getWidth() / 2.0f;
    marginBottom = wrapper->getRowBorderValue(col, row + 1)->getWidth() / 2.0f;
    marginLeft = wrapper->getColumnBorderValue(col, row)->getWidth() / 2.0f;
}

TableWrapperBox::TableWrapperBox(ViewCSSImp* view, Element element, CSSStyleDeclarationImp* style) :
    BlockLevelBox(element, style),
    view(view),
    xWidth(0),
    yHeight(0),
    table(element),
    tableBox(0)
{
    // TODO: Support cases where display is not table or inline-table.
    assert(style->display == CSSDisplayValueImp::Table || style->display == CSSDisplayValueImp::InlineTable);
    formTable(view);

    // Top caption boxes
    for (auto i = topCaptions.begin(); i != topCaptions.end(); ++i)
        appendChild(i->get());

    // Table box
    tableBox = new(std::nothrow) BlockLevelBox();
    if (tableBox) {
        for (unsigned y = 0; y < yHeight; ++y) {
            LineBox* lineBox = new(std::nothrow) LineBox(0);
            if (!lineBox)
                continue;
            tableBox->appendChild(lineBox);
            for (unsigned x = 0; x < xWidth; ++x) {
                CellBox* cellBox = grid[y][x].get();
                if (!cellBox || cellBox->isSpanned(x, y))
                    continue;
                lineBox->appendChild(cellBox);
            }
        }
        appendChild(tableBox);
    }

    // Bottom caption boxes
    for (auto i = bottomCaptions.begin(); i != bottomCaptions.end(); ++i)
        appendChild(i->get());
}

unsigned TableWrapperBox::appendRow()
{
    ++yHeight;
    grid.resize(yHeight);
    grid.back().resize(xWidth);
    rows.resize(yHeight);
    rowGroups.resize(yHeight);
    return yHeight;
}

unsigned TableWrapperBox::appendColumn()
{
    ++xWidth;
    for (auto r = grid.begin(); r != grid.end(); ++r)
        r->resize(xWidth);
    columns.resize(xWidth);
    columnGroups.resize(xWidth);
    return xWidth;
}

void TableWrapperBox::formTable(ViewCSSImp* view)
{
    if (table.getChildElementCount() == 0)
        return;

    // 10.
    unsigned yCurrent = 0;
    // 11.
    std::list<Element> pendingTfootElements;
    // std::list<> downwardGrowingCells;
    for (Element current = table.getFirstElementChild(); current; current = current.getNextElementSibling()) {
        CSSStyleDeclarationImp* currentStyle = view->getStyle(current);
        assert(currentStyle);
        if (!currentStyle->display.isProperTableChild()) {
            yCurrent = endRowGroup(yCurrent);
            continue;
        }
        unsigned display = currentStyle->display.getValue();
        if (display == CSSDisplayValueImp::TableCaption) {
            BlockLevelBox* caption = view->layOutBlockBoxes(current, 0, 0, currentStyle, false);
            if (!caption)
                continue;
            if (currentStyle->captionSide.getValue() == CSSCaptionSideValueImp::Top)
                topCaptions.push_back(caption);
            else
                bottomCaptions.push_back(caption);
            continue;
        }
        if (display == CSSDisplayValueImp::TableColumnGroup) {
            yCurrent = endRowGroup(yCurrent);
            processColGruop(view, current);
            continue;
        }
        if (display == CSSDisplayValueImp::TableColumn)  // TODO HTML doesn't need this though
            continue;
        // 12.
        // TODO: ?
        // 13.
        if (display == CSSDisplayValueImp::TableRow) {
            // TODO
            yCurrent = processRow(view, current, yCurrent);
            continue;
        }
        if (display == CSSDisplayValueImp::TableFooterGroup) {
            pendingTfootElements.push_back(current);
            continue;
        }
        assert(display == CSSDisplayValueImp::TableHeaderGroup || display == CSSDisplayValueImp::TableRowGroup);
        yCurrent = processRowGruop(view, current, yCurrent);
    }
    while (!pendingTfootElements.empty()) {
        Element tfoot = pendingTfootElements.front();
        yCurrent = processRowGruop(view, tfoot, yCurrent);
        pendingTfootElements.pop_front();
    }
}

unsigned TableWrapperBox::processRow(ViewCSSImp* view, Element row, unsigned yCurrent)
{
    if (yHeight == yCurrent)
        appendRow();
    rows[yCurrent] = view->getStyle(row);

    unsigned xCurrent = 0;
    growDownwardGrowingCells();
    for (Element child = row.getFirstElementChild(); child; child = child.getNextElementSibling()) {
        CSSStyleDeclarationImp* childStyle = view->getStyle(child);
        assert(childStyle);
        if (childStyle->display.getValue() != CSSDisplayValueImp::TableCell)
            continue;
        while (xCurrent < xWidth && grid[yCurrent][xCurrent])
            ++xCurrent;
        if (xCurrent == xWidth)
            appendColumn();
        unsigned colspan = 1;
        unsigned rowspan = 1;
        if (html::HTMLTableCellElement::hasInstance(child)) {
            html::HTMLTableCellElement cell(interface_cast<html::HTMLTableCellElement>(child));
            colspan = cell.getColSpan();
            rowspan = cell.getRowSpan();
        }
        // TODO: 10 ?
        bool cellGrowsDownward = false;
        while (xWidth < xCurrent + colspan)
            appendColumn();
        while (yHeight < yCurrent + rowspan)
            appendRow();
        CellBox* cellBox = static_cast<CellBox*>(view->layOutBlockBoxes(child, 0, 0, childStyle, true));
        if (cellBox) {
            cellBox->setPosition(xCurrent, yCurrent);
            cellBox->setColSpan(colspan);
            cellBox->setRowSpan(rowspan);
            for (unsigned x = xCurrent; x < xCurrent + colspan; ++x) {
                for (unsigned y = yCurrent; y < yCurrent + rowspan; ++y)
                    grid[y][x] = cellBox;
            }
        }
        // TODO: 13
        if (cellGrowsDownward)
            ;  // TODO: 14
        xCurrent += colspan;
    }
    return ++yCurrent;
}

unsigned TableWrapperBox::processRowGruop(ViewCSSImp* view, Element section, unsigned yCurrent)
{
    unsigned yStart = yHeight;
    for (Element child = section.getFirstElementChild(); child; child = child.getNextElementSibling()) {
        CSSStyleDeclarationImp* childStyle = view->getStyle(child);
        assert(childStyle);
        if (childStyle->display.getValue() != CSSDisplayValueImp::TableRow)
            continue;
        unsigned next = processRow(view, child, yCurrent);
        rowGroups[yCurrent] = view->getStyle(section);
        yCurrent = next;
    }
    // TODO 3.
    return endRowGroup(yCurrent);
}

unsigned TableWrapperBox::endRowGroup(int yCurrent)
{
    while (yCurrent < yHeight) {
        growDownwardGrowingCells();
        ++yCurrent;
    }
    // downwardGrowingCells.clear();
    return yCurrent;
}

void TableWrapperBox::processColGruop(ViewCSSImp* view, Element colgroup)
{
    bool hasCol = false;
    int xStart = xWidth;
    for (Element child = colgroup.getFirstElementChild(); child; child = child.getNextElementSibling()) {
        CSSStyleDeclarationImp* childStyle = view->getStyle(child);
        assert(childStyle);
        if (childStyle->display.getValue() != CSSDisplayValueImp::TableColumn)
            continue;
        hasCol = true;
        unsigned int span = 1;
        if (html::HTMLTableColElement::hasInstance(child)) {
            html::HTMLTableColElement col(interface_cast<html::HTMLTableColElement>(child));
            span = col.getSpan();
        }
        while (0 < span--) {
            appendColumn();
            columns[xWidth - 1] = childStyle;
            columnGroups[xWidth - 1] = view->getStyle(colgroup);
        }
        // TODO 5.
    }
    if (hasCol) {
        // TODO. 7.
    } else {
        unsigned int span = 1;
        if (html::HTMLTableColElement::hasInstance(colgroup)) {
            html::HTMLTableColElement cg(interface_cast<html::HTMLTableColElement>(colgroup));
            span = cg.getSpan();
        }
        while (0 < span--) {
            appendColumn();
            columnGroups[xWidth - 1] = view->getStyle(colgroup);
        }
        // TODO 3.
    }
}

void TableWrapperBox::growDownwardGrowingCells()
{
}

void TableWrapperBox::fit(float w)
{
    float diff = w - getTotalWidth();
    if (0.0f < diff) {
        unsigned autoMask = Left | Right;
        if (style) {
            if (!style->marginLeft.isAuto())
                autoMask &= ~Left;
            if (!style->marginRight.isAuto())
                autoMask &= ~Right;
        }
        switch (autoMask) {
        case Left | Right:
            diff /= 2.0f;
            marginLeft += diff;
            marginRight += diff;
            break;
        case Left:
            marginLeft += diff;
            break;
        case Right:
            marginRight += diff;
            break;
        default:
            break;
        }
    }
}

bool TableWrapperBox::
BorderValue::resolveBorderConflict(CSSBorderColorValueImp& c, CSSBorderStyleValueImp& s, CSSBorderWidthValueImp& w)
{
    if (s.getValue() == CSSBorderStyleValueImp::None || style.getValue() == CSSBorderStyleValueImp::Hidden)
        return false;
    if (s.getValue() == CSSBorderStyleValueImp::Hidden) {
        style.setValue(CSSBorderStyleValueImp::Hidden);
        width.setValue(0.0f, css::CSSPrimitiveValue::CSS_PX);
        return true;
    }
    if (width < w || width == w && style < s) {
        color.specify(c);
        style.specify(s);
        width.specify(w);
        return true;
    }
    return false;
}

void TableWrapperBox::
BorderValue::resolveBorderConflict(CSSStyleDeclarationPtr s, unsigned trbl)
{
    CSSStyleDeclarationImp* style = s.get();
    if (!style)
        return;
    if (trbl & 0x1)
        resolveBorderConflict(style->borderTopColor, style->borderTopStyle, style->borderTopWidth);
    if (trbl & 0x2)
        resolveBorderConflict(style->borderRightColor, style->borderRightStyle, style->borderRightWidth);
    if (trbl & 0x4)
        resolveBorderConflict(style->borderBottomColor, style->borderBottomStyle, style->borderBottomWidth);
    if (trbl & 0x8)
        resolveBorderConflict(style->borderLeftColor, style->borderLeftStyle, style->borderLeftWidth);
}

void TableWrapperBox::resolveHorizontalBorderConflict(unsigned x, unsigned y, BorderValue* b, CellBox* c, unsigned mask)
{
    if (c) {
        b->resolveBorderConflict(c->getStyle(), mask);
        if (y < yHeight) {
            if (mask & 0x1)
                b->resolveBorderConflict(rows[y], 0x1 & mask);
            else if (0 < y)
                b->resolveBorderConflict(rows[y - 1], 0x4 & mask);
            b->resolveBorderConflict(rowGroups[y], 0x5 & mask);  // TODO: check span
        }
        if (y == 0 || y == yHeight) {
            b->resolveBorderConflict(columns[x], 0x5 & mask);
            b->resolveBorderConflict(columnGroups[x], 0x05 & mask);
            b->resolveBorderConflict(style, 0x05 & mask);
        }
    }
}

void TableWrapperBox::resolveVerticalBorderConflict(unsigned x, unsigned y, BorderValue* b, CellBox* c, unsigned mask)
{
    if (c) {
        b->resolveBorderConflict(c->getStyle(), mask);
        if (x == 0 || x == xWidth) {
            b->resolveBorderConflict(rows[y], 0xa & mask);
            b->resolveBorderConflict(rowGroups[y], 0xa & mask);  // TODO: check span
        }
        if (x < xWidth) {
            if (mask & 0x8)
                b->resolveBorderConflict(columns[x], 0x8 & mask);
            else if (0 < x)
                b->resolveBorderConflict(columns[x - 1], 0x2 & mask);
            b->resolveBorderConflict(columnGroups[x], 0x0a & mask);
        }
        if (x == 0 || x == xWidth)
            b->resolveBorderConflict(style, 0x0a & mask);
    }
}

bool TableWrapperBox::resolveBorderConflict()
{
    assert(style);
    borderRows.clear();
    borderColumns.clear();
    if (style->borderCollapse.getValue() != style->borderCollapse.Collapse)
        return false;
    if (!tableBox)
        return false;
    borderRows.resize((yHeight + 1) * xWidth);
    borderColumns.resize(yHeight * (xWidth + 1));
    for (unsigned y = 0; y < yHeight + 1; ++y) {
        for (unsigned x = 0; x < xWidth + 1; ++x) {
            if (x < xWidth) {
                BorderValue* br = getRowBorderValue(x, y);
                CellBox* top = (y < yHeight) ? grid[y][x].get() : 0;
                CellBox* bottom = (0 < y) ? grid[y - 1][x].get() : 0;
                if (top != bottom) {
                    resolveHorizontalBorderConflict(x, y, br, top, 0x1);
                    resolveHorizontalBorderConflict(x, y, br, bottom, 0x4);
                }
            }
            if (y < yHeight) {
                BorderValue* bc = getColumnBorderValue(x, y);
                CellBox* left = (x < xWidth) ? grid[y][x].get() : 0;
                CellBox* right = (0 < x) ? grid[y][x - 1].get() : 0;
                if (left != right) {
                    resolveVerticalBorderConflict(x, y, bc, left, 0x8);
                    resolveVerticalBorderConflict(x, y, bc, right, 0x2);
                }
            }
        }
    }
    return true;
}

void TableWrapperBox::layOutFixed(ViewCSSImp* view, const ContainingBlock* containingBlock, bool collapsingModel)
{
    float spacing = 0.0f;
    if (!collapsingModel) {
        spacing = style->borderSpacing.getHorizontalSpacing();
        tableBox->width -= tableBox->getBorderWidth() - tableBox->width;  // TODO: HTML, XHTML only
        if (tableBox->width < 0.0f)
            tableBox->width = 0.0f;
    }
    float sum = spacing * (xWidth + 1);
    unsigned remainingColumns = xWidth;
    for (unsigned x = 0; x < xWidth; ++x) {
        if (CSSStyleDeclarationPtr colStyle = columns[x]) {
            if (!colStyle->width.isAuto()) {
                colStyle->resolve(view, containingBlock);
                widths[x] = colStyle->borderLeftWidth.getPx() +
                            colStyle->paddingLeft.getPx() +
                            colStyle->width.getPx() +
                            colStyle->paddingRight.getPx() +
                            colStyle->borderRightWidth.getPx();
                sum += widths[x];
                --remainingColumns;
                continue;
            }
        }
        CellBox* cellBox = grid[0][x].get();
        if (!cellBox || cellBox->isSpanned(x, 0))
            continue;
        CSSStyleDeclarationImp* cellStyle = cellBox->getStyle();
        if (!cellStyle || cellStyle->width.isAuto())
            continue;
        cellStyle->resolve(view, containingBlock);
        unsigned span = cellBox->getColSpan();
        float w = cellStyle->borderLeftWidth.getPx() +
                  cellStyle->paddingLeft.getPx() +
                  cellStyle->width.getPx() +
                  cellStyle->paddingRight.getPx() +
                  cellStyle->borderRightWidth.getPx();
        w = std::max(0.0f, w - spacing * (span - 1)) / span;
        for (unsigned i = x; i < x + span; ++i) {
            widths[i] = w;
            sum += w;
            --remainingColumns;
        }
    }
    if (0 < remainingColumns) {
        float w = std::max(0.0f, width - sum) / remainingColumns;
        for (unsigned x = 0; x < xWidth; ++x) {
            if (isnan(widths[x])) {
                widths[x] = w;
                sum += w;
            }
        }
    }
    if (width <= sum)
        width = sum;
    else {
        float w = (width - sum) / xWidth;
        for (unsigned x = 0; x < xWidth; ++x)
            widths[x] += w;
    }
}

bool TableWrapperBox::layOut(ViewCSSImp* view, FormattingContext* context)
{
    const ContainingBlock* containingBlock = getContainingBlock(view);
    style = view->getStyle(table);
    if (!style)
        return false;  // TODO error

    bool collapsingModel = resolveBorderConflict();
    bool fixedLayout = (style->tableLayout.getValue() == CSSTableLayoutValueImp::Fixed) && !style->width.isAuto();
    style->resolve(view, containingBlock);

    // The computed values of properties 'position', 'float', 'margin-*', 'top', 'right', 'bottom',
    // and 'left' on the table element are used on the table wrapper box and not the table box;
    backgroundColor = 0x00000000;
    paddingTop = paddingRight = paddingBottom = paddingLeft = 0.0f;
    borderTop = borderRight = borderBottom = borderLeft = 0.0f;
    resolveMargin(view, containingBlock, 0.0f);
    stackingContext = style->getStackingContext();

    context = updateFormattingContext(context);

    if (tableBox) {
        float spacing = 0.0f;
        tableBox->setStyle(style.get());
        tableBox->resolveBackground(view);
        tableBox->updatePadding();
         if (!collapsingModel) {
            tableBox->updateBorderWidth();
            spacing = style->borderSpacing.getHorizontalSpacing();
         }
        tableBox->resolveMargin(view, containingBlock, 0.0f);
        tableBox->width = width;
        tableBox->height = height;
        widths.resize(xWidth);
        heights.resize(yHeight);
        for (unsigned x = 0; x < xWidth; ++x)
            widths[x] = fixedLayout ? NAN : 0.0f;
        for (unsigned y = 0; y < yHeight; ++y)
            heights[y] = 0.0f;
        if (fixedLayout)
            layOutFixed(view, containingBlock, collapsingModel);
        float tableWidth = width;
        for (unsigned y = 0; y < yHeight; ++y) {
            for (unsigned x = 0; x < xWidth; ++x) {
                CellBox* cellBox = grid[y][x].get();
                if (!cellBox || cellBox->isSpanned(x, y))
                    continue;
                if (fixedLayout) {
                    tableBox->width = widths[x];
                    for (unsigned i = x + 1; i < x + cellBox->getColSpan(); ++i)
                        tableBox->width += spacing + widths[i];
                    cellBox->fixedLayout = true;
                }
                cellBox->layOut(view, context);
                if (collapsingModel)
                    cellBox->collapseBorder(this);
                else
                    cellBox->separatedBorders(style, xWidth, yHeight);
                if (!fixedLayout && cellBox->getColSpan() == 1)
                    widths[x] = std::max(widths[x], cellBox->getTotalWidth());
                if (cellBox->getRowSpan() == 1)
                    heights[y] = std::max(heights[y], cellBox->getTotalHeight());
            }
        }
        if (fixedLayout)
            tableBox->width = tableWidth;
        for (unsigned x = 0; x < xWidth; ++x) {
            for (unsigned y = 0; y < yHeight; ++y) {
                CellBox* cellBox = grid[y][x].get();
                if (!cellBox || cellBox->isSpanned(x, y))
                    continue;
                if (!fixedLayout) {
                    unsigned span = cellBox->getColSpan();
                    if (1 < span) {
                        float sum = spacing * (span - 1);
                        for (unsigned c = 0; c < span; ++c)
                            sum += widths[x + c];
                        if (sum < cellBox->getTotalWidth()) {
                            float diff = (cellBox->getTotalWidth() - sum) / span;
                            for (unsigned c = 0; c < span; ++c)
                                widths[x + c] += diff;
                        }
                    }
                }
                unsigned span = cellBox->getRowSpan();
                if (1 < span) {
                    float sum = spacing * (span - 1);
                    for (unsigned r = 0; r < span; ++r)
                        sum += heights[y + r];
                    if (sum < cellBox->getTotalHeight()) {
                        float diff = (cellBox->getTotalHeight() - sum) / span;
                        for (unsigned r = 0; r < span; ++r)
                            heights[y + r] += diff;
                    }
                }
            }
        }

        if (!fixedLayout) {
            float w = spacing * (xWidth - 1);
            for (unsigned x = 0; x < xWidth; ++x)
                w += widths[x];
            if (style->width.isAuto())
                tableBox->width = w;
            else if (w < tableBox->width) {
                w = (tableBox->width - w) / xWidth;
                for (unsigned x = 0; x < xWidth; ++x)
                    widths[x] += w;
            }
        }
        float h = spacing * (yHeight - 1);
        for (unsigned y = 0; y < yHeight; ++y)
            h += heights[y];
        if (style->height.isAuto())
            tableBox->height = h;
        else if (h < tableBox->height) {
            h = (tableBox->height- h) / yHeight;
            for (unsigned y = 0; y < yHeight; ++y)
                heights[y] += h;
        }

        for (unsigned x = 0; x < xWidth; ++x) {
            for (unsigned y = 0; y < yHeight; ++y) {
                CellBox* cellBox = grid[y][x].get();
                if (!cellBox || cellBox->isSpanned(x, y))
                    continue;
                if (!fixedLayout) {
                    float w = widths[x];
                    for (unsigned c = 1; c < cellBox->getColSpan(); ++c)
                        w += spacing + widths[x + c];
                    cellBox->fit(w);
                }
                float h = heights[y];
                for (unsigned r = 1; r < cellBox->getRowSpan(); ++r)
                    h += spacing + heights[y + r];
                cellBox->height = h - cellBox->getBlankTop() - cellBox->getBlankBottom();
            }
        }

        Box* lineBox = tableBox->getFirstChild();
        for (unsigned y = 0; y < yHeight; ++y)  {
            assert(lineBox);
            lineBox->width = tableBox->width;
            lineBox->height = heights[y];
            lineBox = lineBox->getNextSibling();

            float xOffset = 0.0f;
            for (unsigned x = 0; x < xWidth; ++x) {
                CellBox* cellBox = grid[y][x].get();
                if (!cellBox || cellBox->isSpanned(x, y) && cellBox->row != y) {
                    xOffset += widths[x];
                    continue;
                }
                cellBox->offsetH += xOffset;
            }
        }
    }

    for (Box* child = getFirstChild(); child; child = child->getNextSibling()) {
        if (child != tableBox)
            child->layOut(view, context);
    }

    width = 0.0f;
    for (Box* child = getFirstChild(); child; child = child->getNextSibling())
        width = std::max(width, child->getTotalWidth());
    height = 0.0f;
    for (Box* child = getFirstChild(); child; child = child->getNextSibling())
        height += child->getTotalHeight() + child->getClearance();

    style->width.setValue();

    collapseMarginBottom(context);
    if (!isAnonymous()) {
        width = std::max(width, style->minWidth.getPx());
        height = std::max(height, style->minHeight.getPx());
    }

    adjustCollapsedThroughMargins(context);

    return true;
}

float TableWrapperBox::shrinkTo()
{
    return getTotalWidth();
}

void TableWrapperBox::dump(std::string indent)
{
    std::cout << indent << "* table wrapper box";
    std::cout << " [" << node.getNodeName() << ']';
    std::cout << " (" << x << ", " << y << "), (" << getTotalWidth() << ", " << getTotalHeight() << ") " <<
                 "[" << xWidth << ", " << yHeight << "]\n";

    indent += "    ";
    for (Box* child = getFirstChild(); child; child = child->getNextSibling())
        child->dump(indent);
}

bool BlockLevelBox::isTableBox() const
{
    if (TableWrapperBox* wrapper = dynamic_cast<TableWrapperBox*>(getParentBox())) {
        if (wrapper->isTableBox(this))
            return true;
    }
    return false;
}

}}}}  // org::w3c::dom::bootstrap
