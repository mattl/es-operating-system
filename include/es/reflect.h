/*
 * Copyright (c) 2006, 2007
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

/**
 * This class loads an interface reflection file.
 */
class Reflect
{
    u8* info;
    ReflectionFile::InterfaceDirectory* interfaceDirectory;
    ReflectionFile::TypeDirectory* typeDirectory;

    static const char* typeID[ReflectionFile::TAG_MAX];

public:

    /**
     * Gets the pointer of the specified record in the specified interface reflection file.
     * @param info the interface reflection file.
     * @param offset the offset to the record.
     * @return the record.
     */
    static void* getPointer(u8* info, u32 offset)
    {
        return static_cast<void*>(info + sizeof(ReflectionFile::Header) +
                (offset & ReflectionFile::OFFSET_MASK));
    }

    class Interface;
    class Structure;

    /**
     * This represents a type record from the specified interface reflection file.
     */
    class Type
    {
        u8* info;
        u32 offset;

    public:
        /**
         * Constructs an object which represents the specified type.
         * @param info the interface reflection file.
         * @param offset the offset to the type record.
         */
        Type(u8* info, u32 offset) :
            info(info),
            offset(offset)
        {
        }

        /**
         * Gets the type of this type descriptor.
         */
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

        /**
         * Checks if this type is const.
         */
        bool isConst() const
        {
            return (isPointer() || isReference()) && !(offset & ReflectionFile::IS_OUT);
        }

        /**
         * Checks if this type is primitive.
         */
        bool isPrimitive() const
        {
            return offset & ReflectionFile::IS_PRIMITIVE;
        }

        /**
         * Checks if this type is a pointer.
         */
        bool isPointer() const
        {
            return offset & (ReflectionFile::IS_STAR | ReflectionFile::IS_STARSTAR);
        }

        /**
         * Checks if this type is a reference.
         */
        bool isReference() const
        {
            return offset & ReflectionFile::IS_REFERENCE;
        }

        /**
         * Checks if this type is a structure.
         */
        bool isStructure() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_STRUCTURE;
        }

