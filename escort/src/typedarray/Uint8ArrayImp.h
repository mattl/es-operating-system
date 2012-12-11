// Generated by esidl 0.2.1.
// This file is expected to be modified for the Web IDL interface
// implementation.  Permission to use, copy, modify and distribute
// this file in any software license is hereby granted.

#ifndef ORG_W3C_DOM_BOOTSTRAP_UINT8ARRAYIMP_H_INCLUDED
#define ORG_W3C_DOM_BOOTSTRAP_UINT8ARRAYIMP_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/typedarray/Uint8Array.h>
#include "ArrayBufferViewImp.h"

#include <org/w3c/dom/typedarray/ArrayBuffer.h>
#include <org/w3c/dom/typedarray/ArrayBufferView.h>
#include <org/w3c/dom/typedarray/Uint8Array.h>

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{
class Uint8ArrayImp : public ObjectMixin<Uint8ArrayImp, ArrayBufferViewImp>
{
public:
    // Uint8Array
    unsigned int getLength();
    unsigned char get(unsigned int index);
    void set(unsigned int index, unsigned char value);
    void set(typedarray::Uint8Array array);
    void set(typedarray::Uint8Array array, unsigned int offset);
    void set(ObjectArray<unsigned char> array);
    void set(ObjectArray<unsigned char> array, unsigned int offset);
    typedarray::Uint8Array subarray(int start, int end);
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return typedarray::Uint8Array::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return typedarray::Uint8Array::getMetaData();
    }
};

}
}
}
}

#endif  // ORG_W3C_DOM_BOOTSTRAP_UINT8ARRAYIMP_H_INCLUDED
