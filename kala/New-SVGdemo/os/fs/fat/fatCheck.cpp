/*
 * Copyright 2008, 2009 Google Inc.
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

/*
 * These coded instructions, statements, and computer programs contain
 * software derived from the following specification:
 *
 * Microsoft, "Microsoft Extensible Firmware Initiative FAT32 File System
 * Specification," 6 Dec. 2000.
 * http://www.microsoft.com/whdc/system/platform/firmware/fatgen.mspx
 */

#include <string.h>
#include <es.h>
#include <es/handle.h>
#include "fatStream.h"

bool FatStream::
check(u8* clusRefs)
{
    bool result = true;
    u64 chainLen = 0;
    u32 clus = fstClus;

    esReport("Cheking '%.11s'...\n", fcb);

    if (isDirectory())
    {
        if (fstClus == 0)
        {
            if (fileSystem->isFat32() || !isRoot())
            {
                esReport("Empty directory: '%.11s'.\n", fcb);
                return false;
            }
            goto SkipChainCheck;
        }
    }
    else if (fstClus == 0)
    {
        if (size != 0)
        {
            esReport("No cluster: A clustor is not assigned to non-empty '%.11s'.\n", fcb);
            return false;
        }
        return true;
    }

    while (!fileSystem->isEof(clus))
    {
        if (clus < 2 || fileSystem->isBadCluster(clus) || fileSystem->countOfClusters + 2 <= clus)
        {
            esReport("Bad chain: A bad chain %d is found in '%.11s'.\n",
                     clus, fcb);
            return false;
        }
        if (2 <= ++clusRefs[clus])
        {
            esReport("Bad chain: cluster %d is referenced more than once from '%.11s'.\n",
                     clus, fcb);
            return false;
        }
        chainLen += fileSystem->bytsPerClus;
        clus = fileSystem->clusEntryVal(clus);
        if (((size + fileSystem->bytsPerClus - 1) & ~(fileSystem->bytsPerClus - 1)) < chainLen)
        {
            break;
        }
    }

    if (((size + fileSystem->bytsPerClus - 1) & ~(fileSystem->bytsPerClus - 1)) != chainLen)
    {
        esReport("Size mismatch: file size %u and the cluster chain length %llu do not match in '%.11s'.\n",
                 size, chainLen, fcb);
        return false;
    }

SkipChainCheck:
    if (isDirectory())
    {
        // XXX clean stale FCB entries.
        Handle<es::Stream> dir(cache->getStream());
        if (!isRoot())
        {
            dir->setPosition(2 * 32);
        }
        for (;;)
        {
            u8 fcb[32];
            u16 longName[256];
            if (!findNext(dir, fcb, longName))
            {
                break;
            }
            long long pos;
            pos = dir->getPosition();
            FatStream* next = fileSystem->lookup(fstClus, pos - 32);
            if (!next)
            {
                next = new FatStream(fileSystem, this, pos - 32, fcb);
            }
            if (!next->check(clusRefs))
            {
                result = false;
            }
            next->release();
        }
    }

    return result;
}

// Check this file system.
bool FatFileSystem::
check()
{
    bool clean = true;

    // 1) Each file has correct number of clusters as specificed in its
    //    FCB.

    // 2) Accumelate orphan clusters and make them free. i.e. Mark
    //    clusters whose next cluster number is zero.

    // 3) Make sure each clusters are referenced at most once. If not,
    //    determine files and directories that share the same set of
    //    clusters.

    // clusRefs holds the reference count of each cluster.
    u8* clusRefs = new u8[countOfClusters + 2];
    memset(clusRefs, 0, countOfClusters + 2);
    clusRefs[0] = clusRefs[1] = 1;

    u32 n;
    for (n = 2; n < countOfClusters + 2; ++n)
    {
        u32 next = clusEntryVal(n);
        if (next == 0 || isBadCluster(next))
        {
            ++clusRefs[n];
        }
        if (next == 1 || !isEof(next) && countOfClusters + 2 <= next)
        {
            esReport("Out of range: The cluster %d is linked to %d.\n",
                     n, next);
        }
    }

    clean = root->check(clusRefs);

    delete[] clusRefs;

    return clean;
}
