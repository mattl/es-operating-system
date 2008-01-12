/*
 * Copyright (c) 2006
 * Nintendo Co., Ltd.
 *  
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Nintendo makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 */

#ifndef NINTENDO_ES_REFLECT_H_INCLUDED
#define NINTENDO_ES_REFLECT_H_INCLUDED

#include <es.h>
#include <es/irf.h>
#include <es/types.h>

class Reflect
{
    u8* info;
    ReflectionFile::InterfaceDirectory* interfaceDirectory;
    ReflectionFile::TypeDirectory* typeDirectory;

    static const char* typeID[ReflectionFile::TAG_MAX];

public:

    static void* getPointer(u8* info, u32 offset)
    {
        return static_cast<void*>(info + sizeof(ReflectionFile::Header) +
                (offset & ReflectionFile::OFFSET_MASK));
    }

    class Interface;

    class Type
    {
        u8* info;
        u32 offset;

    public:
        Type(u8* info, u32 offset) :
            info(info),
            offset(offset)
        {
        }

        u32 getType() const
        {
            if (isPrimitive())
            {
                ASSERT((offset & ReflectionFile::OFFSET_MASK) < ReflectionFile::TAG_MAX);
                return offset;
            }

            ReflectionFile::NameRecord* nr(static_cast<ReflectionFile::NameRecord*>(Reflect::getPointer(info, offset)));
            return nr->type;
        }

        bool isConst() const
        {
            return (isPointer() || isReference()) && !(offset & ReflectionFile::IS_OUT);
        }

        bool isPrimitive() const
        {
            return offset & ReflectionFile::IS_PRIMITIVE;
        }

        bool isPointer() const
        {
            return offset & (ReflectionFile::IS_STAR | ReflectionFile::IS_STARSTAR);
        }

        bool isReference() const
        {
            return offset & ReflectionFile::IS_REFERENCE;
        }

        bool isStructure() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_STRUCTURE;
        }

