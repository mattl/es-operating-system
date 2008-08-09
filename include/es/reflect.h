/*
 * Copyright 2008 Google Inc.
 * Copyright 2006, 2007 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_REFLECT2_H_INCLUDED
#define NINTENDO_ES_REFLECT2_H_INCLUDED

#include <es.h>
#include <es/ent.h>
#include <es/types.h>

/**
 * This class provides an access to the reflection data.
 */
class Reflect
{
    u8* ent;

public:

    /**
     * Gets the pointer of the specified record in the specified reflection data.
     * @param ent the reflection data.
     * @param offset the offset to the record.
     * @return the record.
     */
    static void* getPointer(u8* ent, u32 offset)
    {
        return static_cast<void*>(ent + offset);
    }

    class Interface;

    /**
     * This represents a type record from the specified reflection data.
     */
    class Type
    {
        u8* ent;
        u32 spec;

    public:
        /**
         * Constructs an object which represents the specified type.
         * @param ent the reflection data.
         * @param spec the offset to the type record.
         */
        Type(u8* ent, u32 spec) :
            ent(ent),
            spec(spec)
        {
        }

        /**
         * Gets the type of this type descriptor.
         */
        u32 getType() const
        {
            if (isPrimitive())
            {
                return spec;
            }

            return *static_cast<u32*>(getPointer(ent, spec));
        }

        /**
         * Checks if this type is primitive.
         */
        bool isPrimitive() const
        {
            return Ent::isPrimitive(spec);
        }

        /**
         * Checks if this type is a module.
         */
        bool isModule() const
        {
            return (getType() == Ent::TypeModule) ? true : false;
        }

        /**
         * Checks if this type is an interface.
         */
        bool isInterface() const
        {
            return (getType() == Ent::TypeInterface) ? true : false;
        }

        /**
         * Checks if this type is a structure.
         */
        bool isStructure() const
        {
            return (getType() == Ent::TypeStructure) ? true : false;
        }

        /**
         * Checks if this type is an exception.
         */
        bool isException() const
        {
            return (getType() == Ent::TypeException) ? true : false;
        }

        /**
         * Checks if this type is an enum.
         */
        bool isEnum() const
        {
            return (getType() == Ent::TypeEnum) ? true : false;
        }

        /**
         * Checks if this type is an array.
         */
        bool isArray() const
        {
            return (getType() == Ent::TypeArray) ? true : false;
        }

        /**
         * Checks if this type is a sequence.
         */
        bool isSequence() const
        {
            return (getType() == Ent::TypeSequence) ? true : false;
        }

        /**
         * Checks if this type is a character.
         */
        bool isCharacter() const
        {
            switch (spec)
            {
            case Ent::SpecChar:
            case Ent::SpecWChar:
                return true;
            default:
                return false;
            }
        }

        /**
         * Checks if this type is an integer.
         */
        bool isInteger() const
        {
            switch (spec)
            {
            case Ent::SpecS8:
            case Ent::SpecS16:
            case Ent::SpecS32:
            case Ent::SpecS64:
            case Ent::SpecU8:
            case Ent::SpecU16:
            case Ent::SpecU32:
            case Ent::SpecU64:
                return true;
            default:
                return false;
            }
        }

        /**
         * Checks if this type is a boolean.
         */
        bool isBoolean() const
        {
            return (spec == Ent::SpecBool) ? true : false;
        }

        /**
         * Checks if this type is a floating point number.
         */
        bool isFloat() const
        {
            switch (spec)
            {
            case Ent::SpecF32:
            case Ent::SpecF64:
            case Ent::SpecF128:
            case Ent::SpecFixed:
                return true;
            default:
                return false;
            }
        }

        /**
         * Checks if this type is an object.
         */
        bool isObject() const
        {
            return (spec == Ent::SpecObject) ? true : false;
        }

        /**
         * Checks if this type is any.
         */
        bool isAny() const
        {
            return (spec == Ent::SpecAny) ? true : false;
        }

        /**
         * Checks if this type is a string.
         */
        bool isString() const
        {
            return (spec == Ent::SpecString) ? true : false;
        }

        /**
         * Checks if this type is a reference to uuid.
         */
        bool isUuid() const
        {
            return (spec == Ent::SpecUuid) ? true : false;
        }

