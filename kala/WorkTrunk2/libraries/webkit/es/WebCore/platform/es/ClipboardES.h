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

#ifndef ClipboardES_h
#define ClipboardES_h

#include "Clipboard.h"

namespace WebCore {
    class CachedImage;

    class ClipboardES : public Clipboard {
    public:
        static PassRefPtr<ClipboardES> create(ClipboardAccessPolicy policy, bool isForDragging)
        {
            return adoptRef(new ClipboardES(policy, isForDragging));
        }
        virtual ~ClipboardES();

        void clearData(const String&);
        void clearAllData();
        String getData(const String&, bool&) const;
        bool setData(const String&, const String&);

        HashSet<String> types() const;

        void setDragImage(CachedImage*, const IntPoint&);
        void setDragImageElement(Node*, const IntPoint&);

        virtual DragImageRef createDragImage(IntPoint&) const;
        virtual void declareAndWriteDragImage(Element*, const KURL&, const String&, Frame*);
        virtual void writeURL(const KURL&, const String&, Frame*);
        virtual void writeRange(Range*, Frame*);

        virtual bool hasData();

    private:
        ClipboardES(ClipboardAccessPolicy, bool);
    };
}

#endif // ClipboardES_h
