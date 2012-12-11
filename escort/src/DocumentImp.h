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

#ifndef DOCUMENT_IMP_H
#define DOCUMENT_IMP_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/Document.h>
#include "NodeImp.h"

#include <org/w3c/dom/stylesheets/StyleSheet.h>
#include <org/w3c/dom/css/CSSStyleDeclaration.h>
#include <org/w3c/dom/html/HTMLElement.h>
#include <org/w3c/dom/CaretPosition.h>
#include <org/w3c/dom/ranges/Range.h>
#include <org/w3c/dom/traversal/NodeIterator.h>
#include <org/w3c/dom/traversal/NodeFilter.h>
#include <org/w3c/dom/traversal/TreeWalker.h>
#include <org/w3c/dom/DOMException.h>
#include <org/w3c/dom/events/Event.h>
#include <org/w3c/dom/Node.h>
#include <org/w3c/dom/DocumentFragment.h>
#include <org/w3c/dom/DOMImplementation.h>
#include <org/w3c/dom/Element.h>
#include <org/w3c/dom/DocumentType.h>
#include <org/w3c/dom/ProcessingInstruction.h>
#include <org/w3c/dom/Text.h>
#include <org/w3c/dom/Comment.h>
#include <org/w3c/dom/NodeList.h>
#include <org/w3c/dom/html/HTMLCollection.h>
#include <org/w3c/dom/DOMStringList.h>
#include <org/w3c/dom/DOMElementMap.h>
#include <org/w3c/dom/html/HTMLAllCollection.h>
#include <org/w3c/dom/html/HTMLDocument.h>
#include <org/w3c/dom/html/HTMLHeadElement.h>
#include <org/w3c/dom/html/HTMLScriptElement.h>
#include <org/w3c/dom/html/Window.h>
#include <org/w3c/dom/html/Location.h>
#include <org/w3c/dom/html/Function.h>

#include <deque>
#include <list>

#include "NodeImp.h"
#include "DocumentWindow.h"
#include "EventListenerImp.h"
#include "WindowImp.h"
#include "html/HTMLScriptElementImp.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

class DocumentImp : public ObjectMixin<DocumentImp, NodeImp>
{
    std::u16string url;
    DocumentType doctype;
    int mode;
    std::u16string charset;
    std::u16string readyState;
    std::u16string compatMode;
    std::deque<stylesheets::StyleSheet> styleSheets;
    unsigned loadEventDelayCount;
    bool contentLoaded;

    HTMLScriptElementImp* pendingParsingBlockingScript;
    std::list<html::HTMLScriptElement> deferScripts;
    std::list<html::HTMLScriptElement> asyncScripts;
    std::list<html::HTMLScriptElement> orderedScripts;

    WindowImp* defaultView;
    ElementImp* activeElement;
    int error;

    // XBL 2.0
    std::map<const std::u16string, html::Window> bindingDocuments;

    bool processScripts(std::list<html::HTMLScriptElement>& scripts);

public:
    DocumentImp(const std::u16string& url = u"about:blank");
    ~DocumentImp();

    enum
    {
         NoQuirksMode,  // default
         QuirksMode,
         LimitedQuirksMode
    };

    WindowImp* getDefaultWindow() const {
        return defaultView;
    }
    void setDefaultView(WindowImp* view);

    int getMode() const {
        return mode;
    }
    void setMode(int value) {
        mode = value;
    }

    void addDeferScript(HTMLScriptElementImp* script) {
        deferScripts.push_back(script);
    }
    void addAsyncScript(HTMLScriptElementImp* script) {
        asyncScripts.push_back(script);
    }
    void addOrderedScript(HTMLScriptElementImp* script) {
        orderedScripts.push_back(script);
    }
    void removeDeferScript(HTMLScriptElementImp* script) {
        for (auto i = deferScripts.begin(); i != deferScripts.end(); ++i) {
            if (i->self() == script) {
                deferScripts.erase(i);
                break;
            }
        }
    }
    void removeAsyncScript(HTMLScriptElementImp* script) {
        for (auto i = asyncScripts.begin(); i != asyncScripts.end(); ++i) {
            if (i->self() == script) {
                asyncScripts.erase(i);
                break;
            }
        }
    }
    void removeOrderedScript(HTMLScriptElementImp* script) {
        for (auto i = orderedScripts.begin(); i != orderedScripts.end(); ++i) {
            if (i->self() == script) {
                orderedScripts.erase(i);
                break;
            }
        }
    }
    bool processDeferScripts() {
        return processScripts(deferScripts);
    }
    bool hasAsyncScripts() {
        return !asyncScripts.empty();
    }
    bool processOrderedScripts() {
        return processScripts(orderedScripts);
    }