        /**
         * Gets the size of this type.
         */
        int getSize() const
        {
            switch (getType())
            {
            case Ent::SpecChar:
            case Ent::SpecS8:
            case Ent::SpecU8:
                return sizeof(u8);
            case Ent::SpecS16:
            case Ent::SpecU16:
                return sizeof(u16);
            case Ent::SpecS32:
            case Ent::SpecU32:
                return sizeof(u32);
            case Ent::SpecS64:
            case Ent::SpecU64:
                return sizeof(u64);
            case Ent::SpecF32:
                return sizeof(float);
            case Ent::SpecF64:
                return sizeof(double);
            case Ent::SpecF128:
                return sizeof(long double);
            case Ent::SpecBool:
                return sizeof(bool);
            case Ent::SpecWChar:
                return sizeof(wchar_t);
            case Ent::SpecVoid:
                return 0;
            case Ent::SpecUuid:
                return sizeof(Guid);
            case Ent::SpecAny:
            case Ent::SpecObject:
                return sizeof(void*);

            case Ent::TypeInterface:
                return sizeof(void*);
            case Ent::TypeArray:
                {
                    Array a(ent, spec);
                    return a.getSize();
                }
                break;
            case Ent::TypeStructure:
                {
                    Structure st(ent, spec);
                    return st.getSize();
                }
                break;
#if 0
            case Ent::TypeException:
                {
                    Exception ex(ent, spec);
                    return ex.getSize();
                }
                break;
#endif
            case Ent::TypeSequence:
                {
                    Sequence seq(ent, spec);
                    return seq.getSize();
                }
                break;
            default:
                ASSERT(0);  // XXX
                return 0;
            }
        }

        /**
         * Gets the interface of this type.
         */
        Interface getInterface()
        {
            ASSERT(isInterface());
            return Interface(ent, spec);
        }
    };

    /**
     * This represents a function parameter loaded from the specified reflection data.
     */
    class Parameter
    {
        u8* ent;
        const Ent::Param* param;

    public:
        /** Default constructor
         */
        Parameter() :
            ent(0),
            param(0)
        {
        }

        /**
         * Constructs an object which represents the specified parameter.
         * @param ent the reflection data.
         * @param param pointer to the parameter record.
         */
        Parameter(u8* ent, const Ent::Param* param) :
            ent(ent),
            param(param)
        {
        }

        /**
         * Checks if this parameter is passed from the calling procedure
         * to the called procedure.
         */
        bool isInput() const
        {
            return param->isInput();
        }

        /**
         * Checks if this parameter is returned from the called procedure
         * to the calling procedure.
         */
        bool isOutput() const
        {
            return param->isOutput();
        }

        /**
         * Checks if this parameter is both passed and returned from/to the
         * calling procedure to/from the called procedure.
         */
        bool isInOut() const
        {
            return param->isInOut();
        }

        /**
         * Gets the type of this parameter.
         */
        Type getType() const
        {
            return Type(ent, param->spec);
        }

        /**
         * Gets the name of this parameter.
         */
        const char* getName() const
        {
            return static_cast<const char*>(getPointer(ent, param->name));
        }
    };

    /**
     * This represents a function loaded from the specified reflection data.
     */
    class Method
    {
        u8* ent;
        Ent::Method* method;

    public:
        /** Default constructor
         */
        Method() :
            ent(0),
            method(0)
        {
        }

        /**
         * Constructs an object which represents the specified function.
         * @param ent the reflection data.
         * @param offset the offset to the function record.
         */
        Method(u8* ent, u32 spec) :
            ent(ent),
            method(static_cast<Ent::Method*>(getPointer(ent, spec)))
        {
        }

        /**
         * Copy-constructor.
         */
        Method(const Method& function) :
            ent(function.ent),
            method(function.method)
        {
        }

        /**
         * Gets the name of this function.
         */
        const char* getName() const
        {
            return static_cast<const char*>(getPointer(ent, method->name));
        }

        /**
         * Gets the type of the return value of this function.
         */
        Type getReturnType() const
        {
            return Type(ent, method->spec);
        }

        /**
         * Gets the number of arguments.
         */
        int getParameterCount() const
        {
            return method->paramCount;
        }

        /**
         * Gets the specified parameter.
         * @param n the parameter number.
         */
        Parameter getParameter(u32 n) const
        {
            return Parameter(ent, method->getParam(n));
        }

        bool isOperation() const
        {
            return method->isOperation();
        }

        bool isGetter() const
        {
            return method->isGetter();
        }

        bool isSetter() const
        {
            return method->isSetter();
        }

        bool isIndexGetter() const
        {
            return method->isIndexGetter();
        }

        bool isIndexSetter() const
        {
            return method->isIndexSetter();
        }

        bool isNameGetter() const
        {
            return method->isNameGetter();
        }

        bool isNameSetter() const
        {
            return method->isNameSetter();
        }
    };

    /**
     * This represents a structure/exception member loaded from the specified reflection data.
     */
    class Member
    {
        u8* ent;
        const Ent::Member* member;

    public:
        /** Default constructor
         */
        Member() :
            ent(0),
            member(0)
        {
        }

