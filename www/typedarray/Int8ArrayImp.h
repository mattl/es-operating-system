// Generated by esidl (r1745).
// This file is expected to be modified for the Web IDL interface
// implementation.  Permission to use, copy, modify and distribute
// this file in any software license is hereby granted.

#ifndef ORG_W3C_DOM_BOOTSTRAP_INT8ARRAYIMP_H_INCLUDED
#define ORG_W3C_DOM_BOOTSTRAP_INT8ARRAYIMP_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/typedarray/Int8Array.h>
#include "ArrayBufferViewImp.h"

#include <org/w3c/dom/typedarray/ArrayBuffer.h>
#include <org/w3c/dom/typedarray/ArrayBufferView.h>
#include <org/w3c/dom/typedarray/Int8Array.h>

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{
class Int8ArrayImp : public ObjectMixin<Int8ArrayImp, ArrayBufferViewImp>
{
public:
    // Int8Array
    unsigned int getLength() __attribute__((weak));
    signed char get(unsigned int index) __attribute__((weak));
    void set(unsigned int index, signed char value) __attribute__((weak));
    void set(typedarray::Int8Array array) __attribute__((weak));
    void set(typedarray::Int8Array array, unsigned int offset) __attribute__((weak));
    void set(ObjectArray<signed char> array) __attribute__((weak));
    void set(ObjectArray<signed char> array, unsigned int offset) __attribute__((weak));
    typedarray::Int8Array subarray(int start, int end) __attribute__((weak));
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return typedarray::Int8Array::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return typedarray::Int8Array::getMetaData();
    }
};

}
}
}
}

#endif  // ORG_W3C_DOM_BOOTSTRAP_INT8ARRAYIMP_H_INCLUDED