    bool hasContentLoaded() const {
        return contentLoaded;
    }
    void setContentLoaded() {
        contentLoaded = true;
    }

    int getError() const {
        return error;
    }
    void setError(int value) {
        error = value;
    }

    HTMLScriptElementImp* getPendingParsingBlockingScript() const {
        return pendingParsingBlockingScript;
    }
    void setPendingParsingBlockingScript(HTMLScriptElementImp* element) {
        pendingParsingBlockingScript = element;
    }

    void setReadyState(const std::u16string& readyState);

    void clearStyleSheets() {
        styleSheets.clear();
    }
    void addStyleSheet(stylesheets::StyleSheet sheet) {
        if (sheet)
            styleSheets.push_back(sheet);
    }
    void resetStyleSheets();

    void setURL(const std::u16string& url);

    ElementImp* getFocus() const {
        return activeElement;
    }
    void setFocus(ElementImp* element);

    DocumentWindowPtr activate();

    unsigned incrementLoadEventDelayCount() {
        return ++loadEventDelayCount;
    }
    unsigned decrementLoadEventDelayCount();

    bool isBindingDocumentWindow(const WindowImp* window) const;

    // Node - override
    virtual unsigned short getNodeType();
    virtual Node appendChild(Node newChild) throw(DOMException);