        /**
         * Constructs an object which represents the specified member.
         * @param ent the reflection data.
         * @param member pointer to the parameter record.
         */
        Member(u8* ent, const Ent::Member* member) :
            ent(ent),
            member(member)
        {
        }

        /**
         * Gets the type of this member.
         */
        Type getType() const
        {
            return Type(ent, member->spec);
        }

        /**
         * Gets the name of this member.
         */
        const char* getName() const
        {
            return static_cast<const char*>(getPointer(ent, member->name));
        }
    };

    /**
     * This represents a structure loaded from the specified reflection data.
     */
    class Structure
    {
        u8* ent;
        Ent::Structure* record;

    public:
        /**
         * Constructs a new object.
         */
        Structure() :
            ent(0),
            record(0)
        {
        }

        /**
         * Constructs a new object which represents the specified structure.
         * @param ent the reflection data.
         * @param offset the offset to the structure record.
         */
        Structure(u8* ent, u32 offset) :
            ent(ent),
            record(static_cast<Ent::Structure*>(getPointer(ent, offset)))
        {
        }

        /**
         * Copy-constructor.
         */
        Structure(const Structure& st) :
            ent(st.ent),
            record(st.record)
        {
        }

        /**
         * Gets the number of members in this interface.
         */
        int getMemberCount() const
        {
            return record->memberCount;
        }

        /**
         * Gets the specified member.
         * @param n the member number.
         */
        Member getMember(u32 n) const
        {
            return Member(ent, record->getMember(n));
        }

        /**
         * Gets the size of this type.
         */
        int getSize() const
        {
            int size = 0;
            for (int i = 0; i < getMemberCount(); ++i)
            {
                Member member(getMember(i));
                Type type(member.getType());
                size += type.getSize(); // XXX alignment
            }
            return size;
        }
    };

    /**
     * This represents a constant loaded from the specified reflection data.
     */
    class Constant
    {
        u8* ent;
        const Ent::Constant* c;

    public:
        /** Default constructor
         */
        Constant() :
            ent(0),
            c(0)
        {
        }

        /**
         * Constructs an object which represents the specified constant.
         * @param ent the reflection data.
         * @param c pointer to the parameter record.
         */
        Constant(u8* ent, const Ent::Constant* c) :
            ent(ent),
            c(c)
        {
        }

        /**
         * Gets the type of this constant.
         */
        Type getType() const
        {
            return Type(ent, c->spec);
        }

        /**
         * Gets the name of this constant.
         */
        const char* getName() const
        {
            return static_cast<const char*>(getPointer(ent, c->name));
        }

        template <typename T>
        T getValue() const
        {
            if (sizeof(T) <= sizeof(u32))
            {
                return *reinterpret_cast<const T*>(&c->value);
            }
            else
            {
                return *reinterpret_cast<const T*>(ent + c->value);
            }
        }

        const char* getString() const
        {
            return reinterpret_cast<const char*>(ent + c->value);
        }
    };

    /**
     * This represents an interface loaded from the specified reflection data.
     */
    class Interface
    {
        u8* ent;
        Ent::Interface* record;

    public:
        /**
         * Constructs a new object.
         */
        Interface() :
            ent(0),
            record(0)
        {
        }

        /**
         * Constructs a new object which represents the specified interface.
         * @param ent the reflection data.
         * @param offset the offset to the interface record.
         */
        Interface(u8* ent, u32 offset) :
            ent(ent),
            record(static_cast<Ent::Interface*>(getPointer(ent, offset)))
        {
        }

        /**
         * Copy-constructor.
         */
        Interface(const Interface& interface) :
            ent(interface.ent),
            record(interface.record)
        {
        }

        /**
         * Gets the name of this interface.
         */
        char* getName() const
        {
            return static_cast<char*>(getPointer(ent, record->name));
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
            return record->piid;
        }

        /**
         * Gets the number of methods in this interface.
         * @return the method count excluding super class methods.
         */
        int getMethodCount() const
        {
            return record->methodCount;
        }

        /**
         * Gets the specified method.
         * @param n the method number excluding super class methods.
         */
        Method getMethod(int n) const
        {
            ASSERT(0 <= n && n < getMethodCount());
            return Method(ent, record->getMethod(n));
        }

        /**
         * Gets the number of constants in this interface.
         */
        int getConstantCount() const
        {
            return record->constCount;
        }

        /**
         * Gets the specified constant.
         * @param n the constant number.
         */
        Constant getConstant(int n) const
        {
            ASSERT(0 <= n && n < getConstantCount());
            return Constant(ent, record->getConstant(n));
        }

        /**
         * Gets the number of super class methods.
         * @return the method count excluding this interface methods.
         */
        int getInheritedMethodCount() const
        {
            return record->inheritedMethodCount;
        }
    };

