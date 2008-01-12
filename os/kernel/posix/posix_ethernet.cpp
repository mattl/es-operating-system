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

#include "posix/tap.h"
#include <es/synchronized.h>
#include <es/clsid.h>
#include <es/types.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <net/if.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/if_tun.h>

Tap::Tap(const char* ifName, const char* bridge, const char* script) : ref(0), fd(-1)
{
    if (!ifName || !bridge)
    {
        return;
    }
    strcpy(this->ifName, ifName); // TAP interface.
    strcpy(this->bridge, bridge); // bridge interface.

    if (script)
    {
        strcpy(this->script, script); // startup script.
    }
    else
    {
        this->script[0] = 0;
    }

    getBridgeMacAddress();

    esCreateInstance(CLSID_Monitor,
                     IID_IMonitor,
                     reinterpret_cast<void**>(&monitor));
}

Tap::~Tap()
{
    stop();
    if (monitor)
    {
        monitor->release();
    }
}

int Tap::
setup()
{
    int pid;
    int ret;
    int status;
    char*  args[3];
    char** parg;

    if (script[0])
    {
        pid = fork();
        if (0 <= pid)
        {
            if (pid == 0)
            {
                // child process
                parg = args;
                *parg++ = (char*) script;
                *parg++ = (char*) ifName;
                *parg++ = NULL;
                execv(script, args);
                _exit(1);
            }

            // parent process
            while (waitpid(pid, &status, 0) != pid)
            {
                ;
            }
            if (!WIFEXITED(status) ||
                WEXITSTATUS(status) != 0)
            {
                esReport("%s: could not launch network script\n", script);
                return -1;
            }
        }
    }

    return 0;
}

int Tap::
getBridgeMacAddress()
{
    int fd;
    struct ifreq ifr;

    // get the mac address of the bridge interface.
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        return -1;
    }

    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, bridge, IFNAMSIZ-1);
    ioctl(fd, SIOCGIFHWADDR, &ifr);

    memmove(mac, ifr.ifr_hwaddr.sa_data, sizeof(mac));

    return 0;
}

//
// IStream
//

int Tap::
start()
{
    int ret;
    struct ifreq ifr;

    fd = open("/dev/net/tun", O_RDWR);
#ifdef VERBOSE
    esReport("%s fd %d\n", __func__, fd);
#endif // VERBOSE
    if (fd < 0)
    {
        esReport("warning: could not open /dev/net/tun: no virtual network emulation\n");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    strcpy(ifr.ifr_name, ifName);

    ret = ioctl(fd, TUNSETIFF, (void *) &ifr);
    if (ret != 0)
    {
        esReport("warning: could not configure /dev/net/tun: no virtual network emulation\n");
        close(fd);
        return -1;
    }

    setup();
    return 0;
}

int Tap::
stop()
{
    if (0 <= fd)
    {
        int n = fd;
        fd = -1;
        close(n);
    }
    return 0;
}

int Tap::
read(void* dst, int count)
{
    if (fd < 0)
    {
        return -1;
    }
    return ::read(fd, dst, count);
}

int Tap::
write(const void* src, int count)
{
    if (fd < 0)
    {
        return -1;
    }
    return ::write(fd, src, (size_t) count);
}

//
// INetworkInterface
//

void Tap::
getMacAddress(unsigned char* mac)
{
    // Use 10-00-00 private MAC-48 for now XXX
    static const u8 oui[3] = { 0x10, 0x00, 0x00 };
    memmove(mac, this->mac, 6);
    memmove(mac, oui, 3);
}


//
// IInterface
//

bool Tap::
queryInterface(const Guid& riid, void** objectPtr)
{
    if (riid == IID_IStream)
    {
        *objectPtr = static_cast<IStream*>(this);
    }
    else if (riid == IID_INetworkInterface)
    {
        *objectPtr = static_cast<INetworkInterface*>(this);
    }
    else if (riid == IID_IInterface)
    {
        *objectPtr = static_cast<INetworkInterface*>(this);
    }
    else
    {
        *objectPtr = NULL;
        return false;
    }
    static_cast<IInterface*>(*objectPtr)->addRef();
    return true;
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
