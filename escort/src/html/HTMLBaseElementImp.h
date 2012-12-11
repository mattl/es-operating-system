/*
 * Copyright 2010, 2011 Esrille Inc.
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

#ifndef HTMLBASEELEMENT_IMP_H
#define HTMLBASEELEMENT_IMP_H

#include <Object.h>
#include <org/w3c/dom/html/HTMLBaseElement.h>

#include "HTMLElementImp.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

class HTMLBaseElementImp : public ObjectMixin<HTMLBaseElementImp, HTMLElementImp>
{
public:
    // Node
    virtual Node cloneNode(bool deep);

    // HTMLBaseElement
    virtual std::u16string getHref();
    virtual void setHref(const std::u16string& href);
    virtual std::u16string getTarget();
    virtual void setTarget(const std::u16string& target);

    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv) {
        return html::HTMLBaseElement::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return html::HTMLBaseElement::getMetaData();
    }

    HTMLBaseElementImp(DocumentImp* ownerDocument) :
        ObjectMixin(ownerDocument, u"base") {
    }
    HTMLBaseElementImp(HTMLBaseElementImp* org, bool deep) :
        ObjectMixin(org, deep) {
    }
};

}}}}  // org::w3c::dom::bootstrap

#endif  // HTMLBASEELEMENT_IMP_H
