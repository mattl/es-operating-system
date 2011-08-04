// Generated by esidl (r1752).
// This file is expected to be modified for the Web IDL interface
// implementation.  Permission to use, copy, modify and distribute
// this file in any software license is hereby granted.

#ifndef ORG_W3C_DOM_BOOTSTRAP_MEDIAQUERYLISTIMP_H_INCLUDED
#define ORG_W3C_DOM_BOOTSTRAP_MEDIAQUERYLISTIMP_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <org/w3c/dom/html/MediaQueryList.h>

#include <org/w3c/dom/html/MediaQueryListListener.h>

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{
class MediaQueryListImp : public ObjectMixin<MediaQueryListImp>
{
public:
    // MediaQueryList
    std::u16string getMedia() __attribute__((weak));
    bool getMatches() __attribute__((weak));
    void addListener(html::MediaQueryListListener listener) __attribute__((weak));
    void removeListener(html::MediaQueryListListener listener) __attribute__((weak));
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return html::MediaQueryList::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return html::MediaQueryList::getMetaData();
    }
};

}
}
}
}

#endif  // ORG_W3C_DOM_BOOTSTRAP_MEDIAQUERYLISTIMP_H_INCLUDED
