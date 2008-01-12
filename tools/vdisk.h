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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <es.h>
#include <es/ref.h>
#include <es/endian.h>
#include <es/base/IStream.h>
#include <es/device/IDiskManagement.h>

int esInit(IInterface** nameSpace);

class VDisk : public IStream, public IDiskManagement
{
    Ref      ref;
    int      fd;
    Geometry geometry;
    bool     vhd;

public:
    VDisk(char* vdisk) :
        vhd(false)
    {
        geometry.cylinders = 0;
        geometry.heads = 0;
        geometry.sectorsPerTrack = 0;
        geometry.bytesPerSector = 0;
        geometry.diskSize = 0;

        fd = open(vdisk, O_RDWR);
        if (fd < 0)
        {
            fd = open(vdisk, O_RDWR | O_CREAT, 0777);
            if (fd < 0)
            {
                esThrow(ENOSPC);
            }
            setSize(512 * 2880);
        }
        long long size;
        size = getSize();
        if (size <= 512 * 2880 * 2)
        {
            switch (size / 512)
            {
            case 2880:  // 3-1/2 2HD
                geometry.cylinders = 80;
                geometry.heads = 2;
                geometry.sectorsPerTrack = 18;
                geometry.bytesPerSector = 512;
                geometry.diskSize = size;
                break;
            case 1440:  // 3-1/2 2DD
                geometry.cylinders = 80;
                geometry.heads = 2;
                geometry.sectorsPerTrack = 9;
                geometry.bytesPerSector = 512;
                geometry.diskSize = size;
                break;
            default:
                geometry.cylinders = 0;
                geometry.heads = 0;
                geometry.sectorsPerTrack = 0;
                geometry.bytesPerSector = 512;
                geometry.diskSize = size;
                break;
            }
        }
        else
        {
            u8 sig[8];
            read(sig, 8, size - 512);
            if (memcmp(sig, "conectix", 8) == 0)
            {
                // VPC 2004 vhd format
                vhd = true;
                u8 chs[4];
                read(chs, 4, size - 512 + 0x38);
                geometry.cylinders = BigEndian::word(chs + 0);
                geometry.heads = BigEndian::byte(chs + 2);
                geometry.sectorsPerTrack = BigEndian::byte(chs + 3);
                geometry.bytesPerSector = 512;
                geometry.diskSize = size - 512;
            }
            else
            {
                geometry.cylinders = 0;
                geometry.heads = 0;
                geometry.sectorsPerTrack = 0;
                geometry.bytesPerSector = 512;
                geometry.diskSize = size;
            }
            setPosition(0);
        }
        esReport("CHS: %u %u %u\n",
                 geometry.cylinders, geometry.heads, geometry.sectorsPerTrack);
    }

    VDisk(char* vdisk, unsigned int cylinders, unsigned int heads, unsigned int sectorsPerTrack) :
        vhd(false)
    {
        geometry.cylinders = cylinders;
        geometry.heads = heads;
        geometry.sectorsPerTrack = sectorsPerTrack;
        geometry.bytesPerSector = 512;
        geometry.diskSize = geometry.bytesPerSector * cylinders * heads * sectorsPerTrack;

        fd = open(vdisk, O_RDWR);
        if (fd < 0)
        {
            fd = open(vdisk, O_RDWR | O_CREAT, 0777);
            if (fd < 0)
            {
                esThrow(ENOSPC);
            }
            setSize(geometry.diskSize);
        }
        setPosition(0);

        esReport("CHS: %u %u %u\n",
                 geometry.cylinders, geometry.heads, geometry.sectorsPerTrack);
    }

    ~VDisk()
    {
        close(fd);
    }

    //
    // IStream
    //

    long long getPosition()
    {
        int err;

        long long pos = lseek64(fd, 0, SEEK_CUR);
        if (pos < 0)
        {
            esThrow(errno);
        }
        return pos;
    }

    void setPosition(long long pos)
    {
        int err;

        if (geometry.diskSize && (pos % 512))
        {
            esThrow(EINVAL);
        }

        err = lseek64(fd, pos, SEEK_SET);
        if (err < 0)
        {
            esThrow(errno);
        }
    }

    long long getSize()
    {
        long long tmp;
        long long size;

        tmp = getPosition();
        lseek64(fd, 0, SEEK_END);
        size = getPosition();
        setPosition(tmp);
        if (vhd)
        {
            size -= 512;
        }
        return size;
    }

    void setSize(long long size)
    {
        int err;

        err = ftruncate(fd, size);
        if (err)
        {
            esThrow(errno);
        }
    }

    int read(void* buffer, int size)
    {
        esThrow(EINVAL);

        size_t n = ::read(fd, buffer, size);
        return (int) n;
    }

    int read(void* buffer, int size, long long offset)
    {
#ifdef VERBOSE
        esReport("vdisk::read %ld byte at 0x%llx to %p.\n", size, offset, buffer);
#endif
        setPosition(offset);
        size_t n = ::read(fd, buffer, size);
        return (int) n;
    }

    int write(const void* buffer, int size)
    {
        esThrow(EINVAL);

        size_t n = ::write(fd, buffer, size);
        return (int) n;
    }

    int write(const void* buffer, int size, long long offset)
    {
#ifdef VERBOSE
        esReport("vdisk::write %ld byte at 0x%llx from %p.\n", size, offset, buffer);
#endif
        setPosition(offset);
        size_t n = ::write(fd, buffer, size);
        return (int) n;
    }

    void flush()
    {
    }

    //
    // IDiskManagement
    //

    int initialize()
    {
        return -1;
    }

    int getGeometry(Geometry* geometry)
    {
        memmove(geometry, &this->geometry, sizeof(Geometry));
        return 0;
    }

    int getLayout(Partition* partition)
    {
        return -1;
    }

    int setLayout(Partition* partition)
    {
        return -1;
    }

    //
    // IInterface
    //

    bool queryInterface(const Guid& riid, void** objectPtr)
    {
        if (riid == IID_IStream)
        {
            *objectPtr = static_cast<IStream*>(this);
        }
        else if (riid == IID_IDiskManagement)
        {
            *objectPtr = static_cast<IDiskManagement*>(this);
        }
        else if (riid == IID_IInterface)
        {
            *objectPtr = static_cast<IStream*>(this);
        }
        else
        {
            *objectPtr = NULL;
            return false;
        }
        static_cast<IInterface*>(*objectPtr)->addRef();
        return true;
    }

    unsigned int addRef(void)
    {
        return ref.addRef();
    }

    unsigned int release(void)
    {
        unsigned int count = ref.release();
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }
};