        /**
         * Checks if this type is an interface.
         */
        bool isInterface() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_INTERFACE;
        }

        /**
         * Checks if this type is an enum.
         */
        bool isEnum() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_ENUM;
        }

        /**
         * Checks if this type is a constant.
         */
        bool isConstant() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_CONSTANT;
        }

        /**
         * Checks if this type is a function.
         */
        bool isFunction() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_FUNCTION;
        }

        /**
         * Checks if this type is an array.
         */
        bool isArray() const
        {
            return (getType() & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_ARRAY;
        }

        /**
         * Checks if this type is a character string.
         */
        bool isString() const
        {
            if (!(offset & ReflectionFile::IS_STAR))
            {
                return false;
            }
            if (!isPrimitive())
            {
                return false;
            }
            if ((offset & ReflectionFile::OFFSET_MASK) != ReflectionFile::TAG_CHAR)
            {
                return false;
            }
            return true;
        }

        /**
         * Checks if this type is imported.
         */
        bool isImported() const
        {
            return offset & ReflectionFile::IS_IID;
        }

        /**
         * Gets the number of indirections in this type.
         * @return 0 if this is not a pointer. 1 if a pointer. 2 if a pointer to a pointer.
         */
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

        /**
         * Gets the name of this type.
         */
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

        /**
         * Gets the size of this type.
         */
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
            if (isInterface())
            {
                return sizeof(void*);
            }
            if (isStructure())
            {
                Structure st(info, offset);
                return st.getSize();
            }
            ASSERT(0);  // XXX
            return 0;
        }

        /**
         * Gets the size of object being referenced by this type.
         */
        int getReferentSize() const
        {
            ASSERT(isPointer() || isReference());
            if (1 < getPointer())
            {
                return sizeof(void*);
            }
            Type type(info, offset & ~(ReflectionFile::IS_REFERENCE | ReflectionFile::IS_STAR));
            return type.getSize();
        }

        /**
         * Checks if this type is an interface pointer.
         */
        bool isInterfacePointer() const
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

        /**
         * Checks if this type is an integer.
         */
        bool isInteger() const
        {
            if (isPointer() || isReference() || isArray() || isFunction())
            {
                return false;
            }
            if (isPrimitive())
            {
                switch (offset & ReflectionFile::OFFSET_MASK)
                {
                case ReflectionFile::TAG_S8:
                case ReflectionFile::TAG_U8:
                case ReflectionFile::TAG_S16:
                case ReflectionFile::TAG_U16:
                case ReflectionFile::TAG_S32:
                case ReflectionFile::TAG_U32:
                case ReflectionFile::TAG_S64:
                case ReflectionFile::TAG_U64:
                case ReflectionFile::TAG_CHAR:
                case ReflectionFile::TAG_WIDECHAR:
                    return true;
                default:
                    return false;
                }
            }
            return false;
        }

        /**
         * Checks if this type is a boolean.
         */
        bool isBoolean() const
        {
            if (isPointer() || isReference() || isArray() || isFunction())
            {
                return false;
            }
            if (isPrimitive())
            {
                switch (offset & ReflectionFile::OFFSET_MASK)
                {
                case ReflectionFile::TAG_BOOLEAN:
                    return true;
                default:
                    return false;
                }
            }
            return false;
        }

        /**
         * Checks if this type is a float.
         */
        bool isFloat() const
        {
            if (isPointer() || isReference() || isArray() || isFunction())
            {
                return false;
            }
            if (isPrimitive())
            {
                switch (offset & ReflectionFile::OFFSET_MASK)
                {
                case ReflectionFile::TAG_F32:
                case ReflectionFile::TAG_F64:
                    return true;
                default:
                    return false;
                }
            }
            return false;
        }

        /**
         * Checks if this type is a reference to uuid.
         */
        bool isUuid() const
        {
            if (!isReference())
            {
                return false;
            }
            if (!isPrimitive())
            {
                return false;
            }
            if ((offset & ReflectionFile::OFFSET_MASK) != ReflectionFile::TAG_UUID)
            {
                return false;
            }
            return true;
        }

        /**
         * Gets the interface of this type.
         */
        Interface getInterface()
        {
            ASSERT(isInterface());
            return Reflect::Interface(info, offset);
        }
    };

    /**
     * This represents an identifier loaded from the specified interface reflection file.
     */
    class Identifier
    {
        u8* info;
        u32 offset;
        ReflectionFile::NameRecord* record;

    public:
        /**
         * Constructs an object which represents the specified identifier.
         * @param info the interface reflection file.
         * @param offset the offset to the identifier record.
         */
        Identifier(u8* info, u32 offset) :
            info(info),
            offset(offset),
            record(static_cast<ReflectionFile::NameRecord*>(Reflect::getPointer(info, offset)))
        {
        }

        /**
         * Gets the type of this identifier.
         */
        Type getType() const
        {
            // IS_IN, IS_OUT, IS_IIS, IS_SIZE are set in offset.
            return Type(info, record->type | (offset & 0x07000000));
        }

        /**
         * Gets the name of this identifier.
         */
        char* getName() const
        {
            return static_cast<char*>(Reflect::getPointer(info, record->name));
        }

        /**
         * Checks if this identifier is passed from the calling procedure
         * to the called procedure.
         */
        bool isInput()
        {
            return offset & ReflectionFile::IS_IN;
        }

        /**
         * Checks if this identifier is returned from the called procedure
         * to the calling procedure.
         */
        bool isOutput()
        {
            return offset & ReflectionFile::IS_OUT;
        }

        /**
         * Gets the offset of the parameter which specifies the IID of this identifier.
         */
        int getIidIs() const
        {
            if (!(offset & ReflectionFile::IS_IID))
            {
                return -1;
            }
            u32* attr = (u32*) &record[1];
            return *attr;
        }

        /**
         * Gets the offset of the parameter which specifies the size of this identifier.
         */
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

        /**
         * Checks if this identifier is an interface pointer.
         */
        bool isInterfacePointer() const
        {
            return 0 <= getIidIs() || getType().isInterfacePointer();
        }
    };

    /**
     * This represents a function loaded from the specified interface reflection file.
     */
    class Function
    {
        u8* info;
        u32 offset;
        ReflectionFile::FunctionRecord* record;

    public:
        /** Default constructor
         */
        Function() :
            info(0),
            offset(0),
            record(0)
        {
        }

        /**
         * Constructs an object which represents the specified function.
         * @param info the interface reflection file.
         * @param offset the offset to the interface record.
         */
        Function(u8* info, u32 offset) :
            info(info),
            offset(offset),
            record(static_cast<ReflectionFile::FunctionRecord*>(Reflect::getPointer(info, offset)))
        {
            ASSERT((offset & ReflectionFile::TYPE_MASK) == ReflectionFile::IS_FUNCTION);
        }

        /**
         * Copy-constructor.
         */
        Function(const Function& function) :
            info(function.info),
            offset(function.offset),
            record(function.record)
        {
        }

        /**
         * Gets the type of this function.
         */
        Type getType() const
        {
            return Type(info, record->type);
        }

        /**
         * Gets the name of this function.
         */
        char* getName() const
        {
            return static_cast<char*>(Reflect::getPointer(info, record->name));
        }

        /**
         * Gets the type of the return value of this function.
         */
        Type getReturnType() const
        {
            return Type(info, record->returnType);
        }

        /**
         * Gets the number of arguments.
         */
        int getParameterCount() const
        {
            return record->numParams;
        }

        /**
         * Gets the specified parameter.
         * @param n the parameter number.
         */
        Identifier getParameter(int n) const
        {
            return Identifier(info, record->params[n]);
        }

        /**
         * Gets the offset of the specified parameter.
         * @param n the parameter number.
         */
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

    /**
     * This represents a structure loaded from the specified interface reflection file.
     */
    class Structure
    {
        u8* info;
        u32 offset;
        ReflectionFile::StructureRecord* record;

    public:
        /**
         * Constructs a new object.
         */
        Structure() :
            info(0),
            offset(0),
            record(0)
        {
        }

        /**
         * Constructs a new object which represents the specified structure.
         * @param info the interface reflection file.
         * @param offset the offset to the structure record.
         */
        Structure(u8* info, u32 offset) :
            info(info),
            offset(offset),
            record(static_cast<ReflectionFile::StructureRecord*>(Reflect::getPointer(info, offset)))
        {
        }

        /**
         * Copy-constructor.
         */
        Structure(const Structure& st) :
            info(st.info),
            offset(st.offset),
            record(st.record)
        {
        }

        /**
         * Gets the type of this interface.
         */
        Type getType() const
        {
            return Type(info, record->type);
        }

        /**
         * Gets the name of this interface.
         */
        char* getName() const
        {
            return static_cast<char*>(Reflect::getPointer(info, record->name));
        }

        /**
         * Gets the number of members in this interface.
         */
        int getMemberCount() const
        {
            return record->numMembers;
        }

        /**
         * Gets the specified member.
         * @param n the member number.
         */
        Identifier getMember(int n) const
        {
            return Identifier(info, record->members[n]);
        }

        /**
         * Gets the size of this type.
         */
        int getSize() const
        {
            int size = 0;
            for (int i = 0; i < getMemberCount(); ++i)
            {
                Identifier member(getMember(i));
                Type type(member.getType());
                size += type.getSize();
            }
            return size;
        }
    };

    /**
     * This represents an interface loaded from the specified interface reflection file.
     */
    class Interface
    {
        u8* info;
        u32 offset;
        ReflectionFile::InterfaceRecord* record;
        ReflectionFile::InterfaceMethods* methods;

    public:
        /**
         * Constructs a new object.
         */
        Interface() :
            info(0),
            offset(0),
            record(0),
            methods(0)
        {
        }

        /**
         * Constructs a new object which represents the specified interface.
         * @param info the interface reflection file.
         * @param offset the offset to the interface record.
         */
        Interface(u8* info, u32 offset) :
            info(info),
            offset(offset),
            record(static_cast<ReflectionFile::InterfaceRecord*>(Reflect::getPointer(info, offset))),
            methods(static_cast<ReflectionFile::InterfaceMethods*>(Reflect::getPointer(info, offset +
                    sizeof(ReflectionFile::InterfaceRecord) + sizeof(u32) * (record->numMembers - 1))))
        {
        }

        /**
         * Copy-constructor.
         */
        Interface(const Interface& interface) :
            info(interface.info),
            offset(interface.offset),
            record(interface.record),
            methods(interface.methods)
        {
        }

        /**
         * Gets the type of this interface.
         */
        Type getType() const
        {
            return Type(info, record->type);
        }

        /**
         * Gets the name of this interface.
         */
        char* getName() const
        {
            return static_cast<char*>(Reflect::getPointer(info, record->name));
        }

        /**
         * Gets the interface identifier of this interface.
         */
        Guid& getIid() const
        {
            return record->iid;
        }

        /**
         * Gets the identifier of the super interface.
         * @return the IID of the super interface. 0 if not inherited any interface.
         */
        Guid& getSuperIid() const
        {
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            return methods->piid;
        }

        /**
         * Gets the total number of methods in this interface.
         * @return the method count including super class methods.
         */
        int getTotalMethodCount() const
        {
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            return record->numMethods;
        }

        /**
         * Gets the number of methods in this interface.
         * @return the method count excluding super class methods.
         */
        int getMethodCount() const
        {
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            return methods->numMethods;
        }

        /**
         * Gets the specified method.
         * @param n the method number excluding super class methods.
         */
        Function getMethod(int n) const
        {
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            ASSERT(0 <= n && n < getMethodCount());
            return Function(info, methods->methods[n]);
        }

        /**
         * Gets the number of members in this interface.
         */
        int getMemberCount() const
        {
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            ASSERT(!(record->type & ReflectionFile::IS_IID));
            return record->numMembers;
        }
    };

    /**
     * Constructs a new object.
     * @param irf the interface reflection file to be loaded.
     */
    Reflect(void* irf) :
        info(static_cast<u8*>(irf)),
        interfaceDirectory(static_cast<ReflectionFile::InterfaceDirectory*>(Reflect::getPointer(info, 0))),
        typeDirectory(static_cast<ReflectionFile::TypeDirectory*>(Reflect::getPointer(info, sizeof(ReflectionFile::InterfaceDirectory) +
                sizeof(u32) * (interfaceDirectory->numInterfaces - 1))))
    {
    }

    /**
     * Gets the specified interface.
     * @param n the interface number.
     * @return the interface.
     */
    Interface getInterface(int n) const
    {
        ASSERT(0 <= n && n < getInterfaceCount());
        return Interface(info, interfaceDirectory->interfaces[n]);
    }

    /**
     * Gets the specified identifier
     * @param n the identifier number.
     * @return the identifier.
     */
    Identifier getIdentifier(int n) const
    {
        ASSERT(0 <= n && n < getTypeCount());
        return Identifier(info, typeDirectory->types[n]);
    }

    /**
     * Gets the number of interfaces in the interface reflection file.
     * @return the number of interfaces.
     */
    int getInterfaceCount() const
    {
        return interfaceDirectory->numInterfaces;
    }

    /**
     * Gets the number of types in the interface reflection file.
     * @return the number of types.
     */
    int getTypeCount() const
    {
        return typeDirectory->numTypes;
    }
};

#endif  // #ifndef NINTENDO_ES_REFLECT_H_INCLUDED
