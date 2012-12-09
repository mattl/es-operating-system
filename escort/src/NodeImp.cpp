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

#include "NodeImp.h"
#include "DocumentImp.h"
#include "MutationEventImp.h"
#include "ElementImp.h"
#include "NodeListImp.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

//
// Tree management
//

NodeImp* NodeImp::removeChild(NodeImp* item)
{
    NodeImp* next = item->nextSibling;
    NodeImp* prev = item->previousSibling;
    if (!next)
        lastChild = prev;
    else
        next->previousSibling = prev;
    if (!prev)
        firstChild = next;
    else
        prev->nextSibling = next;
    item->parentNode = item->previousSibling = item->nextSibling = 0;
    --childCount;
    return item;
}

NodeImp* NodeImp::insertBefore(NodeImp* item, NodeImp* after)
{
    item->previousSibling = after->previousSibling;
    item->nextSibling = after;
    after->previousSibling = item;
    if (!item->previousSibling)
        firstChild = item;
    else
        item->previousSibling->nextSibling = item;
    item->parentNode = this;
    ++childCount;
    return item;
}

NodeImp* NodeImp::appendChild(NodeImp* item)
{
    NodeImp* prev = lastChild;
    if (!prev)
        firstChild = item;
    else
        prev->nextSibling = item;
    item->previousSibling = prev;
    item->nextSibling = 0;
    lastChild = item;
    item->parentNode = this;
    ++childCount;
    return item;
}

void NodeImp::setOwnerDocument(DocumentImp* document)
{
    assert(!ownerDocument);
    ownerDocument = document;
}

// Node
unsigned short NodeImp::getNodeType()
{
    // SHOULD NOT REACH HERE
    return 0;
}

std::u16string NodeImp::getNodeName()
{
    return nodeName;
}

Nullable<std::u16string> NodeImp::getBaseURI()
{
    if (!ownerDocument)
        return Nullable<std::u16string>();
    return ownerDocument->getDocumentURI();
}

Document NodeImp::getOwnerDocument()
{
    return ownerDocument;
}

Node NodeImp::getParentNode()
{
    return parentNode;
}

Element NodeImp::getParentElement()
{
    return dynamic_cast<ElementImp*>(parentNode);
}

bool NodeImp::hasChildNodes()
{
    return firstChild;
}

NodeList NodeImp::getChildNodes()
{
    NodeListImp* nodeList = new(std::nothrow) NodeListImp;
    if (nodeList) {
        for (NodeImp* node = firstChild; node; node = node->nextSibling)
            nodeList->addItem(node);
    }
    return nodeList;
}

Node NodeImp::getFirstChild()
{
    return firstChild;
}

Node NodeImp::getLastChild()
{
    return lastChild;
}

Node NodeImp::getPreviousSibling()
{
    return previousSibling;
}

Node NodeImp::getNextSibling()
{
    return nextSibling;
}

unsigned short NodeImp::compareDocumentPosition(Node other)
{
    // TODO: implement me!
    return 0;
}

Nullable<std::u16string> NodeImp::getNodeValue()
{
    return Nullable<std::u16string>();
}

void NodeImp::setNodeValue(Nullable<std::u16string> nodeValue)
{
}

Nullable<std::u16string> NodeImp::getTextContent()
{
    return Nullable<std::u16string>();
}

void NodeImp::setTextContent(Nullable<std::u16string> textContent)
{
}

Node NodeImp::insertBefore(Node newChild, Node refChild)
{
    if (!newChild)
        return newChild;
    if (!refChild)
        return appendChild(newChild);
    if (newChild != refChild) {
        Document ownerOfChild = newChild.getOwnerDocument();
        if (ownerOfChild != getOwnerDocument() && ownerOfChild != *this)
            throw DOMException{DOMException::WRONG_DOCUMENT_ERR};
        if (refChild.getParentNode() != *this)
            throw DOMException{DOMException::NOT_FOUND_ERR};
        if (NodeImp* child = dynamic_cast<NodeImp*>(newChild.self())) {
            if (child == this || child->isAncestorOf(this))
                throw DOMException{DOMException::HIERARCHY_REQUEST_ERR};
            if (NodeImp* ref = dynamic_cast<NodeImp*>(refChild.self())) {
                child->retain_();
                if (child->parentNode) {
                    child->parentNode->removeChild(child);
                    child->release_();
                }
                insertBefore(child, ref);

                events::MutationEvent event = new(std::nothrow) MutationEventImp;
                event.initMutationEvent(u"DOMNodeInserted",
                                        true, false, this, u"", u"", u"", 0);
                child->dispatchEvent(event);
            }
        }
    }
    return newChild;
}

