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

#include <new>
#include "inet4.h"
#include "inet4address.h"

// StateNonMember

bool Inet4Address::
StateNonMember::input(InetMessenger* m, Inet4Address* a)
{
}

bool Inet4Address::
StateNonMember::output(InetMessenger* m, Inet4Address* a)
{
}

bool Inet4Address::
StateNonMember::error(InetMessenger* m, Inet4Address* a)
{
}

// StateDelayingMember

bool Inet4Address::
StateDelayingMember::input(InetMessenger* m, Inet4Address* a)
{
}

bool Inet4Address::
StateDelayingMember::output(InetMessenger* m, Inet4Address* a)
{
}

bool Inet4Address::
StateDelayingMember::error(InetMessenger* m, Inet4Address* a)
{
}

// StateIdleMember

bool Inet4Address::
StateIdleMember::input(InetMessenger* m, Inet4Address* a)
{
}

bool Inet4Address::
StateIdleMember::output(InetMessenger* m, Inet4Address* a)
{
}

bool Inet4Address::
StateIdleMember::error(InetMessenger* m, Inet4Address* a)
{
}
