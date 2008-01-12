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

#ifndef NINTENDO_ES_IRF_H_INCLUDED
#define NINTENDO_ES_IRF_H_INCLUDED

//
// Interface reflection file format
//

#include <es/uuid.h>

namespace ReflectionFile
{
    /** An interface reflection file begins with a Header.
     */
    struct Header
    {
        char magic[4];      // magic number (0x7f, 'I', 'R', 'D')
        char data;          // data encoding (DATA2MSB)
        char reserved[3];
        Guid guid;          // revison
        u32  stringOffset;
    };

    /** The interface directory comes after the Header.
     */
    struct InterfaceDirectory
    {
        u32 numInterfaces;  // count of interface records
        u32 interfaces[1];  // an array of offsets to the interface records. The
                            // actual number of entries are given by numInterfaces.
    };

    /** The type directory comes after the interface directory.
     */
    struct TypeDirectory
    {
        u32 numTypes;       // count of type records
        u32 types[1];       // an array of offsets to the type records. The
                            // actual number of entries are given by numTypes.
    };

    struct NameRecord
    {
        u32 name;           // offset to the character string of this record
        u32 type;           // type of this record
    };

    struct FunctionRecord : public NameRecord
    {
        u32 returnType;
        u32 numParams;
        u32 params[1];
    };

    struct ConstantRecord : public NameRecord
    {
        u8 value[1];
    };

    struct StructureRecord : public NameRecord
    {
        u32 numMembers;     // count of members of this record
        u32 members[1];     // an array of offsets to the members. The
                            // actual number of entries are given by numMembers.
    };

    struct InterfaceRecord : public NameRecord
    {
        Guid iid;

        u32 numMethods;     // including super class methods.
        u32 numMembers;     // count of members of this record
        u32 members[1];     // an array of offsets to the members. The
                            // actual number of entries are given by numMembers.
    };

    /** The InterfaceMethods record comes after the interface record.
     */
    struct InterfaceMethods
    {
        Guid piid;          // Guid of the parent interface
        u32 numMethods;     // excluding super class methods.
        u32 methods[1];
    };

    struct InterfacePointerRecord : public NameRecord
    {
        u32 paramNum;       // method parameter number of Uuid type
    };

    struct EnumRecord : public NameRecord
    {
        u32 numEnums;
        struct
        {
            u32 name;
            u32 value;
        } enums[1];
    };

    struct ArrayRecord : public NameRecord
    {
        u32 typeAttributes;
        u32 numAttributes;
    };

    enum
    {
        TAG_S8 = 0,
        TAG_S16,
        TAG_S32,
        TAG_S64,
        TAG_U8,
        TAG_U16,
        TAG_U32,
        TAG_U64,
        TAG_F32,
        TAG_F64,
        TAG_BOOLEAN,
        TAG_CHAR,
        TAG_WIDECHAR,
        TAG_VOID,
        TAG_UUID,
        TAG_MAX,

        IS_STAR =      0x10000000,
        IS_STARSTAR =  0x20000000,
        IS_REFERENCE = 0x40000000,
        IS_PRIMITIVE = 0x80000000,

        IS_IN =        0x01000000,
        IS_OUT =       0x02000000,
        IS_IID =       0x04000000,
        IS_SIZE =      0x08000000,

        IS_STRUCTURE = 0x00100000,
        IS_INTERFACE = 0x00200000,
        IS_ENUM =      0x00300000,
        IS_CONSTANT =  0x00400000,
        IS_FUNCTION =  0x00500000,
        IS_ARRAY =     0x00600000,
        TYPE_MASK =    0x00f00000,
        OFFSET_MASK =  0x000fffff
    };

    enum
    {
        DATANONE, // Invalid data encoding
        DATA2LSB, // little endian
        DATA2MSB, // big endian
    };

};

#endif  // #ifndef NINTENDO_ES_IRF_H_INCLUDED