Node NodeImp::replaceChild(Node newChild, Node oldChild)
{
    if (!newChild)
        return oldChild;
    if (!oldChild || oldChild.getParentNode() != *this)
        throw DOMException{DOMException::NOT_FOUND_ERR};
    if (newChild != oldChild) {
        Document ownerOfChild = newChild.getOwnerDocument();
        if (ownerOfChild != getOwnerDocument() && ownerOfChild != *this)
            throw DOMException{DOMException::WRONG_DOCUMENT_ERR};
        if (NodeImp* child = dynamic_cast<NodeImp*>(newChild.self())) {
            if (child == this || child->isAncestorOf(this))
                throw DOMException{DOMException::HIERARCHY_REQUEST_ERR};
            if (NodeImp* ref = dynamic_cast<NodeImp*>(oldChild.self())) {
                child->retain_();
                if (child->parentNode) {
                    child->parentNode->removeChild(child);
                    child->release_();
                }
                insertBefore(child, ref);
                removeChild(ref);
                ref->release_();
            }
        }
    }
    return oldChild;
}

Node NodeImp::removeChild(Node oldChild)
{
    if (!oldChild)
        throw DOMException{DOMException::NOT_FOUND_ERR};
    if (NodeImp* child = dynamic_cast<NodeImp*>(oldChild.self())) {
        if (child->parentNode != this)
            throw DOMException{DOMException::NOT_FOUND_ERR};
        if (0 < count_()) {  // Prevent dispatching an event from the destructor.
            events::MutationEvent event = new(std::nothrow) MutationEventImp;
            event.initMutationEvent(u"DOMNodeRemoved",
                                    true, false, this, u"", u"", u"", 0);
            child->dispatchEvent(event);
        }
        removeChild(child);
        child->release_();
    }
    return oldChild;
}

Node NodeImp::appendChild(Node newChild, bool clone)
{
    if (!newChild)
        return newChild;
    Document ownerOfChild = newChild.getOwnerDocument();
    if (ownerOfChild != getOwnerDocument() && ownerOfChild != *this)
        throw DOMException{DOMException::WRONG_DOCUMENT_ERR};
    if (NodeImp* child = dynamic_cast<NodeImp*>(newChild.self())) {
        if (child == this || child->isAncestorOf(this))
            throw DOMException{DOMException::HIERARCHY_REQUEST_ERR};
        // TODO: case newChild is a DocumentFragment
        child->retain_();
        if (child->parentNode) {
            child->parentNode->removeChild(child);
            child->release_();
        }
        appendChild(child);
        if (!clone) {
            events::MutationEvent event = new(std::nothrow) MutationEventImp;
            event.initMutationEvent(u"DOMNodeInserted",
                                    true, false, this, u"", u"", u"", 0);
            child->dispatchEvent(event);
        }
    }  // TODO: else ...
    return newChild;
}

Node NodeImp::cloneNode(bool deep)
{
    return new(std::nothrow) NodeImp(this, deep);
}

bool NodeImp::isSameNode(Node other)
{
    NodeImp* node = dynamic_cast<NodeImp*>(other.self());
    return this == node;
}

bool NodeImp::isEqualNode(Node arg)
{
    NodeImp* node = dynamic_cast<NodeImp*>(arg.self());
    if (this == node)
        return true;
    if (!node)
        return false;
    if (getNodeType() != node->getNodeType())
        return false;
    if (getChildCount() != node->getChildCount())
        return false;
    NodeImp* x;
    NodeImp* y;
    for (x = firstChild, y = node->firstChild; x; x = x->nextSibling, y = y->nextSibling) {
        if (!y || !x->isEqualNode(y))
            return false;
    }
    return y ? false : true;
}

std::u16string NodeImp::lookupPrefix(std::u16string namespaceURI)
{
    // TODO: implement me!
    return u"";
}

std::u16string NodeImp::lookupNamespaceURI(Nullable<std::u16string> prefix)
{
    // TODO: implement me!
    return u"";
}

bool NodeImp::isDefaultNamespace(std::u16string namespaceURI)
{
    // TODO: implement me!
    return 0;
}

NodeImp::NodeImp(DocumentImp* ownerDocument) :
    ownerDocument(ownerDocument),
    parentNode(0),
    firstChild(0),
    lastChild(0),
    previousSibling(0),
    nextSibling(0),
    childCount(0)
{
}

NodeImp::NodeImp(NodeImp* org, bool deep) :
    ObjectMixin(org),
    ownerDocument(0),
    parentNode(0),
    firstChild(0),
    lastChild(0),
    previousSibling(0),
    nextSibling(0),
    childCount(0),
    nodeName(org->nodeName)
{
    setOwnerDocument(org->ownerDocument);
    if (!deep)
        return;
    for (NodeImp* node = org->firstChild; node; node = node->nextSibling) {
        Node clone = node->cloneNode(true);
        appendChild(clone, true);
    }
}

NodeImp::~NodeImp()
{
    assert(0 == count_());
    while (0 < childCount)
        removeChild(getFirstChild());
}

}}}}  // org::w3c::dom::bootstrap
