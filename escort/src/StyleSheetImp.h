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

#ifndef STYLESHEET_IMP_H
#define STYLESHEET_IMP_H

#include <Object.h>
#include <org/w3c/dom/stylesheets/StyleSheet.h>

#include <org/w3c/dom/stylesheets/MediaList.h>
#include <org/w3c/dom/Node.h>

namespace org { namespace w3c { namespace dom { namespace bootstrap {

class StyleSheetImp : public ObjectMixin<StyleSheetImp>
{
    std::u16string href;
    Node owner;
    stylesheets::StyleSheet parent;
    bool disabled;

public:
    StyleSheetImp();

    void setHref(const std::u16string& location);
    void setOwnerNode(Node node);
    void setParentStyleSheet(stylesheets::StyleSheet sheet);

    // StyleSheet
    virtual std::u16string getType();
    std::u16string getHref();
    Node getOwnerNode();
    stylesheets::StyleSheet getParentStyleSheet();
    virtual std::u16string getTitle();
    virtual stylesheets::MediaList getMedia();
    virtual void setMedia(const std::u16string& media);
    bool getDisabled();
    void setDisabled(bool disabled);
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return stylesheets::StyleSheet::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return stylesheets::StyleSheet::getMetaData();
    }
};

}}}}  // org::w3c::dom::bootstrap

#endif  // STYLESHEET_IMP_H
