/*
 * Copyright (c) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ScriptObject_h
#define ScriptObject_h

#include "ScriptState.h"
#include "ScriptValue.h"

namespace WebCore {
    class InspectorController;

    class ScriptObject : public ScriptValue {
    public:
        ScriptObject() {}

        bool set(ScriptState*, const String& name, const String&);
        bool set(ScriptState*, const char* name, const ScriptObject&);
        bool set(ScriptState*, const char* name, const String&);
        bool set(ScriptState*, const char* name, double);
        bool set(ScriptState*, const char* name, long long);
        bool set(ScriptState*, const char* name, int);
        bool set(ScriptState*, const char* name, bool);

        static ScriptObject createNew(ScriptState*);
    };

    class ScriptGlobalObject {
    public:
        static bool set(ScriptState*, const char* name, const ScriptObject&);
        static bool set(ScriptState*, const char* name, InspectorController*);
        static bool get(ScriptState*, const char* name, ScriptObject&);
        static bool remove(ScriptState*, const char* name);
    private:
        ScriptGlobalObject() {}
    };

}

#endif // ScriptObject_h