    // Document
    DOMImplementation getImplementation();
    std::u16string getDocumentURI();
    void setDocumentURI(const std::u16string& documentURI);
    std::u16string getCompatMode();
    std::u16string getCharset();
    void setCharset(const std::u16string& charset);
    std::u16string getCharacterSet();
    std::u16string getDefaultCharset();
    std::u16string getContentType();
    DocumentType getDoctype();
    Element getDocumentElement();
    NodeList getElementsByTagName(const std::u16string& qualifiedName);
    NodeList getElementsByTagNameNS(const std::u16string& _namespace, const std::u16string& localName);
    NodeList getElementsByClassName(const std::u16string& classNames);
    Element getElementById(const std::u16string& elementId);
    Element createElement(const std::u16string& localName);
    Element createElementNS(const std::u16string& _namespace, const std::u16string& qualifiedName);
    DocumentFragment createDocumentFragment();
    Text createTextNode(const std::u16string& data);
    Comment createComment(const std::u16string& data);
    ProcessingInstruction createProcessingInstruction(const std::u16string& target, const std::u16string& data);
    Node importNode(Node node, bool deep);
    Node adoptNode(Node node);
    events::Event createEvent(const std::u16string& eventInterfaceName);
    // DocumentCSS
    css::CSSStyleDeclaration getOverrideStyle(Element elt, Nullable<std::u16string> pseudoElt);
    // Document-1
    stylesheets::StyleSheetList getStyleSheets();
    Nullable<std::u16string> getSelectedStyleSheetSet();
    void setSelectedStyleSheetSet(Nullable<std::u16string> selectedStyleSheetSet);
    Nullable<std::u16string> getLastStyleSheetSet();
    Nullable<std::u16string> getPreferredStyleSheetSet();
    DOMStringList getStyleSheetSets();
    void enableStyleSheetsForSet(Nullable<std::u16string> name);
    // Document-2
    Element elementFromPoint(float x, float y);
    CaretPosition caretPositionFromPoint(float x, float y);
    // NodeSelector
    Element querySelector(const std::u16string& selectors);
    NodeList querySelectorAll(const std::u16string& selectors);
    // HTMLDocument
    html::Location getLocation();
    void setLocation(const std::u16string& location);
    std::u16string getURL();
    std::u16string getDomain();
    void setDomain(const std::u16string& domain);
    std::u16string getReferrer();
    std::u16string getCookie();
    void setCookie(const std::u16string& cookie);
    std::u16string getLastModified();
    std::u16string getReadyState();
    Any getElement(const std::u16string& name);
    std::u16string getTitle();
    void setTitle(const std::u16string& title);
    std::u16string getDir();
    void setDir(const std::u16string& dir);
    html::HTMLElement getBody();
    void setBody(html::HTMLElement body);
    html::HTMLHeadElement getHead();
    html::HTMLCollection getImages();
    html::HTMLCollection getEmbeds();
    html::HTMLCollection getPlugins();
    html::HTMLCollection getLinks();
    html::HTMLCollection getForms();
    html::HTMLCollection getScripts();
    NodeList getElementsByName(const std::u16string& elementName);
    DOMElementMap getCssElementMap();
    std::u16string getInnerHTML();
    void setInnerHTML(const std::u16string& innerHTML);
    html::HTMLDocument open(const std::u16string& type = u"text/html", const std::u16string& replace = u"");
    html::Window open(const std::u16string& url, const std::u16string& name, const std::u16string& features, bool replace = false);
    void close();
    void write(Variadic<std::u16string> text = Variadic<std::u16string>());
    void writeln(Variadic<std::u16string> text = Variadic<std::u16string>());
    html::Window getDefaultView();
    Element getActiveElement();
    bool hasFocus();
    std::u16string getDesignMode();
    void setDesignMode(const std::u16string& designMode);
    bool execCommand(const std::u16string& commandId);
    bool execCommand(const std::u16string& commandId, bool showUI);
    bool execCommand(const std::u16string& commandId, bool showUI, const std::u16string& value);
    bool queryCommandEnabled(const std::u16string& commandId);
    bool queryCommandIndeterm(const std::u16string& commandId);
    bool queryCommandState(const std::u16string& commandId);
    bool queryCommandSupported(const std::u16string& commandId);
    std::u16string queryCommandValue(const std::u16string& commandId);
    html::HTMLCollection getCommands();
    html::Function getOnabort();
    void setOnabort(html::Function onabort);
    html::Function getOnblur();
    void setOnblur(html::Function onblur);
    html::Function getOncanplay();
    void setOncanplay(html::Function oncanplay);
    html::Function getOncanplaythrough();
    void setOncanplaythrough(html::Function oncanplaythrough);
    html::Function getOnchange();
    void setOnchange(html::Function onchange);
    html::Function getOnclick();
    void setOnclick(html::Function onclick);
    html::Function getOncontextmenu();
    void setOncontextmenu(html::Function oncontextmenu);
    html::Function getOncuechange();
    void setOncuechange(html::Function oncuechange);
    html::Function getOndblclick();
    void setOndblclick(html::Function ondblclick);
    html::Function getOndrag();
    void setOndrag(html::Function ondrag);
    html::Function getOndragend();
    void setOndragend(html::Function ondragend);
    html::Function getOndragenter();
    void setOndragenter(html::Function ondragenter);
    html::Function getOndragleave();
    void setOndragleave(html::Function ondragleave);
    html::Function getOndragover();
    void setOndragover(html::Function ondragover);
    html::Function getOndragstart();
    void setOndragstart(html::Function ondragstart);
    html::Function getOndrop();
    void setOndrop(html::Function ondrop);
    html::Function getOndurationchange();
    void setOndurationchange(html::Function ondurationchange);
    html::Function getOnemptied();
    void setOnemptied(html::Function onemptied);
    html::Function getOnended();
    void setOnended(html::Function onended);
    html::Function getOnerror();
    void setOnerror(html::Function onerror);
    html::Function getOnfocus();
    void setOnfocus(html::Function onfocus);
    html::Function getOninput();
    void setOninput(html::Function oninput);
    html::Function getOninvalid();
    void setOninvalid(html::Function oninvalid);
    html::Function getOnkeydown();
    void setOnkeydown(html::Function onkeydown);
    html::Function getOnkeypress();
    void setOnkeypress(html::Function onkeypress);
    html::Function getOnkeyup();
    void setOnkeyup(html::Function onkeyup);
    html::Function getOnload();
    void setOnload(html::Function onload);
    html::Function getOnloadeddata();
    void setOnloadeddata(html::Function onloadeddata);
    html::Function getOnloadedmetadata();
    void setOnloadedmetadata(html::Function onloadedmetadata);
    html::Function getOnloadstart();
    void setOnloadstart(html::Function onloadstart);
    html::Function getOnmousedown();
    void setOnmousedown(html::Function onmousedown);
    html::Function getOnmousemove();
    void setOnmousemove(html::Function onmousemove);
    html::Function getOnmouseout();
    void setOnmouseout(html::Function onmouseout);
    html::Function getOnmouseover();
    void setOnmouseover(html::Function onmouseover);
    html::Function getOnmouseup();
    void setOnmouseup(html::Function onmouseup);
    html::Function getOnmousewheel();
    void setOnmousewheel(html::Function onmousewheel);
    html::Function getOnpause();
    void setOnpause(html::Function onpause);
    html::Function getOnplay();
    void setOnplay(html::Function onplay);
    html::Function getOnplaying();
    void setOnplaying(html::Function onplaying);
    html::Function getOnprogress();
    void setOnprogress(html::Function onprogress);
    html::Function getOnratechange();
    void setOnratechange(html::Function onratechange);
    html::Function getOnreadystatechange();
    void setOnreadystatechange(html::Function onreadystatechange);
    html::Function getOnreset();
    void setOnreset(html::Function onreset);
    html::Function getOnscroll();
    void setOnscroll(html::Function onscroll);
    html::Function getOnseeked();
    void setOnseeked(html::Function onseeked);
    html::Function getOnseeking();
    void setOnseeking(html::Function onseeking);
    html::Function getOnselect();
    void setOnselect(html::Function onselect);
    html::Function getOnshow();
    void setOnshow(html::Function onshow);
    html::Function getOnstalled();
    void setOnstalled(html::Function onstalled);
    html::Function getOnsubmit();
    void setOnsubmit(html::Function onsubmit);
    html::Function getOnsuspend();
    void setOnsuspend(html::Function onsuspend);
    html::Function getOntimeupdate();
    void setOntimeupdate(html::Function ontimeupdate);
    html::Function getOnvolumechange();
    void setOnvolumechange(html::Function onvolumechange);
    html::Function getOnwaiting();
    void setOnwaiting(html::Function onwaiting);
    // HTMLDocument-38
    std::u16string getFgColor();
    void setFgColor(const std::u16string& fgColor);
    std::u16string getBgColor();
    void setBgColor(const std::u16string& bgColor);
    std::u16string getLinkColor();
    void setLinkColor(const std::u16string& linkColor);
    std::u16string getVlinkColor();
    void setVlinkColor(const std::u16string& vlinkColor);
    std::u16string getAlinkColor();
    void setAlinkColor(const std::u16string& alinkColor);
    html::HTMLCollection getAnchors();
    html::HTMLCollection getApplets();
    void clear();
    html::HTMLAllCollection getAll();
    // DocumentRange
    ranges::Range createRange();
    // DocumentTraversal
    traversal::NodeIterator createNodeIterator(Node root, unsigned int whatToShow, traversal::NodeFilter filter, bool entityReferenceExpansion) throw(DOMException);
    traversal::TreeWalker createTreeWalker(Node root, unsigned int whatToShow, traversal::NodeFilter filter, bool entityReferenceExpansion) throw(DOMException);
    // DocumentXBL
    html::HTMLCollection getBindingDocuments();
    Document loadBindingDocument(const std::u16string& documentURI);
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return Document::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return Document::getMetaData();
    }
};

}}}}  // org::w3c::dom::bootstrap

#endif  // DOCUMENT_IMP_H
