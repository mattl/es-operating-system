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

#ifndef LOCATION_IMP_H
#define LOCATION_IMP_H

#include <Object.h>
#include <org/w3c/dom/html/Location.h>

#include "url/URL.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

class WindowImp;

class LocationImp : public ObjectMixin<LocationImp>
{
    WindowImp* window;
    URL url;

public:
    LocationImp(WindowImp* window, const std::u16string& url);
    LocationImp(const LocationImp& other);

    // Location
    virtual std::u16string getHref();
    virtual void setHref(const std::u16string& href);
    virtual void assign(const std::u16string& url);
    virtual void replace(const std::u16string& url);
    virtual void reload();
    virtual std::u16string getProtocol();
    virtual void setProtocol(const std::u16string& protocol);
    virtual std::u16string getHost();
    virtual void setHost(const std::u16string& host);
    virtual std::u16string getHostname();
    virtual void setHostname(const std::u16string& hostname);
    virtual std::u16string getPort();
    virtual void setPort(const std::u16string& port);
    virtual std::u16string getPathname();
    virtual void setPathname(const std::u16string& pathname);
    virtual std::u16string getSearch();
    virtual void setSearch(const std::u16string& search);
    virtual std::u16string getHash();
    virtual void setHash(const std::u16string& hash);
    virtual std::u16string resolveURL(const std::u16string& url);
    // Object
    virtual Any message_(uint32_t selector, const char* id, int argc, Any* argv)
    {
        return html::Location::dispatch(this, selector, id, argc, argv);
    }
    static const char* const getMetaData()
    {
        return html::Location::getMetaData();
    }
};

}}}}  // org::w3c::dom::bootstrap

#endif  // LOCATION_IMP_H
