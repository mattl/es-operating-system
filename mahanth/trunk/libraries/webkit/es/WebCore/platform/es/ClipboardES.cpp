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
#include "ClipboardES.h"

#include "Editor.h"
#include "StringHash.h"

#include "NotImplemented.h"

namespace WebCore {

PassRefPtr<Clipboard> Editor::newGeneralClipboard(ClipboardAccessPolicy policy)
{
    return ClipboardES::create(policy, false);
}

ClipboardES::ClipboardES(ClipboardAccessPolicy policy, bool forDragging)
    : Clipboard(policy, forDragging)
{
    notImplemented();
}

ClipboardES::~ClipboardES()
{
    notImplemented();
}

void ClipboardES::clearData(const String&)
{
    notImplemented();
}

void ClipboardES::clearAllData()
{
    notImplemented();
}

String ClipboardES::getData(const String&, bool &success) const
{
    notImplemented();
    success = false;
    return String();
}

bool ClipboardES::setData(const String&, const String&)
{
    notImplemented();
    return false;
}

HashSet<String> ClipboardES::types() const
{
    notImplemented();
    return HashSet<String>();
}

void ClipboardES::setDragImage(CachedImage*, const IntPoint&)
{
    notImplemented();
}

void ClipboardES::setDragImageElement(Node*, const IntPoint&)
{
    notImplemented();
}

DragImageRef ClipboardES::createDragImage(IntPoint&) const
{
    notImplemented();
    return 0;
}

void ClipboardES::declareAndWriteDragImage(Element*, const KURL&, const String&, Frame*)
{
    notImplemented();
}

void ClipboardES::writeURL(const KURL&, const String&, Frame*)
{
    notImplemented();
}

void ClipboardES::writeRange(Range*, Frame*)
{
    notImplemented();
}

bool ClipboardES::hasData()
{
    notImplemented();
    return false;
}

} // namespace WebCore
