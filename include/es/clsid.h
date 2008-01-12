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

#ifndef NINTENDO_ES_CLSID_H_INCLUDED
#define NINTENDO_ES_CLSID_H_INCLUDED

#include <es/base/IClassStore.h>

#ifdef __cplusplus
extern "C" {
#endif

/** <code>3979e8b4-2e86-11db-9c02-0009bf000001</code>
 */
const Guid CLSID_Process =
{
    0x3979e8b4, 0x2e86, 0x11db, { 0x9c, 0x02, 0x00, 0x09, 0xbf, 0x00, 0x00, 0x01 }
};

/** <code>397a9b6a-2e86-11db-9c02-0009bf000001</code>
 */
const Guid CLSID_CacheFactory =
{
    0x397a9b6a, 0x2e86, 0x11db, { 0x9c, 0x02, 0x00, 0x09, 0xbf, 0x00, 0x00, 0x01 }
};

/** <code>397b45f6-2e86-11db-9c02-0009bf000001</code>
 */
const Guid CLSID_Monitor =
{
    0x397b45f6, 0x2e86, 0x11db, { 0x9c, 0x02, 0x00, 0x09, 0xbf, 0x00, 0x00, 0x01 }
};

/** <code>397be8bc-2e86-11db-9c02-0009bf000001</code>
 */
const Guid CLSID_PageSet =
{
    0x397be8bc, 0x2e86, 0x11db, { 0x9c, 0x02, 0x00, 0x09, 0xbf, 0x00, 0x00, 0x01 }
};

/** <code>397c8ccc-2e86-11db-9c02-0009bf000001</code>
 */
const Guid CLSID_Alarm =
{
    0x397c8ccc, 0x2e86, 0x11db, { 0x9c, 0x02, 0x00, 0x09, 0xbf, 0x00, 0x00, 0x01 }
};

/** <code>397d44f0-2e86-11db-9c02-0009bf000001</code>
 */
const Guid CLSID_Partition =
{
    0x397d44f0, 0x2e86, 0x11db, { 0x9c, 0x02, 0x00, 0x09, 0xbf, 0x00, 0x00, 0x01 }
};

/** <code>397dec0c-2e86-11db-9c02-0009bf000001</code>
 */
const Guid CLSID_FatFileSystem =
{
    0x397dec0c, 0x2e86, 0x11db, { 0x9c, 0x02, 0x00, 0x09, 0xbf, 0x00, 0x00, 0x01 }
};

/** <code>397e8e00-2e86-11db-9c02-0009bf000001</code>
 */
const Guid CLSID_IsoFileSystem =
{
    0x397e8e00, 0x2e86, 0x11db, { 0x9c, 0x02, 0x00, 0x09, 0xbf, 0x00, 0x00, 0x01 }
};

void esRegisterFatFileSystemClass(IClassStore* classStore);
void esRegisterIsoFileSystemClass(IClassStore* classStore);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // #ifndef NINTENDO_ES_CLSID_H_INCLUDED
