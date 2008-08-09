/*
 * Copyright 2008 Google Inc.
 * Copyright 2007 Nintendo Co., Ltd.
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

#ifndef NINTENDO_ES_ENT_H_INCLUDED
#define NINTENDO_ES_ENT_H_INCLUDED

#include <assert.h>
#include <new>
#include <es/uuid.h>

// Amend namespace pollution
#ifdef minor
#undef minor
#endif
#ifdef major
#undef major
#endif

namespace Ent
{
    // Primitive types
    typedef u32 Spec;
    static const Spec SpecPrimitive = 0x80000000u;
    static const Spec SpecS8 = SpecPrimitive | 0u;
    static const Spec SpecS16 = SpecPrimitive | 1u;
    static const Spec SpecS32 = SpecPrimitive | 2u;
    static const Spec SpecS64 = SpecPrimitive | 3u;
    static const Spec SpecU8 = SpecPrimitive | 4u;
    static const Spec SpecU16 = SpecPrimitive | 5u;
    static const Spec SpecU32 = SpecPrimitive | 6u;
    static const Spec SpecU64 = SpecPrimitive | 7u;
    static const Spec SpecF32 = SpecPrimitive | 8u;
    static const Spec SpecF64 = SpecPrimitive | 9u;
    static const Spec SpecF128 = SpecPrimitive | 10u;
    static const Spec SpecBool = SpecPrimitive | 11u;
    static const Spec SpecChar = SpecPrimitive | 12u;
    static const Spec SpecWChar = SpecPrimitive | 13u;
    static const Spec SpecVoid = SpecPrimitive | 14u;
    static const Spec SpecUuid = SpecPrimitive | 15u;
    static const Spec SpecString = SpecPrimitive | 16u;
    static const Spec SpecWString = SpecPrimitive | 17u;
    static const Spec SpecAny = SpecPrimitive | 18u;
    static const Spec SpecObject = SpecPrimitive | 19u;
    static const Spec SpecFixed = SpecPrimitive | 20u;
    static const Spec SpecValue = SpecPrimitive | 21u;
    static const Spec SpecVariant = SpecPrimitive | 22u;
    static const int MaxSpec = 23;

    // Non-primitive types
    typedef u32 Type;
    static const Type TypeModule = 0u;
    static const Type TypeInterface = 1u;
    static const Type TypeStructure = 2u;
    static const Type TypeException = 3u;
    static const Type TypeEnum = 4u;
    static const Type TypeArray = 5u;
    static const Type TypeSequence = 6u;
    static const int MaxType = 7;

    static bool isPrimitive(Spec spec)
    {
        return (spec & SpecPrimitive) ? true : false;
    }

    /** An interface reflection file begins with a Header.
     */
    struct Header
    {
        static const u8 Major = 0;
        static const u8 Minor = 1;
        static const u8 Patch = 0;

        char magic[4];       // magic number (0x7f, 'E', 'N', 'T')
        u8   major;
        u8   minor;
        u8   patch;
        u8   reserved;
        u32  fileSize;
        // Module global;

        Header(size_t fileSize) :
            major(Major),
            minor(Minor),
            patch(Patch),
            reserved(0u),
            fileSize(fileSize)
        {
            magic[0] = 0x7f,
            magic[1] = 'E';
            magic[2] = 'N';
            magic[3] = 'T';
        }
    };

    struct Constant
    {
        Spec spec;
        u32  name;
        u32  value;

        Constant(Spec spec, u32 name, u32 value) :
            spec(spec),
            name(name),
            value(value)
        {
            assert(isPrimitive(spec));
        }

        static size_t getSize()
        {
            return sizeof(Constant);
        }
    };

    /** The Module record for the global name space comes after the Header.
     */
    struct Module
    {
        Type type;          // TypeModule
        u32  name;
        Spec parent;
        u32  moduleCount;
        u32  interfaceCount;
        u32  constCount;
        // Spec modules[moduleCount];
        // Spec interfaces[interfaceCount];
        // Constant consts[constCount];

        Module(u32 name, Spec parent, u32 moduleCount, u32 interfaceCount, u32 constCount) :
            type(TypeModule),
            name(name),
            parent(parent),
            moduleCount(moduleCount),
            interfaceCount(interfaceCount),
            constCount(constCount)
        {
        }

        void addModule(Spec spec)
        {
            assert(spec);
            Spec* p = reinterpret_cast<Spec*>(&this[1]);
            u32 i;
            for (i = 0; i < moduleCount; ++i, ++p)
            {
                if (*p == 0)
                {
                    *p = spec;
                    break;
                }
            }
            assert(i < moduleCount);
        }

        Spec getModule(u32 index) const
        {
            assert(index < moduleCount);
            const Spec* p = reinterpret_cast<const Spec*>(&this[1]);
            return p[index];
        }

        void addInterface(Spec spec)
        {
            assert(spec);
            Spec* p = reinterpret_cast<Spec*>(&this[1]);
            p += moduleCount;
            u32 i;
            for (i = 0; i < interfaceCount; ++i, ++p)
            {
                if (*p == 0)
                {
                    *p = spec;
                    break;
                }
            }
            assert(i < interfaceCount);
        }

        Spec getInterface(u32 index) const
        {
            assert(index < interfaceCount);
            const Spec* p = reinterpret_cast<const Spec*>(&this[1]);
            return p[moduleCount + index];
        }

        void addConstant(Spec spec, u32 name, u32 value)
        {
            assert(spec);
            Spec* p = reinterpret_cast<Spec*>(&this[1]);
            p += moduleCount;
            p += interfaceCount;
            Constant* c = reinterpret_cast<Constant*>(p);
            u32 i;
            for (i = 0; i < constCount; ++i, ++p)
            {
                if (c->spec == 0)
                {
                    new(c) Constant(spec, name, value);
                    break;
                }
            }
            assert(i < constCount);
        }

        const Constant* getConstant(u32 index) const
        {
            assert(index < constCount);
            const Spec* p = reinterpret_cast<const Spec*>(&this[1]);
            p += moduleCount;
            p += interfaceCount;
            const Constant* c = reinterpret_cast<const Constant*>(p);
            return c + index;
        }

        static size_t getSize(u32 moduleCount, u32 interfaceCount, u32 constCount)
        {
            return sizeof(Module) +
                   sizeof(Spec) * (moduleCount + interfaceCount) +
                   sizeof(Constant) * constCount;
        }
    };

    struct Interface
    {
        Type type;          // TypeInterface
        u32  name;
        Guid iid;
        Guid piid;
        Spec module;
        u32  methodCount;
        u32  constCount;
        u32  inheritedMethodCount;
        // Spec methods[methodCount];
        // Constant consts[constCount];

        Interface(u32 name, Guid& iid, Guid& piid, Spec module,
                  u32 methodCount, u32 constCount, u32 inheritedMethodCount) :
            type(TypeInterface),
            name(name),
            iid(iid),
            piid(piid),
            module(module),
            methodCount(methodCount),
            constCount(constCount),
            inheritedMethodCount(inheritedMethodCount)
        {
        }

        void addMethod(Spec spec)
        {
            assert(spec);
            Spec* p = reinterpret_cast<Spec*>(&this[1]);
            u32 i;
            for (i = 0; i < methodCount; ++i, ++p)
            {
                if (*p == 0)
                {
                    *p = spec;
                    break;
                }
            }
            assert(i < methodCount);
        }

        Spec getMethod(u32 index) const
        {
            assert(index < methodCount);
            const Spec* p = reinterpret_cast<const Spec*>(&this[1]);
            return p[index];
        }

        void addConstant(Spec spec, u32 name, u32 value)
        {
            assert(spec);
            Spec* p = reinterpret_cast<Spec*>(&this[1]);
            p += methodCount;
            Constant* c = reinterpret_cast<Constant*>(p);
            u32 i;
            for (i = 0; i < constCount; ++i, ++p)
            {
                if (c->spec == 0)
                {
                    new(c) Constant(spec, name, value);
                    break;
                }
            }
            assert(i < constCount);
        }

        const Constant* getConstant(u32 index) const
        {
            assert(index < constCount);
            const Spec* p = reinterpret_cast<const Spec*>(&this[1]);
            p += methodCount;
            const Constant* c = reinterpret_cast<const Constant*>(p);
            return c + index;
        }

        static size_t getSize(u32 methodCount, u32 constCount)
        {
            return sizeof(Interface) +
                   sizeof(Spec) * methodCount +
                   sizeof(Constant) * constCount;
        }
    };

    struct Param
    {
        static const u32 AttrIn = 0u;
        static const u32 AttrOut = 1u;
        static const u32 AttrInOut = 2u;

        Spec spec;
        u32  name;
        u32  attr;

        bool isInput() const
        {
            return (attr == AttrIn) ? true : false;
        }

        bool isOutput() const
        {
            return (attr == AttrOut) ? true : false;
        }

        bool isInOut() const
        {
            return (attr == AttrInOut) ? true : false;
        }
    };

    struct Method
    {
        static const u32 AttrNone = 0u;
        static const u32 AttrGetter = 1u;
        static const u32 AttrSetter = 2u;
        static const u32 IndexGetter = 3u;
        static const u32 IndexSetter = 4u;
        static const u32 NameGetter = 5u;
        static const u32 NameSetter = 6u;

        Spec spec;          // return type
        u32  name;
        u32  attr;
        u32  paramCount;
        u32  raiseCount;
        // Param params[paramCount];
        // Spec raises[raiseCount];

        Method(Spec spec, u32 name, u32 attr, u32 paramCount, u32 raiseCount) :
            spec(spec),
            name(name),
            attr(attr),
            paramCount(paramCount),
            raiseCount(raiseCount)
        {
        }

        void addParam(Spec spec, u32 name, u32 attr)
        {
            assert(spec);
            Param* p = reinterpret_cast<Param*>(&this[1]);
            u32 i;
            for (i = 0; i < paramCount; ++i, ++p)
            {
                if (p->spec == 0)
                {
                    p->spec = spec;
                    p->name = name;
                    p->attr = attr;
                    break;
                }
            }
            assert(i < paramCount);
        }

        const Param* getParam(u32 index) const
        {
            assert(index < paramCount);
            const Param* p = reinterpret_cast<const Param*>(&this[1]);
            return p + index;
        }

        void addRaise(Spec spec)
        {
            Param* params = reinterpret_cast<Param*>(&this[1]);
            Spec* raises = reinterpret_cast<Spec*>(&params[paramCount]);
            u32 i;
            for (i = 0; i < raiseCount; ++i, ++raises)
            {
                if (*raises == 0)
                {
                    *raises = spec;
                    break;
                }
            }
            assert(i < raiseCount);
        }

        Spec getRaise(u32 index) const
        {
            assert(index < raiseCount);
            const Param* params = reinterpret_cast<const Param*>(&this[1]);
            const Spec* raises = reinterpret_cast<const Spec*>(&params[paramCount]);
            return raises[index];
        }

        static size_t getSize(u32 paramCount, u32 raiseCount)
        {
            return sizeof(Method) +
                   sizeof(Param) * paramCount +
                   sizeof(Spec) * raiseCount;
        }

        bool isOperation() const
        {
            if ((attr == AttrNone)
                || (attr == IndexGetter)
                || (attr == IndexSetter)
                || (attr == NameGetter)
                || (attr == NameSetter))
            {
                return true;
            }
            return false;
        }

        bool isGetter() const
        {
            return (attr == AttrGetter) ? true : false;
        }

        bool isSetter() const
        {
            return (attr == AttrSetter) ? true : false;
        }

        bool isIndexGetter() const
        {
            return (attr == IndexGetter) ? true : false;
        }

        bool isIndexSetter() const
        {
            return (attr == IndexSetter) ? true : false;
        }

        bool isNameGetter() const
        {
            return (attr == NameGetter) ? true : false;
        }

        bool isNameSetter() const
        {
            return (attr == NameSetter) ? true : false;
        }

    };

    struct Member
    {
        Spec spec;
        u32  name;
    };

    struct Structure
    {
        Type type;          // TypeStructure
        u32  memberCount;
        // Member members[memberCount];

        Structure(u32 memberCount) :
            type(TypeStructure),
            memberCount(memberCount)
        {
        }

        void addMember(Spec spec, u32 name)
        {
            assert(spec);
            Member* p = reinterpret_cast<Member*>(&this[1]);
            u32 i;
            for (i = 0; i < memberCount; ++i, ++p)
            {
                if (p->spec == 0)
                {
                    p->spec = spec;
                    p->name = name;
                    break;
                }
            }
            assert(i < memberCount);
        }

        const Member* getMember(u32 index) const
        {
            assert(index < memberCount);
            const Member* p = reinterpret_cast<const Member*>(&this[1]);
            return p + index;
        }

        static size_t getSize(u32 memberCount)
        {
            return sizeof(Structure) + sizeof(Member) * memberCount;
        }
    };

    struct Exception
    {
        Type type;          // TypeException
        u32  memberCount;
        // Member members[memberCount];

        Exception(u32 memberCount) :
            type(TypeException),
            memberCount(memberCount)
        {
        }

        void addMember(Spec spec, u32 name)
        {
            assert(spec);
            Member* p = reinterpret_cast<Member*>(&this[1]);
            u32 i;
            for (i = 0; i < memberCount; ++i, ++p)
            {
                if (p->spec == 0)
                {
                    p->spec = spec;
                    p->name = name;
                    break;
                }
            }
            assert(i < memberCount);
        }

        const Member* getMember(u32 index) const
        {
            const Member* p = reinterpret_cast<const Member*>(&this[1]);
            return p + index;
        }

        static size_t getSize(u32 memberCount)
        {
            return sizeof(Exception) + sizeof(Member) * memberCount;
        }
    };

    struct Enum
    {
        Type type;          // TypeEnum
        u32  enumCount;
        // u32 enums[enumCount];

        Enum(u32 enumCount) :
            type(TypeEnum),
            enumCount(enumCount)
        {
        }

        void add(u32 name)
        {
            u32* p = reinterpret_cast<u32*>(&this[1]);
            u32 i;
            for (i = 0; i < enumCount; ++i, ++p)
            {
                if (*p == 0)
                {
                    *p = name;
                    break;
                }
            }
            assert(i < enumCount);
        }

        u32 get(u32 index) const
        {
            assert(index < enumCount);
            const u32* p = reinterpret_cast<const u32*>(&this[1]);
            return p[index];
        }

        static size_t getSize(u32 enumCount)
        {
            return sizeof(Enum) + sizeof(u32) * enumCount;
        }
    };

    struct Array
    {
        Type type;          // TypeArray
        Spec spec;
        u32  dim;
        // u32 rank[dim];

        Array(Spec spec, u32 dim) :
            type(TypeArray),
            spec(spec),
            dim(dim)
        {
            assert(0 < dim);
        }

        void setRank(u32 rank)
        {
            u32* p = reinterpret_cast<u32*>(&this[1]);
            u32 i;
            for (i = 0; i < dim; ++i, ++p)
            {
                if (*p == 0)
                {
                    *p = rank;
                    break;
                }
            }
            assert(i < dim);
        }

        u32 getRank(u32 index) const
        {
            assert(index < dim);
            const u32* p = reinterpret_cast<const u32*>(&this[1]);
            return p[index];
        }

        static size_t getSize(u32 dim)
        {
            return sizeof(Array) + sizeof(u32) * dim;
        }
    };

    struct Sequence
    {
        Type type;          // TypeSequence
        Spec spec;
        u64  max;

        Sequence(Spec spec, u64 max = 0) :
            type(TypeSequence),
            spec(spec),
            max(max)
        {
        }

        static size_t getSize()
        {
            return sizeof(Sequence);
        }
    };
}

#endif  // #ifndef NINTENDO_ES_ENT_H_INCLUDED
