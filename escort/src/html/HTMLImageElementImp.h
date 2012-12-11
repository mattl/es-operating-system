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

#ifndef HTMLIMAGEELEMENT_IMP_H
#define HTMLIMAGEELEMENT_IMP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/html/HTMLImageElement.h>
#include "HTMLReplacedElementImp.h"

#include "http/HTTPRequest.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

class BoxImage;

class HTMLImageElementImp : public ObjectMixin<HTMLImageElementImp, HTMLReplacedElementImp>
{
public:
    HTMLImageElementImp(DocumentImp* ownerDocument);
    HTMLImageElementImp(HTMLImageElementImp* org, bool deep);

    virtual void eval();
    void notify();

    // TODO: Refine this interface as this is only for CSS
    bool getIntrinsicSize(float& w, float& h);
    BoxImage* getImage() const {
        return image;
    }

    // Node
    virtual Node cloneNode(bool deep);

    // HTMLImageElement
    std::u16string getAlt();
    void setAlt(const std::u16string& alt);
    std::u16string getSrc();
    void setSrc(const std::u16string& src);
    std::u16string getCrossOrigin();
    void setCrossOrigin(const std::u16string& crossOrigin);
    std::u16string getUseMap();
    void setUseMap(const std::u16string& useMap);
    bool getIsMap();
    void setIsMap(bool isMap);
    unsigned int getWidth();
    void setWidth(unsigned int width);
    unsigned int getHeight();
    void setHeight(unsigned int height);
    unsigned int getNaturalWidth();
    unsigned int getNaturalHeight();
    bool getComplete();
    // HTMLImageElement-20
    std::u16string getName();
    void setName(const std::u16string& name);
    std::u16string getAlign();
    void setAlign(const std::u16string& align);
    std::u16string getBorder();
    void setBorder(const std::u16string& border);
    unsigned int getHspace();
    void setHspace(unsigned int hspace);
    std::u16string getLongDesc();
    void setLongDesc(const std::u16string& longDesc);
    unsigned int getVspace();
    void setVspace(unsigned int vspace);
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return html::HTMLImageElement::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return html::HTMLImageElement::getMetaData();
    }
};

}}}}  // org::w3c::dom::bootstrap

#endif  // HTMLIMAGEELEMENT_IMP_H