    /**
     * This represents a module loaded from the specified reflection data.
     */
    class Module
    {
        u8* ent;
        Ent::Module* record;

    public:
        /**
         * Constructs a new object.
         */
        Module() :
            ent(0),
            record(0)
        {
        }

        /**
         * Constructs a new object which represents the specified module.
         * @param ent the reflection data.
         * @param offset the offset to the module record.
         */
        Module(u8* ent, u32 offset) :
            ent(ent),
            record(static_cast<Ent::Module*>(getPointer(ent, offset)))
        {
        }

        /**
         * Copy-constructor.
         */
        Module(const Module& interface) :
            ent(interface.ent),
            record(interface.record)
        {
        }

        /**
         * Gets the name of this module.
         */
        char* getName() const
        {
            return static_cast<char*>(getPointer(ent, record->name));
        }

        /**
         * Gets the number of interfaces in this module.
         * @return the number of interfaces.
         */
        int getInterfaceCount() const
        {
            return record->interfaceCount;
        }

        /**
         * Gets the specified interface.
         * @param n the interface number.
         * @return the interface.
         */
        Interface getInterface(int n) const
        {
            ASSERT(0 <= n && n < getInterfaceCount());
            return Interface(ent, record->getInterface(n));
        }

        /**
         * Gets the number of constants in this module.
         */
        int getConstantCount() const
        {
            return record->constCount;
        }

        /**
         * Gets the specified constant.
         * @param n the constant number.
         */
        Constant getConstant(int n) const
        {
            ASSERT(0 <= n && n < getConstantCount());
            return Constant(ent, record->getConstant(n));
        }

        /**
         * Gets the number of modules in this module.
         * @return the number of modules.
         */
        int getModuleCount() const
        {
            return record->moduleCount;
        }

        /**
         * Gets the specified module.
         * @param n the module number.
         * @return the module.
         */
        Module getModule(int n) const
        {
            ASSERT(0 <= n && n < getModuleCount());
            return Module(ent, record->getModule(n));
        }
    };

    /**
     * This represents an array type loaded from the specified reflection data.
     */
    class Array
    {
        u8* ent;
        Ent::Array* record;

    public:
        /**
         * Constructs a new object.
         */
        Array() :
            ent(0),
            record(0)
        {
        }

        /**
         * Constructs a new object which represents the specified array.
         * @param ent the reflection data.
         * @param offset the offset to the array record.
         */
        Array(u8* ent, u32 offset) :
            ent(ent),
            record(static_cast<Ent::Array*>(getPointer(ent, offset)))
        {
        }

        /**
         * Gets the type of elements value of this array.
         */
        Type getType() const
        {
            return Type(ent, record->spec);
        }

        /**
         * Gets the number of dimensions of this array type.
         */
        u32 getDimension() const
        {
            return record->dim;
        }

        /**
         * Gets the rank of the specified dimension.
         * @param n the dimension number.
         */
        u32 getRank(u32 n) const
        {
            return record->getRank(n);
        }

        /**
         * Gets the size of this array type.
         */
        int getSize() const
        {
            Type type = getType();
            int size = type.getSize();
            for (u32 i = 0; i < getDimension(); ++i)
            {
                size *= getRank(i);
            }
            return size;
        }
    };

    /**
     * This represents a sequence type loaded from the specified reflection data.
     */
    class Sequence
    {
        u8* ent;
        Ent::Sequence* record;

    public:
        /**
         * Constructs a new object.
         */
        Sequence() :
            ent(0),
            record(0)
        {
        }

        /**
         * Constructs a new object which represents the specified sequence.
         * @param ent the reflection data.
         * @param offset the offset to the sequence record.
         */
        Sequence(u8* ent, u32 offset) :
            ent(ent),
            record(static_cast<Ent::Sequence*>(getPointer(ent, offset)))
        {
        }

        /**
         * Gets the type of elements value of this sequence.
         */
        Type getType() const
        {
            return Type(ent, record->spec);
        }

        /**
         * Gets the maximum number of elements in this sequence type.
         */
        u64 getMax() const
        {
            return record->max;
        }

        /**
         * Gets the size of the sequence of 'count' elements.
         * @param count the number of elements.
         */
        int getSize(int count = 1) const
        {
            Type type = getType();
            int size = type.getSize();
            return count * size;
        }
    };

    /**
     * Constructs a new reflection object.
     * @param ent the reflection data to be loaded.
     */
    Reflect(void* ent) :
        ent(static_cast<u8*>(ent))
    {
    }

    /**
     * Gets the global module.
     * @return the global module.
     */
    Module getGlobalModule() const
    {
        return Module(ent, sizeof(Ent::Header));
    }
};

#endif  // #ifndef NINTENDO_ES_REFLECT2_H_INCLUDED
