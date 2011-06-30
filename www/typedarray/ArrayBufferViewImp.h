// Generated by esidl (r1745).
// This file is expected to be modified for the Web IDL interface
// implementation.  Permission to use, copy, modify and distribute
// this file in any software license is hereby granted.

#ifndef ORG_W3C_DOM_BOOTSTRAP_ARRAYBUFFERVIEWIMP_H_INCLUDED
#define ORG_W3C_DOM_BOOTSTRAP_ARRAYBUFFERVIEWIMP_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/typedarray/ArrayBufferView.h>

#include <org/w3c/dom/typedarray/ArrayBuffer.h>

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{
class ArrayBufferViewImp : public ObjectMixin<ArrayBufferViewImp>
{
public:
    // ArrayBufferView
    typedarray::ArrayBuffer getBuffer() __attribute__((weak));
    unsigned int getByteOffset() __attribute__((weak));
    unsigned int getByteLength() __attribute__((weak));
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return typedarray::ArrayBufferView::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return typedarray::ArrayBufferView::getMetaData();
    }
};

}
}
}
}

#endif  // ORG_W3C_DOM_BOOTSTRAP_ARRAYBUFFERVIEWIMP_H_INCLUDED
