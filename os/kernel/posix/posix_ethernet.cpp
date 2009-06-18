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

#ifdef __linux__

#include "posix/tap.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <net/if_arp.h>

#include <netpacket/packet.h>
#include <linux/if_ether.h>

#include <es/exception.h>
#include <es/synchronized.h>

namespace
{
    unsigned char scratchBuffer[1518];
}

// cf. $ man netdevice

Tap::Tap(const char* interfaceName) :
    monitor(0),
    sd(-1)
{
    struct ifreq ifr;

    memset(&statistics, 0, sizeof(statistics));

    ASSERT(interfaceName);
    strncpy(this->interfaceName, interfaceName, IFNAMSIZ - 1);
    this->interfaceName[IFNAMSIZ - 1] = '\0';

    int s = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (s == -1)
    {
        throw SystemException<ENODEV>();
    }

    // Get interface index number
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFINDEX, &ifr) == -1)
    {
        perror("SIOCGIFINDEX");
        throw SystemException<ENODEV>();
    }
    ifindex = ifr.ifr_ifindex;
    esReport("ifindex: %d\n", ifindex);

    // Get hardware address
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ - 1);
    if (ioctl(s, SIOCGIFHWADDR, &ifr) == -1)
    {
        perror("SIOCGIFINDEX");
        throw SystemException<ENODEV>();
    }
    struct sockaddr hwaddr;
    hwaddr = ifr.ifr_hwaddr;
    if (hwaddr.sa_family != ARPHRD_ETHER)
    {
        fprintf(stderr, "ARPHRD_ETHER");
        throw SystemException<ENODEV>();
    }
    memmove(mac, hwaddr.sa_data, 6);
    esReport("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    close(s);

    monitor = es::Monitor::createInstance();
    ASSERT(monitor);
}

Tap::~Tap()
{
    stop();
    if (monitor)
    {
        monitor->release();
    }
}

//
// es::Stream
//

int Tap::
start()
{
    Synchronized<es::Monitor*> method(monitor);

    if (0 <= sd)
    {
        return 0;
    }

    sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sd == -1)
    {
        perror("socket()");
        return -1;
    }

    // Bind to the interface
    struct sockaddr_ll sll;
    memset(&sll, 0xff, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = ifindex;
    if (bind(sd, (struct sockaddr*) &sll, sizeof sll) == -1)
    {
        perror("bind()");
        return -1;
    }

    // Flush packets before the binding
    ssize_t len;
    do {
        len = recv(sd, scratchBuffer, sizeof scratchBuffer, MSG_DONTWAIT);
    } while (0 < len);

    memset(&statistics, 0, sizeof(statistics));

    return 0;
}

int Tap::
stop()
{
    Synchronized<es::Monitor*> method(monitor);

    if (0 <= sd)
    {
        close(sd);
        sd = -1;
    }
    return 0;
}

int Tap::
read(void* dst, int count)
{
    ssize_t len = recv(sd, dst, count, 0);
    if (len == -1)
    {
        ++statistics.inErrors;
    }
    else
    {
        statistics.inOctets += len;
        if (0 < len)
        {
            if (static_cast<u8*>(dst)[0] & 0x01)
            {
                ++statistics.inNUcastPkts;
            }
            else
            {
                ++statistics.inUcastPkts;
            }
        }
    }
    return len;
}

int Tap::
write(const void* src, int count)
{
    struct sockaddr_ll sll;

    memset(&sll, 0, sizeof(sll));
    sll.sll_ifindex = ifindex;
    ssize_t len = sendto(sd, src, count, 0, (struct sockaddr*) &sll, sizeof(sll));
    if (len == -1)
    {
        ++statistics.outErrors;
    }
    else
    {
        statistics.outOctets += len;
        if (0 < len)
        {
            if (static_cast<const u8*>(src)[0] & 0x01)
            {
                ++statistics.outNUcastPkts;
            }
            else
            {
                ++statistics.outUcastPkts;
            }
        }
    }
    return len;
}

//
// es::NetworkInterface
//

bool Tap::
getPromiscuousMode()
{
    Synchronized<es::Monitor*> method(monitor);

    if (sd < 0)
    {
        return false;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ - 1);
    ioctl(sd, SIOCGIFFLAGS, &ifr);
    return (ifr.ifr_flags & IFF_PROMISC) ? true : false;
}

void Tap::
setPromiscuousMode(bool on)
{
    Synchronized<es::Monitor*> method(monitor);

    if (sd < 0)
    {
        return;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ - 1);
    ioctl(sd, SIOCGIFFLAGS, &ifr);
    if (on)
    {
        ifr.ifr_flags |= IFF_PROMISC;
    }
    else
    {
        ifr.ifr_flags &= ~IFF_PROMISC;
    }
    ioctl(sd, SIOCSIFFLAGS, &ifr);
}

int Tap::
addMulticastAddress(const unsigned char macaddr[6])
{
    Synchronized<es::Monitor*> method(monitor);

    if (sd < 0)
    {
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ - 1);
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memmove(ifr.ifr_hwaddr.sa_data, macaddr, 6);
    return ioctl(sd, SIOCADDMULTI, &ifr);
}

int Tap::
removeMulticastAddress(const unsigned char macaddr[6])
{
    Synchronized<es::Monitor*> method(monitor);

    if (sd < 0)
    {
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ - 1);
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
    memmove(ifr.ifr_hwaddr.sa_data, macaddr, 6);
    return ioctl(sd, SIOCDELMULTI, &ifr);
}

void Tap::
getMacAddress(unsigned char* mac)
{
    memmove(mac, this->mac, 6);
}

bool Tap::
getLinkState()
{
    Synchronized<es::Monitor*> method(monitor);

    if (sd < 0)
    {
        return false;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interfaceName, IFNAMSIZ - 1);
    ioctl(sd, SIOCGIFFLAGS, &ifr);
    return (ifr.ifr_flags & IFF_RUNNING) ? true : false;
}

//
// Object
//

Object* Tap::
queryInterface(const char* riid)
{
    Object* objectPtr;
    if (strcmp(riid, es::Stream::iid()) == 0)
    {
        objectPtr = static_cast<es::Stream*>(this);
    }
    else if (strcmp(riid, es::NetworkInterface::iid()) == 0)
    {
        objectPtr = static_cast<es::NetworkInterface*>(this);
    }
    else if (strcmp(riid, Object::iid()) == 0)
    {
        objectPtr = static_cast<es::NetworkInterface*>(this);
    }
    else
    {
        return NULL;
    }
    objectPtr->addRef();
    return objectPtr;
}

unsigned int Tap::
addRef()
{
    return ref.addRef();
}

unsigned int Tap::
release()
{
    unsigned long count = ref.release();
    if (count == 0)
    {
        delete this;
        return 0;
    }
    return count;
}

#endif  // __linux__