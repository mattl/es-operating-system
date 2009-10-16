/*
 * Copyright (c) 2008, 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#include "CString.h"
#include "FileChooser.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "SharedBuffer.h"
#include <float.h>

namespace WebCore {

String signedPublicKeyAndChallengeString(unsigned, const String&, const KURL&)
{
    notImplemented();
    return String();
}

void getSupportedKeySizes(Vector<String>&)
{
    notImplemented();
}

String KURL::fileSystemPath() const
{
    notImplemented();
    return String();
}

PassRefPtr<SharedBuffer> SharedBuffer::createWithContentsOfFile(const String&)
{
    notImplemented();
    return 0;
}

float userIdleTime()
{
    notImplemented();
    return FLT_MAX;
}

void prefetchDNS(const String& hostname)
{
    notImplemented();
}

String FileChooser::basenameForWidth(const Font&, int width) const
{
    notImplemented();
    return String();
}

CString fileSystemRepresentation(const String&)
{
    return "";
}

extern "C" {

int pthread_rwlockattr_init(pthread_rwlockattr_t *__attr)
{
    notImplemented();
    return 0;
}

int pthread_rwlockattr_destroy(pthread_rwlockattr_t *__attr)
{
    notImplemented();
    return 0;
}

int pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *__attr, int *__pshared)
{
    notImplemented();
    return 0;
}

int pthread_rwlockattr_setpshared(pthread_rwlockattr_t *__attr, int __pshared)
{
    notImplemented();
    return 0;
}

int pthread_rwlock_init(pthread_rwlock_t *__rwlock, const pthread_rwlockattr_t *__attr)
{
    notImplemented();
    return 0;
}

int pthread_rwlock_destroy(pthread_rwlock_t *__rwlock)
{
    notImplemented();
    return 0;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *__rwlock)
{
    notImplemented();
    return 0;
}

int pthread_rwlock_tryrdlock(pthread_rwlock_t *__rwlock)
{
    notImplemented();
    return 0;
}

int pthread_rwlock_timedrdlock(pthread_rwlock_t *__rwlock, const struct timespec *__abstime)
{
    notImplemented();
    return 0;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *__rwlock)
{
    notImplemented();
    return 0;
}

int pthread_rwlock_unlock(pthread_rwlock_t *__rwlock)
{
    notImplemented();
    return 0;
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *__rwlock)
{
    notImplemented();
    return 0;
}

int pthread_rwlock_timedwrlock(pthread_rwlock_t *__rwlock, const struct timespec *__abstime)
{
    notImplemented();
    return 0;
}

}// End of Extern C


} // namespace WebCore