        bool isInterface() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_INTERFACE;
        }

        bool isEnum() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_ENUM;
        }

        bool isConstant() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_CONSTANT;
        }

        bool isFunction() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_FUNCTION;
        }

        bool isArray() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_ARRAY;
        }

        bool isImported() const
        {
            return offset & ReflectionFile::IS_IID;
        }

        int getPointer() const
        {
            if (offset & ReflectionFile::IS_STAR)
            {
                return 1;
            }
            if (offset & ReflectionFile::IS_STARSTAR)
            {
                return 2;
            }
            return 0;
        }

        const char* getName() const
        {
            if (isPrimitive())
            {
                ASSERT((offset & ReflectionFile::OFFSET_MASK) < ReflectionFile::TAG_MAX);
                return typeID[offset & ReflectionFile::OFFSET_MASK];
            }

            ReflectionFile::NameRecord* nr(static_cast<ReflectionFile::NameRecord*>(Reflect::getPointer(info, offset)));
            return static_cast<char*>(Reflect::getPointer(info, nr->name));
        }

        int getSize() const
        {
            if (isPointer() || isReference() || isArray() || isFunction())
            {
                return sizeof(void*);
            }
            if (isPrimitive())
            {
                switch (offset & ReflectionFile::OFFSET_MASK)
                {
                case ReflectionFile::TAG_CHAR:
                case ReflectionFile::TAG_S8:
                case ReflectionFile::TAG_U8:
                    return sizeof(u8);
                case ReflectionFile::TAG_S16:
                case ReflectionFile::TAG_U16:
                    return sizeof(u16);
                case ReflectionFile::TAG_S32:
                case ReflectionFile::TAG_U32:
                case ReflectionFile::TAG_F32:
                    return sizeof(u32);
                case ReflectionFile::TAG_S64:
                case ReflectionFile::TAG_U64:
                case ReflectionFile::TAG_F64:
                    return sizeof(u64);
                case ReflectionFile::TAG_BOOLEAN:
                    return sizeof(bool);
                case ReflectionFile::TAG_WIDECHAR:
                    return sizeof(wchar_t);
                case ReflectionFile::TAG_VOID:
                    return 0;
                case ReflectionFile::TAG_UUID:
                    return sizeof(Guid);
                }
            }
            ASSERT(0);  // XXX
            return 0;
        }

        bool isInterfacePointer()
        {
            int str = getPointer();
            if (isReference())
            {
                ++str;
            }
            if (str == 1)
            {
                if (!isPrimitive() &&
                    (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_INTERFACE)
                {
                    return true;
                }
            }
            return false;
        }

        Interface getInterface();
    };

    class Identifier
    {
        u8* info;
        u32 offset;
        ReflectionFile::NameRecord* record;

    public:
        Identifier(u8* info, u32 offset) :
            info(info),
            offset(offset),
            record(static_cast<ReflectionFile::NameRecord*>(Reflect::getPointer(info, offset)))
        {
        }

        Type getType() const
        {
            // IS_IN, IS_OUT, IS_IIS, IS_SIZE are set in offset.
            return Type(info, record->type | (offset & 0x07000000));
        }

        char* getName() const
        {
            return static_cast<char*>(Reflect::getPointer(info, record->name));
        }

        bool isInput()
        {
            return offset & ReflectionFile::IS_IN;
        }

        bool isOutput()
        {
            return offset & ReflectionFile::IS_OUT;
        }

        int getIidIs() const
        {
            if (!(offset & ReflectionFile::IS_IID))
            {
                return -1;
            }
            u32* attr = (u32*) &record[1];
            return *attr;
        }

        int getSizeIs() const
        {
            if (!(offset & ReflectionFile::IS_SIZE))
            {
                return -1;
            }
            u32* attr = (u32*) &record[1];
            if (offset & ReflectionFile::IS_IID)
            {
                ++attr;
            }
            return *attr;
        }

        bool isInterfacePointer()
        {
            return 0 <= getIidIs() || getType().isInterfacePointer();
        }
    };

    class Function
    {
        u8* info;
        u32 offset;
        ReflectionFile::FunctionRecord* record;

    public:
        Function(u8* info, u32 offset) :
            info(info),
            offset(offset),
            record(static_cast<ReflectionFile::FunctionRecord*>(Reflect::getPointer(info, offset)))
        {
            ASSERT((offset & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_FUNCTION);
        }

        Type getType() const
        {
            return Type(info, record->type);
        }

        char* getName() const
        {
            return static_cast<char*>(Reflect::getPointer(info, record->name));
        }

        Type getReturnType() const
        {
            return Type(info, record->returnType);
        }

        int getParameterCount() const
        {
            return record->numParams;
        }

        Identifier getParameter(int n) const
        {
            return Identifier(info, record->params[n]);
        }

        int getParameterOffset(int n) const
        {
            int o(0);
            for (int i(0); i < n; ++i)
            {
                Reflect::Identifier parameter(getParameter(i));
                int size = parameter.getType().getSize();
                size += sizeof(unsigned) - 1;
                size &= ~(sizeof(unsigned) - 1);
                o += size;
            }
            return o;
        }
    };

    class Interface
    {
        u8* info;
        u32 offset;
        ReflectionFile::InterfaceRecord* record;
        ReflectionFile::InterfaceMethods* methods;

    public:
        Interface() :
            info(0),
            offset(0),
            record(0),
            methods(0)
        {
        }

        Interface(u8* info, u32 offset) :
            info(info),
            offset(offset),
            record(static_cast<ReflectionFile::InterfaceRecord*>(Reflect::getPointer(info, offset))),
            methods(static_cast<ReflectionFile::InterfaceMethods*>(Reflect::getPointer(info, offset +
                    sizeof(ReflectionFile::InterfaceRecord) + sizeof(u32) * (record->numMembers - 1))))
        {
        }

        Type getType() const
        {
            return Type(info, record->type);
        }

        char* getName() const
        {
            return static_cast<char*>(Reflect::getPointer(info, record->name));
        }

        Guid* getIid() const
        {
            return &record->iid;
        }

        // Returns 0 if not inherited any interface.
        Guid* getSuperIid() const
        {
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            if (methods->piid == GUID_NULL)
            {
                return 0;
            }
            return &methods->piid;
        }

        // method count including super class methods
        int getTotalMethodCount() const
        {
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            return record->numMethods;
        }

        // method count excluding super class methods
        int getMethodCount() const
        {
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            return methods->numMethods;
        }

        // method number excluding super class methods
        Function getMethod(int n) const
        {
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            ASSERT(0 <= n && n < getMethodCount());
            return Function(info, methods->methods[n]);
        }

        int getMemberCount() const
        {
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            return record->numMembers;
        }
    };

    Reflect(void* irf) :
        info(static_cast<u8*>(irf)),
        interfaceDirectory(static_cast<ReflectionFile::InterfaceDirectory*>(Reflect::getPointer(info, 0))),
        typeDirectory(static_cast<ReflectionFile::TypeDirectory*>(Reflect::getPointer(info, sizeof(ReflectionFile::InterfaceDirectory) +
                sizeof(u32) * (interfaceDirectory->numInterfaces - 1))))
    {
    }

    Interface getInterface(int n) const
    {
        ASSERT(0 <= n && n < getInterfaceCount());
        return Interface(info, interfaceDirectory->interfaces[n]);
    }

    Identifier getIdentifier(int n) const
    {
        ASSERT(0 <= n && n < getTypeCount());
        return Identifier(info, typeDirectory->types[n]);
    }

    int getInterfaceCount() const
    {
        return interfaceDirectory->numInterfaces;
    }

    int getTypeCount() const
    {
        return typeDirectory->numTypes;
    }
};

inline Reflect::Interface Reflect::Type::
getInterface()
{
    ASSERT(isInterface());
    return Reflect::Interface(info, offset);
}

#endif  // #ifndef NINTENDO_ES_REFLECT_H_INCLUDED
