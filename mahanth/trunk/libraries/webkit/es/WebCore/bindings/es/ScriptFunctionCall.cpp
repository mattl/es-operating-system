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
#include "ScriptFunctionCall.h"

#include "ScriptValue.h"
#include "ScriptObject.h"

#include "NotImplemented.h"

namespace WebCore {

ScriptFunctionCall::ScriptFunctionCall(ScriptState* exec, const ScriptObject& thisObject, const String& name)
{
    notImplemented();
}

void ScriptFunctionCall::appendArgument(const ScriptObject& argument)
{
    notImplemented();
}

void ScriptFunctionCall::appendArgument(const ScriptString& argument)
{
    notImplemented();
}

void ScriptFunctionCall::appendArgument(const ScriptValue& argument)
{
    notImplemented();
}

void ScriptFunctionCall::appendArgument(const String& argument)
{
    notImplemented();
}

void ScriptFunctionCall::appendArgument(long long argument)
{
    notImplemented();
}

void ScriptFunctionCall::appendArgument(unsigned int argument)
{
    notImplemented();
}

void ScriptFunctionCall::appendArgument(int argument)
{
    notImplemented();
}

void ScriptFunctionCall::appendArgument(bool argument)
{
    notImplemented();
}

ScriptValue ScriptFunctionCall::call(bool& hadException, bool reportExceptions)
{
    notImplemented();
}

ScriptValue ScriptFunctionCall::call()
{
    bool hadException = false;
    return call(hadException);
}

ScriptObject ScriptFunctionCall::construct(bool& hadException, bool reportExceptions)
{
    notImplemented();
}

} // namespace WebCore
