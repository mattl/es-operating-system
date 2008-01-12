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

#include <es/endian.h>
#include <es/net/inet4.h>
#include <es/net/inet6.h>

const InAddr InAddrAny = { htonl(INADDR_ANY) };
const InAddr InAddrLoopback = { htonl(INADDR_LOOPBACK) };
const InAddr InAddrBroadcast = { htonl(INADDR_BROADCAST) };
const InAddr InAddrAllHost = { htonl(INADDR_ALLHOSTS_GROUP) };
const InAddr InAddrAllRouters = { htonl(INADDR_ALLROUTERS_GROUP) };
