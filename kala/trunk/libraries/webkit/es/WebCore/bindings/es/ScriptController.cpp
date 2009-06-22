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
#include "ScriptController.h"

#include "NotImplemented.h"

namespace WebCore {

ScriptController::ScriptController(Frame* frame)
    : m_frame(frame)
{
}

ScriptController::~ScriptController()
{
}

ScriptValue ScriptController::evaluate(const ScriptSourceCode& sourceCode)
{
    notImplemented();
}

bool ScriptController::haveWindowShell() const
{
    notImplemented();
}

ScriptController* ScriptController::windowShell()
{
    notImplemented();
}

bool ScriptController::isEnabled() const
{
    notImplemented();
}

void ScriptController::setEventHandlerLineNumber(int lineNumber)
{
    notImplemented();
}

PassScriptInstance ScriptController::createScriptInstanceForWidget(Widget* widget)
{
    notImplemented();
}

void ScriptController::disconnectFrame()
{
    notImplemented();
}

void ScriptController::attachDebugger(void*)
{
    notImplemented();
}

bool ScriptController::processingUserGesture() const
{
    notImplemented();
}

bool ScriptController::isPaused() const
{
    notImplemented();
}

const String* ScriptController::sourceURL() const
{
    notImplemented();
    return 0;
}

void ScriptController::clearWindowShell()
{
    notImplemented();
}

void ScriptController::updateDocument()
{
    notImplemented();
}

void ScriptController::updateSecurityOrigin()
{
    notImplemented();
}

void ScriptController::clearScriptObjects()
{
    notImplemented();
}

void ScriptController::updatePlatformScriptObjects()
{
    notImplemented();
}

} // namespace WebCore
