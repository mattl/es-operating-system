/*
 * Copyright 2008 Google Inc.
 * Copyright 2006 Nintendo Co., Ltd.
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

#include <errno.h>
#include <es.h>
#include <es/exception.h>

void esThrow(int result)
{
    switch (result)
    {
    case EPERM:         // Operation not permitted
        throw SystemException<EPERM>();
    case ENOENT:        // No such file or directory
        throw SystemException<ENOENT>();
    case ESRCH:         // No such process
        throw SystemException<ESRCH>();
    case EINTR:         // Interrupted system call
        throw SystemException<EINTR>();
    case EIO:           // I/O error
        throw SystemException<EIO>();
    case ENXIO:         // No such device or address
        throw SystemException<ENXIO>();
    case E2BIG:         // Arg list too long
        throw SystemException<E2BIG>();
    case ENOEXEC:       // Exec format error
        throw SystemException<ENOEXEC>();
    case EBADF:         // Bad file number
        throw SystemException<EBADF>();
    case ECHILD:        // No child processes
        throw SystemException<ECHILD>();
    case EAGAIN:        // Try again
        throw SystemException<EAGAIN>();
    case ENOMEM:        // Out of memory
        throw SystemException<ENOMEM>();
    case EACCES:        // Permission denied
        throw SystemException<EACCES>();
    case EFAULT:        // Bad address
        throw SystemException<EFAULT>();
    case ENOTBLK:       // Block device required
        throw SystemException<ENOTBLK>();
    case EBUSY:         // Device or resource busy
        throw SystemException<EBUSY>();
    case EEXIST:        // File exists
        throw SystemException<EEXIST>();
    case EXDEV:         // Cross-device link
        throw SystemException<EXDEV>();
    case ENODEV:        // No such device
        throw SystemException<ENODEV>();
    case ENOTDIR:       // Not a directory
        throw SystemException<ENOTDIR>();
    case EISDIR:        // Is a directory
        throw SystemException<EISDIR>();
    case EINVAL:        // Invalid argument
        throw SystemException<EINVAL>();
    case ENFILE:        // File table overflow
        throw SystemException<ENFILE>();
    case EMFILE:        // Too many open files
        throw SystemException<EMFILE>();
    case ENOTTY:        // Not a typewriter
        throw SystemException<ENOTTY>();
    case ETXTBSY:       // Text file busy
        throw SystemException<ETXTBSY>();
    case EFBIG:         // File too large
        throw SystemException<EFBIG>();
    case ENOSPC:        // No space left on device
        throw SystemException<ENOSPC>();
    case ESPIPE:        // Illegal seek
        throw SystemException<ESPIPE>();
    case EROFS:         // Read-only file system
        throw SystemException<EROFS>();
    case EMLINK:        // Too many links
        throw SystemException<EMLINK>();
    case EPIPE:         // Broken pipe
        throw SystemException<EPIPE>();
    case EDOM:          // Math argument out of domain of func
        throw SystemException<EDOM>();
    case ERANGE:        // Math result not representable
        throw SystemException<ERANGE>();
    case EDEADLK:       // Resource deadlock would occur
        throw SystemException<EDEADLK>();
    case ENAMETOOLONG:  // File name too long
        throw SystemException<ENAMETOOLONG>();
    case ENOLCK:        // No record locks available
        throw SystemException<ENOLCK>();
    case ENOSYS:        // Function not implemented
        throw SystemException<ENOSYS>();
    case ENOTEMPTY:     // Directory not empty
        throw SystemException<ENOTEMPTY>();
    case ELOOP:         // Too many symbolic links encountered
        throw SystemException<ELOOP>();
    case ENOMSG:        // No message of desired type
        throw SystemException<ENOMSG>();
    case EIDRM:         // Identifier removed
        throw SystemException<EIDRM>();
#ifndef __APPLE__
    case ECHRNG:        // Channel number out of range
        throw SystemException<ECHRNG>();
    case EL2NSYNC:      // Level 2 not synchronized
        throw SystemException<EL2NSYNC>();
    case EL3HLT:        // Level 3 halted
        throw SystemException<EL3HLT>();
    case EL3RST:        // Level 3 reset
        throw SystemException<EL3RST>();
    case ELNRNG:        // Link number out of range
        throw SystemException<ELNRNG>();
    case EUNATCH:       // Protocol driver not attached
        throw SystemException<EUNATCH>();
    case ENOCSI:        // No CSI structure available
        throw SystemException<ENOCSI>();
    case EL2HLT:        // Level 2 halted
        throw SystemException<EL2HLT>();
    case EBADE:         // Invalid exchange
        throw SystemException<EBADE>();
    case EBADR:         // Invalid request descriptor
        throw SystemException<EBADR>();
    case EXFULL:        // Exchange full
        throw SystemException<EXFULL>();
    case ENOANO:        // No anode
        throw SystemException<ENOANO>();
    case EBADRQC:       // Invalid request code
        throw SystemException<EBADRQC>();
    case EBADSLT:       // Invalid slot
        throw SystemException<EBADSLT>();
    case EBFONT:        // Bad font file format
        throw SystemException<EBFONT>();
#endif  // __APPLE__
    case ENOSTR:        // Device not a stream
        throw SystemException<ENOSTR>();
    case ENODATA:       // No data available
        throw SystemException<ENODATA>();
    case ETIME:         // Timer expired
        throw SystemException<ETIME>();
    case ENOSR:         // Out of streams resources
        throw SystemException<ENOSR>();
#ifndef __APPLE__
    case ENONET:        // Machine is not on the network
        throw SystemException<ENONET>();
    case ENOPKG:        // Package not installed
        throw SystemException<ENOPKG>();
#endif  // __APPLE__
    case EREMOTE:       // Object is remote
        throw SystemException<EREMOTE>();
    case ENOLINK:       // Link has been severed
        throw SystemException<ENOLINK>();
#ifndef __APPLE__
    case EADV:          // Advertise error
        throw SystemException<EADV>();
    case ESRMNT:        // Srmount error
        throw SystemException<ESRMNT>();
    case ECOMM:         // Communication error on send
        throw SystemException<ECOMM>();
#endif  // __APPLE__
    case EPROTO:        // Protocol error
        throw SystemException<EPROTO>();
    case EMULTIHOP:     // Multihop attempted
        throw SystemException<EMULTIHOP>();
#ifndef __APPLE__
    case EDOTDOT:       // RFS specific error
        throw SystemException<EDOTDOT>();
#endif  // __APPLE__
    case EBADMSG:       // Not a data message
        throw SystemException<EBADMSG>();
    case EOVERFLOW:     // Value too large for defined data type
        throw SystemException<EOVERFLOW>();
#ifndef __APPLE__
    case ENOTUNIQ:      // Name not unique on network
        throw SystemException<ENOTUNIQ>();
    case EBADFD:        // File descriptor in bad state
        throw SystemException<EBADFD>();
    case EREMCHG:       // Remote address changed
        throw SystemException<EREMCHG>();
    case ELIBACC:       // Can not access a needed shared library
        throw SystemException<ELIBACC>();
    case ELIBBAD:       // Accessing a corrupted shared library
        throw SystemException<ELIBBAD>();
    case ELIBSCN:       // .lib section in a.out corrupted
        throw SystemException<ELIBSCN>();
    case ELIBMAX:       // Attempting to link in too many shared libraries
        throw SystemException<ELIBMAX>();
    case ELIBEXEC:      // Cannot exec a shared library directly
        throw SystemException<ELIBEXEC>();
#endif  // __APPLE__
    case EILSEQ:        // Illegal byte sequence
        throw SystemException<EILSEQ>();
#ifdef __posix__
    case ERESTART:      // Interrupted system call should be restarted
        throw SystemException<ERESTART>();
    case ESTRPIPE:      // Streams pipe error
        throw SystemException<ESTRPIPE>();
#endif
    case EUSERS:        // Too many users
        throw SystemException<EUSERS>();
    case ENOTSOCK:      // Socket operation on non-socket
        throw SystemException<ENOTSOCK>();
    case EDESTADDRREQ:  // Destination address required
        throw SystemException<EDESTADDRREQ>();
    case EMSGSIZE:      // Message too long
        throw SystemException<EMSGSIZE>();
    case EPROTOTYPE:    // Protocol wrong type for socket
        throw SystemException<EPROTOTYPE>();
    case ENOPROTOOPT:   // Protocol not available
        throw SystemException<ENOPROTOOPT>();
    case EPROTONOSUPPORT:   // Protocol not supported
        throw SystemException<EPROTONOSUPPORT>();
    case ESOCKTNOSUPPORT:   // Socket type not supported
        throw SystemException<ESOCKTNOSUPPORT>();
    case EOPNOTSUPP:    // Operation not supported on transport endpoint
        throw SystemException<EOPNOTSUPP>();
    case EPFNOSUPPORT:  // Protocol family not supported
        throw SystemException<EPFNOSUPPORT>();
    case EAFNOSUPPORT:  // Address family not supported by protocol
        throw SystemException<EAFNOSUPPORT>();
    case EADDRINUSE:    // Address already in use
        throw SystemException<EADDRINUSE>();
    case EADDRNOTAVAIL: // Cannot assign requested address
        throw SystemException<EADDRNOTAVAIL>();
    case ENETDOWN:      // Network is down
        throw SystemException<ENETDOWN>();
    case ENETUNREACH:   // Network is unreachable
        throw SystemException<ENETUNREACH>();
    case ENETRESET:     // Network dropped connection because of reset
        throw SystemException<ENETRESET>();
    case ECONNABORTED:  // Software caused connection abort
        throw SystemException<ECONNABORTED>();
    case ECONNRESET:    // Connection reset by peer
        throw SystemException<ECONNRESET>();
    case ENOBUFS:       // No buffer space available
        throw SystemException<ENOBUFS>();
    case EISCONN:       // Transport endpoint is already connected
        throw SystemException<EISCONN>();
    case ENOTCONN:      // Transport endpoint is not connected
        throw SystemException<ENOTCONN>();
    case ESHUTDOWN:     // Cannot send after transport endpoint shutdown
        throw SystemException<ESHUTDOWN>();
    case ETOOMANYREFS:  // Too many references: cannot splice
        throw SystemException<ETOOMANYREFS>();
    case ETIMEDOUT:     // Connection timed out
        throw SystemException<ETIMEDOUT>();
    case ECONNREFUSED:  // Connection refused
        throw SystemException<ECONNREFUSED>();
    case EHOSTDOWN:     // Host is down
        throw SystemException<EHOSTDOWN>();
    case EHOSTUNREACH:  // No route to host
        throw SystemException<EHOSTUNREACH>();
    case EALREADY:      // Operation already in progress
        throw SystemException<EALREADY>();
    case EINPROGRESS:   // Operation now in progress
        throw SystemException<EINPROGRESS>();
    case ESTALE:        // Stale NFS file handle
        throw SystemException<ESTALE>();
#ifdef __posix__
    case EUCLEAN:       // Structure needs cleaning
        throw SystemException<EUCLEAN>();
    case ENOTNAM:       // Not a XENIX named type file
        throw SystemException<ENOTNAM>();
    case ENAVAIL:       // No XENIX semaphores available
        throw SystemException<ENAVAIL>();
    case EISNAM:        // Is a named type file
        throw SystemException<EISNAM>();
    case EREMOTEIO:     // Remote I/O error
        throw SystemException<EREMOTEIO>();
#endif
    case EDQUOT:        // Quota exceeded
        throw SystemException<EDQUOT>();
#ifndef __APPLE__
    case ENOMEDIUM:     // No medium found
        throw SystemException<ENOMEDIUM>();
#endif  // __APPLE__
#ifdef __posix__
    case EMEDIUMTYPE:   // Wrong medium type
        throw SystemException<EMEDIUMTYPE>();
#endif
    default:
        throw SystemException<-1>();
    }
}
