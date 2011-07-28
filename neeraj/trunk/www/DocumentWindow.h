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

#ifndef DOCUMENT_WINDOW_H
#define DOCUMENT_WINDOW_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/Document.h>

#include <boost/intrusive_ptr.hpp>

#include "EventTargetImp.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

// DocumentWindow implements the Window object
class DocumentWindow : public EventTargetImp
{
    Document document;

public:
    DocumentWindow() :
        document(0)
    {
    }
    DocumentWindow(const Document& document) :
        document(document)
    {
    }
    DocumentWindow(const DocumentWindow& window) :
        document(window.document)
    {
    }

    Document getDocument() const {
        return document;
    }
    void setDocument(const Document& document) {
        this->document = document;
    }
};

typedef boost::intrusive_ptr<DocumentWindow> DocumentWindowPtr;

}}}}  // org::w3c::dom::bootstrap

#endif  // DOCUMENT_WINDOW_H
