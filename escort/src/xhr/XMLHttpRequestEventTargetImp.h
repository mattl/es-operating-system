// Generated by esidl 0.2.1.
// This file is expected to be modified for the Web IDL interface
// implementation.  Permission to use, copy, modify and distribute
// this file in any software license is hereby granted.

#ifndef ORG_W3C_DOM_BOOTSTRAP_XMLHTTPREQUESTEVENTTARGETIMP_H_INCLUDED
#define ORG_W3C_DOM_BOOTSTRAP_XMLHTTPREQUESTEVENTTARGETIMP_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/XMLHttpRequestEventTarget.h>
#include "EventTargetImp.h"

#include <org/w3c/dom/events/EventTarget.h>
#include <org/w3c/dom/html/Function.h>

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{
class XMLHttpRequestEventTargetImp : public ObjectMixin<XMLHttpRequestEventTargetImp, EventTargetImp>
{
public:
    // XMLHttpRequestEventTarget
    html::Function getOnloadstart();
    void setOnloadstart(html::Function onloadstart);
    html::Function getOnprogress();
    void setOnprogress(html::Function onprogress);
    html::Function getOnabort();
    void setOnabort(html::Function onabort);
    html::Function getOnerror();
    void setOnerror(html::Function onerror);
    html::Function getOnload();
    void setOnload(html::Function onload);
    html::Function getOntimeout();
    void setOntimeout(html::Function ontimeout);
    html::Function getOnloadend();
    void setOnloadend(html::Function onloadend);
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return XMLHttpRequestEventTarget::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return XMLHttpRequestEventTarget::getMetaData();
    }
};

}
}
}
}

#endif  // ORG_W3C_DOM_BOOTSTRAP_XMLHTTPREQUESTEVENTTARGETIMP_H_INCLUDED
