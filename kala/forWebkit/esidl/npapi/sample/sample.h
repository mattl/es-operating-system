/*
 * Copyright 2009 Google Inc.
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

#include "esnpapi.h"

// Forward declarations for mixins.
// TODO: These must be generated by esidl.
namespace es
{
    class EventListener;
    class Event;
    class ClientRect;
    class ClientRectList;
    class AbstractView;
    class Range;
    class Location;
    class Selection;
    class HTMLDocument;
    class HTMLElement;
    class HTMLCollection;
    class Window;
    typedef Window WindowProxy;
}

class PluginInstance
{
    NPP npp;
    NPObject* window;

    void test();

public:
    PluginInstance(NPP npp) :
        npp(npp),
        window(0)
    {
        NPN_GetValue(npp, NPNVWindowNPObject, &window);
        test();
    }

    ~PluginInstance()
    {
        if (window)
        {
            NPN_ReleaseObject(window);
        }
    }

    NPObject* getScriptableInstance()
    {
        return 0;
    }
};