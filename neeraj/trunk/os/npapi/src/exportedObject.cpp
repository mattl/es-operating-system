/*
 * Copyright 2008, 2009 Google Inc.
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

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <string.h>
#include <unistd.h>

#include <es/reflect.h>
#include <es/object.h>
#include <es/util/ICanvasRenderingContext2D.h>

#include "exportedObject.h"
#include "bridge.h"

// #define VERBOSE

#ifndef VERBOSE
#define FPRINTF(...)    (__VA_ARGS__)
#else
#define FPRINTF(...)    fprintf(__VA_ARGS__)
#endif

using namespace es;

extern Reflect::Interface& getInterface(const Guid& iid);

namespace
{

struct RpcRecord
{
    ExportedObject* exported;
    es::RpcReq* req;
    Process* process;
    int* fdv;
    int* fdmax;
    NPP npp;
    int s;
    void* resultPtr;
    size_t resultSize;
    Reflect::Method method;
    Param* argp;
    bool interfaceQuery;
};

}   // namespace

int ExportedObject::
asyncInvoke(void* param)
{
    RpcRecord* record = static_cast<RpcRecord*>(param);

    es::RpcReq* req = record->req;
    int* fdv = record->fdv;
    int* fdmax = record->fdmax;
    Reflect::Method method = record->method;
    Param* argp = record->argp;

    es::RpcRes res = { es::RPC_RES, req->tag, getpid(), 0 };

    Reflect::Type returnType = method.getReturnType();
    Guid iid = IInterface::iid();

    // Convert argp->size to argp->ptr
    NPVariant paramv[8];
    NPVariant* paramp = paramv;
    NPVariant result;
    u8* data = static_cast<u8*>(req->getData());    // TODO review alignment issues
    size_t size;
    for (int i = 0; i < method.getParameterCount(); ++i, ++argp, ++paramp)
    {
        Reflect::Parameter param(method.getParameter(i));
        Reflect::Type type(param.getType());
        assert(param.isInput());

        switch (type.getType())
        {
        case Ent::TypeSequence:
            // xxx* buf, int len, ...
            size = type.getSize() * argp->size;
            argp->ptr = data;
            argp->cls = Param::PTR;
            data += size;
            ++argp;
            // TODO: convert to object, then OBJECT_TO_NPVARIANT(object, *paramp);
            ++paramp;
            break;
        case Ent::SpecString:
            size = argp->size;
            argp->ptr = data;
            argp->cls = Param::PTR;
            data += size;
            STRINGZ_TO_NPVARIANT(static_cast<const char*>(argp->ptr), *paramp);
            break;
        case Ent::SpecWString:
            size = sizeof(wchar_t) * argp->size;
            argp->ptr = data;
            argp->cls = Param::PTR;
            data += size;
            // TODO: convert string to utf8, then STRINGZ_TO_NPVARIANT(static_cast<const char*>(argp->ptr), *paramp);
            break;
        case Ent::SpecUuid:
            iid = argp->guid;
            // TODO: convert string to utf8, then STRINGZ_TO_NPVARIANT(static_cast<const char*>(argp->ptr), *paramp);
            break;
        case Ent::TypeStructure:
        case Ent::TypeArray:
            size = argp->size;
            argp->ptr = data;
            argp->cls = Param::PTR;
            data += size;
            // TODO: convert to object, then OBJECT_TO_NPVARIANT(object, *paramp);
            break;
        case Ent::TypeInterface:
            iid = type.getInterface().getIid();
            // FALL THROUGH
        case Ent::SpecObject:
            if (argp->ptr)
            {
                // Import object
                Capability* cap = reinterpret_cast<Capability*>(data);
                if (cap->check == 0)
                {
                    cap->object = *fdv++;   // TODO check range
                }
                NPObject* object = record->process->importObject(*cap, iid, false);
                data += sizeof(Capability);
                OBJECT_TO_NPVARIANT(object, *paramp);    // TODO handle error condition where object == NULL
            }
            else
            {
                NULL_TO_NPVARIANT(*paramp);
            }
            break;
        case Ent::SpecBool:
            BOOLEAN_TO_NPVARIANT(argp->s32, *paramp);
            break;
        case Ent::SpecAny:  // TODO should not be s32
        case Ent::SpecS8:
        case Ent::SpecS16:
        case Ent::SpecS32:
        case Ent::SpecU8:
        case Ent::SpecU16:
        case Ent::SpecU32:
            INT32_TO_NPVARIANT(argp->s32, *paramp);
            break;
        case Ent::SpecS64:
        case Ent::SpecU64:
            DOUBLE_TO_NPVARIANT(argp->s64, *paramp);
            break;
        case Ent::SpecF32:
            DOUBLE_TO_NPVARIANT(argp->f32, *paramp);
            break;
        case Ent::SpecF64:
            DOUBLE_TO_NPVARIANT(argp->f64, *paramp);
            break;
        default:
            break;
        }
    }

    // TODO Close unused fdv
    FPRINTF(stderr, "%s: %s\n", __func__, method.getName());

    int fd;
    fdv = &fd;
    int* fdp = fdv;

    if (record->interfaceQuery)
    {
        // TODO check mothod, etc.
        FPRINTF(stderr, "queryInterface\n");
        if (strcmp(iid, ICanvasRenderingContext2D::iid()) == 0)
        {
            OBJECT_TO_NPVARIANT(object, result);
        }
        else
        {
            NULL_TO_NPVARIANT(result);
        }
    }
    else if (method.isOperation())
    {
        NPIdentifier methodName = NPN_GetStringIdentifier(method.getName());
        if (NPN_HasMethod(record->npp, object, methodName))
        {
            NPN_Invoke(record->npp, object, methodName, paramv, paramp - paramv, &result);
        }
    }
    else if (method.isGetter())
    {
        NPIdentifier propertyName = NPN_GetStringIdentifier(method.getName());
        if (NPN_HasProperty(record->npp, object, propertyName))
        {
            NPN_GetProperty(record->npp, object, propertyName, &result);
        }
    }
    else
    {
        assert(method.isSetter());
        NPIdentifier propertyName = NPN_GetStringIdentifier(method.getName());
        if (NPN_HasProperty(record->npp, object, propertyName))
        {
            NPN_SetProperty(record->npp, object, propertyName, paramv);
        }
        VOID_TO_NPVARIANT(result);
    }

    // TODO catch exception while applying
    switch (returnType.getType())
    {
    case Ent::SpecAny:  // XXX x86 specific
    case Ent::SpecBool:
    case Ent::SpecChar:
    case Ent::SpecWChar:
    case Ent::SpecS8:
    case Ent::SpecS16:
    case Ent::SpecS32:
    case Ent::SpecU8:
    case Ent::SpecU16:
    case Ent::SpecU32:
        res.result.s32 = NPVARIANT_TO_INT32(result);
        res.result.cls = Param::S32;
        break;
    case Ent::SpecS64:
    case Ent::SpecU64:
        res.result.s64 = NPVARIANT_TO_DOUBLE(result);
        res.result.cls = Param::S64;
        break;
    case Ent::SpecF32:
        res.result.f32 = static_cast<f32>(NPVARIANT_TO_DOUBLE(result));
        res.result.cls = Param::F32;
        break;
    case Ent::SpecF64:
        res.result.f64 = NPVARIANT_TO_DOUBLE(result);
        res.result.cls = Param::F64;
        break;
    case Ent::SpecString:
        res.result.s32 = NPVARIANT_TO_STRING(result).utf8length;    // TODO: check length
        res.result.cls = Param::S32;
        record->resultSize = 1 + res.result.s32;
        memcpy(record->resultPtr, NPVARIANT_TO_STRING(result).utf8characters, record->resultSize);
        break;
    case Ent::SpecWString:
        res.result.s32 = 0; // TODO
        res.result.cls = Param::S32;
        record->resultSize = sizeof(wchar_t) * (1 + res.result.s32);
        break;
    case Ent::TypeSequence:
        res.result.s32 = 0; // TODO
        res.result.cls = Param::S32;
        record->resultSize = sizeof(returnType.getSize() * res.result.s32);
        break;
    case Ent::TypeInterface:
        iid = returnType.getInterface().getIid();
        // FALL THROUGH
    case Ent::SpecObject:
        if (NPVARIANT_IS_NULL(result))
        {
            res.result.ptr = 0;
            res.result.cls = Param::PTR;
        }
        else
        {
            res.result.ptr = NPVARIANT_TO_OBJECT(result);
            res.result.cls = Param::PTR;
            Capability* cap = static_cast<Capability*>(record->resultPtr);
            record->process->exportObject(NPVARIANT_TO_OBJECT(result), iid, cap, true);   // TODO error check
            if (cap->check == 0)
            {
                *fdp++ = cap->object;
            }
        }
        break;
    case Ent::TypeArray:
    case Ent::SpecVoid:
        res.result.s32 = 0;
        res.result.cls = Param::S32;
        break;
    }

    NPN_ReleaseVariantValue(&result);

    // Send response
    struct iovec iov[2];
    iov[0].iov_base = &res;
    iov[0].iov_len = sizeof(RpcRes);

    // Invoke method
    struct msghdr msg;

    msg.msg_name = 0;
    msg.msg_namelen = 0;

    msg.msg_iov = iov;
    if (record->resultSize == 0)
    {
        msg.msg_iovlen = 1;
    }
    else
    {
        iov[1].iov_base = record->resultPtr;
        iov[1].iov_len = record->resultSize;
        msg.msg_iovlen = 2;
    }

    unsigned char buf[CMSG_SPACE(sizeof(int))];
    if (fdv < fdp)
    {
        int size = sizeof(int) * (fdp - fdv);
        msg.msg_control = buf;
        msg.msg_controllen = CMSG_SPACE(size);

        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(size);
        memcpy((int*) CMSG_DATA(cmsg), fdv, size);
        msg.msg_controllen = cmsg->cmsg_len;
    }
    else
    {
        msg.msg_control = 0;
        msg.msg_controllen = 0;
    }

    msg.msg_flags = 0;

    record->process->putExported(req->capability.object);

    // Note once the response is send back to the client,
    // RpcRecord is not longer valid for this invocation.
    // FPRINTF(stderr, "%s: %d\n", __func__, record->s);
    // dump(&res, sizeof(RpcRes));
    int rc = sendmsg(record->s, &msg, 0);
    if (rc == -1)
    {
        perror("sendmsg");
    }

    return rc;
}

namespace
{

void asyncCall(void* param)
{
    RpcRecord* record = static_cast<RpcRecord*>(param);
    record->exported->asyncInvoke(param);
}

}   // namespace

int ExportedObject::
invoke(es::RpcReq* req, int* fdv, int* fdmax, int s)
{
    FPRINTF(stderr, "%s(%p, %p, %p, %d)\n", __func__, req, fdv, fdmax, s);

    Process* process = currentThread->getProcess();
    es::RpcRes res = { es::RPC_RES, req->tag, getpid(), 0 };
    unsigned methodNumber = req->methodNumber;

    // Determine the type of interface and which method is being invoked.
    Reflect::Interface interface = getInterface(iid);

    // If this interface inherits another interface,
    // methodNumber is checked accordingly.
    if (interface.getInheritedMethodCount() + interface.getMethodCount() <= methodNumber)
    {
        res.exceptionCode = ENOSYS;
        return send(s, &res, sizeof res, 0);
    }
    unsigned baseMethodCount;
    Reflect::Interface super(interface);
    for (;;)
    {
        baseMethodCount = super.getInheritedMethodCount();
        if (baseMethodCount <= methodNumber)
        {
            break;
        }
        super = getInterface(super.getSuperIid());
    }
    Reflect::Method method(Reflect::Method(super.getMethod(methodNumber - baseMethodCount)));

    // TODO Review later.
    bool interfaceQuery = false;
    if (super.getIid() == IInterface::iid())
    {
        int count;
        switch (methodNumber - baseMethodCount)
        {
        case 1: // addRef
            count = addRef();
            if (1 != count)
            {
                res.result.s32 = count;
                res.result.cls = Param::S32;
                return send(s, &res, sizeof res, 0);
            }
            process->getExported(req->capability.object);
            break;
        case 2: // release
            count = release();
            process->putExported(req->capability.object);
            if (0 == count)
            {
                process->putExported(req->capability.object);
            }
            res.result.s32 = count;
            res.result.cls = Param::S32;
            return send(s, &res, sizeof res, 0);
            break;
        case 0: // queryInterface
            interfaceQuery = true;
            break;
        }
    }

    RpcRecord* record = static_cast<RpcRecord*>(RpcStack::alloc(sizeof(RpcRecord)));
    record->exported = this;
    record->req = req;
    record->process = currentThread->getProcess();
    record->fdv = fdv;
    record->fdmax = fdmax;
    record->npp = currentThread->getNpp();
    record->resultPtr = 0;
    record->resultSize = 0;
    record->s = s;
    record->method = method;
    record->interfaceQuery = interfaceQuery;

    Param* argv = req->getArgv();
    Param* argp = argv;

    // Skip this
    ++argp;

    // Reserve space from rpcStack to store result
    Reflect::Type returnType = method.getReturnType();
    switch (returnType.getType())
    {
    case Ent::TypeSequence:
        // int op(xxx* buf, int len, ...);
        argp->ptr = record->resultPtr = RpcStack::alloc(returnType.getSize() * argp[1].s32);
        argp->cls = Param::PTR;
        ++argp;
        ++argp;
        break;
    case Ent::SpecString:
        // int op(char* buf, int len, ...);
        argp->ptr = record->resultPtr = RpcStack::alloc(argp[1].s32);
        argp->cls = Param::PTR;
        ++argp;
        ++argp;
        break;
    case Ent::SpecWString:
        // int op(wchar_t* buf, int len, ...);
        argp->ptr = record->resultPtr = RpcStack::alloc(sizeof(wchar_t) * argp[1].s32);
        argp->cls = Param::PTR;
        ++argp;
        ++argp;
        break;
    case Ent::SpecUuid:
    case Ent::TypeStructure:
        // void op(struct* buf, ...);
        record->resultSize = returnType.getSize();
        argp->ptr = record->resultPtr = RpcStack::alloc(record->resultSize);
        argp->cls = Param::PTR;
        ++argp;
        break;
    case Ent::TypeArray:
        // void op(xxx[x] buf, ...);
        record->resultSize = returnType.getSize();
        argp->ptr = record->resultPtr = RpcStack::alloc(record->resultSize);
        argp->cls = Param::PTR;
        ++argp;
        break;
    case Ent::TypeInterface:
    case Ent::SpecObject:
        record->resultSize = sizeof(Capability);
        record->resultPtr = RpcStack::alloc(record->resultSize);
        break;
    }

    record->argp = argp;

    NPN_PluginThreadAsyncCall(currentThread->getNpp(), asyncCall, record);

    // TODO: should sync. with the plugin thread

    return 0;
}
