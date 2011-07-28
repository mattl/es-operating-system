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

#include "config.h"
#include "ScriptObject.h"

#include "NotImplemented.h"

namespace WebCore {

bool ScriptObject::set(ScriptState* scriptState, const String& name, const String& value)
{
    notImplemented();
    return false;
}

bool ScriptObject::set(ScriptState* scriptState, const char* name, const ScriptObject& value)
{
    notImplemented();
    return false;
}

bool ScriptObject::set(ScriptState* scriptState, const char* name, const String& value)
{
    notImplemented();
    return false;
}

bool ScriptObject::set(ScriptState* scriptState, const char* name, double value)
{
    notImplemented();
    return false;
}

bool ScriptObject::set(ScriptState* scriptState, const char* name, long long value)
{
    notImplemented();
    return false;
}

bool ScriptObject::set(ScriptState* scriptState, const char* name, int value)
{
    notImplemented();
    return false;
}

bool ScriptObject::set(ScriptState* scriptState, const char* name, bool value)
{
    notImplemented();
    return false;
}

bool ScriptGlobalObject::set(ScriptState* scriptState, const char* name, const ScriptObject& value)
{
    notImplemented();
    return false;
}

bool ScriptGlobalObject::set(ScriptState* scriptState, const char* name, InspectorController* value)
{
    notImplemented();
    return false;
}

bool ScriptGlobalObject::get(ScriptState* scriptState, const char* name, ScriptObject& value)
{
    notImplemented();
    return false;
}

bool ScriptGlobalObject::remove(ScriptState* scriptState, const char* name)
{
    notImplemented();
    return false;
}

ScriptObject ScriptObject::createNew(ScriptState* scriptState)
{
    notImplemented();
    return ScriptObject();
}

} // namespace WebCore
