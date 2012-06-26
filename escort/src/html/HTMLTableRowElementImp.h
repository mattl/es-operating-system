/*
 * Copyright 2012 Esrille Inc.
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

#ifndef ORG_W3C_DOM_BOOTSTRAP_HTMLTABLEROWELEMENTIMP_H_INCLUDED
#define ORG_W3C_DOM_BOOTSTRAP_HTMLTABLEROWELEMENTIMP_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/html/HTMLTableRowElement.h>
#include "HTMLElementImp.h"

#include <org/w3c/dom/html/HTMLElement.h>
#include <org/w3c/dom/html/HTMLCollection.h>

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{
class HTMLTableRowElementImp : public ObjectMixin<HTMLTableRowElementImp, HTMLElementImp>
{
public:
    void eval();

    // HTMLTableRowElement
    int getRowIndex();
    int getSectionRowIndex();
    html::HTMLCollection getCells();
    html::HTMLElement insertCell();
    html::HTMLElement insertCell(int index);
    void deleteCell(int index);
    // HTMLTableRowElement-33
    std::u16string getAlign();
    void setAlign(std::u16string align);
    std::u16string getBgColor();
    void setBgColor(std::u16string bgColor);
    std::u16string getCh();
    void setCh(std::u16string ch);
    std::u16string getChOff();
    void setChOff(std::u16string chOff);
    std::u16string getVAlign();
    void setVAlign(std::u16string vAlign);
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return html::HTMLTableRowElement::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return html::HTMLTableRowElement::getMetaData();
    }
    HTMLTableRowElementImp(DocumentImp* ownerDocument) :
        ObjectMixin(ownerDocument, u"tr") {
    }
    HTMLTableRowElementImp(HTMLTableRowElementImp* org, bool deep) :
        ObjectMixin(org, deep) {
    }
};

}
}
}
}

#endif  // ORG_W3C_DOM_BOOTSTRAP_HTMLTABLEROWELEMENTIMP_H_INCLUDED
