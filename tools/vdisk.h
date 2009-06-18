/*
 * Copyright 2008, 2009 Google Inc.
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
#include <es/device/IDisk.h>

int esInit(Object** nameSpace);

class VDisk : public es::Disk
{
    struct Geometry
    {
        unsigned int heads;
        unsigned int cylinders;
        unsigned int sectorsPerTrack;
        unsigned int bytesPerSector;
        long long diskSize;
    };

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

        long long pos = lseek(fd, 0, SEEK_CUR);
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

        err = lseek(fd, pos, SEEK_SET);
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
        lseek(fd, 0, SEEK_END);
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

    unsigned int getHeads()
    {
        return geometry.heads;
    }
    unsigned int getCylinders()
    {
        return geometry.cylinders;
    }
    unsigned int getSectorsPerTrack()
    {
        return geometry.sectorsPerTrack;
    }
    unsigned int getBytesPerSector()
    {
        return geometry.bytesPerSector;
    }
    long long getDiskSize()
    {
        return geometry.diskSize;
    }

    //
    // IInterface
    //

    Object* queryInterface(const char* riid)
    {
        Object* objectPtr;
        if (strcmp(riid, es::Stream::iid()) == 0)
        {
            objectPtr = static_cast<es::Stream*>(this);
        }
        else if (strcmp(riid, es::Disk::iid()) == 0)
        {
            objectPtr = static_cast<es::Disk*>(this);
        }
        else if (strcmp(riid, Object::iid()) == 0)
        {
            objectPtr = static_cast<es::Stream*>(this);
        }
        else
        {
            return NULL;
        }
        objectPtr->addRef();
        return objectPtr;
    }

    unsigned int addRef()
    {
        return ref.addRef();
    }

    unsigned int release()
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
