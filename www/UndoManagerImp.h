// Generated by esidl (r1745).
// This file is expected to be modified for the Web IDL interface
// implementation.  Permission to use, copy, modify and distribute
// this file in any software license is hereby granted.

#ifndef ORG_W3C_DOM_BOOTSTRAP_UNDOMANAGERIMP_H_INCLUDED
#define ORG_W3C_DOM_BOOTSTRAP_UNDOMANAGERIMP_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/html/UndoManager.h>

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{
class UndoManagerImp : public ObjectMixin<UndoManagerImp>
{
public:
    // UndoManager
    unsigned int getLength();
    Any item(unsigned int index);
    unsigned int getPosition();
    unsigned int add(Any data, std::u16string title);
    void remove(unsigned int index);
    void clearUndo();
    void clearRedo();
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return html::UndoManager::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return html::UndoManager::getMetaData();
    }
};

}
}
}
}

#endif  // ORG_W3C_DOM_BOOTSTRAP_UNDOMANAGERIMP_H_INCLUDED