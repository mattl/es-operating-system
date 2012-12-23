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

#include "LocationImp.h"

#include "WindowImp.h"
#include "http/HTTPConnection.h"

namespace org { namespace w3c { namespace dom { namespace bootstrap {

// Location
std::u16string LocationImp::getHref()
{
    return static_cast<std::u16string>(url);
}

void LocationImp::setHref(const std::u16string& href)
{
    assign(href);
}

void LocationImp::assign(const std::u16string& url)
{
    URL base(window->getLocation().getHref());    // TODO: window must be of the script.
    URL resolved(base, url);
    if (resolved.isEmpty())
        return;
    window->open(resolved, u"_self", u"", false);
}

void LocationImp::replace(const std::u16string& url)
{
    URL base(window->getLocation().getHref());    // TODO: window must be of the script.
    URL resolved(base, url);
    if (resolved.isEmpty())
        return;
    window->open(resolved, u"_self", u"", true);
}

void LocationImp::reload()
{
    // TODO: Refine me!
    window->open(url, u"_self", u"", true);
}

std::u16string LocationImp::getProtocol()
{
    return url.getProtocol();
}

void LocationImp::setProtocol(const std::u16string& protocol)
{
    // TODO: implement me!
}

std::u16string LocationImp::getHost()
{
    return url.getHost();
}

void LocationImp::setHost(const std::u16string& host)
{
    // TODO: implement me!
}

std::u16string LocationImp::getHostname()
{
    return url.getHostname();
}

void LocationImp::setHostname(const std::u16string& hostname)
{
    // TODO: implement me!
}

std::u16string LocationImp::getPort()
{
    return url.getPort();
}

void LocationImp::setPort(const std::u16string& port)
{
    // TODO: implement me!
}

std::u16string LocationImp::getPathname()
{
    return url.getPathname();
}

void LocationImp::setPathname(const std::u16string& pathname)
{
    // TODO: implement me!
}

std::u16string LocationImp::getSearch()
{
    return url.getSearch();
}

void LocationImp::setSearch(const std::u16string& search)
{
    // TODO: implement me!
}

std::u16string LocationImp::getHash()
{
    return url.getHash();
}

void LocationImp::setHash(const std::u16string& hash)
{
    // TODO: implement me!
}

std::u16string LocationImp::resolveURL(const std::u16string& url)
{
    URL base(window->getLocation().getHref());    // TODO: window must be of the script.
    URL resolved(base, url);
    // TODO: Raise an exception upon failure.
    return resolved;
}

LocationImp::LocationImp(WindowImp* window, const std::u16string& url) :
    window(window),
    url(url)
{
}

LocationImp::LocationImp(const LocationImp& other) :
    window(other.window),
    url(other.url)
{
}

}}}}  // org::w3c::dom::bootstrap
