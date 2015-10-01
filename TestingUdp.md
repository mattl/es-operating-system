# Setting up vlan #

A virtual LAN, commonly known as a VLAN, is a group of hosts with a common set of requirements that communicate as if they were attached to the Broadcast domain, regardless of their physical location. A VLAN has the same attributes as a physical LAN, but it allows for end stations to be grouped together even if they are not located on the same network switch.

For testing udp, you will have to set up vlan (while testing apps under es/local/testsuite, if loopback interface is used, packets are not forwarded to linux loopback interface, but are simply echoed back. To get a better picture of how packets move in an actual network, It is better to set up vlan)

For setting up vlan on ubuntu, please follow the link http://wilsonyang.com/2008/04/27/vlan-configuration-on-ubuntu/
A successful set up should show various virtual lan interfaces when 'ifconfig' comand is executed. Default virtual interfaces are
vlan4
vlan5
vlan101

# Changes in source code #

Step1:-

go to /es/os/kernel/posix/posix\_init.cpp
modify the following part of code

try
> {
> > Tap**tap = new Tap("eth1");
> > device->bind("ethernet",
> > static\_cast<es::Stream**>(tap));
}

to point to one of virtual Lan ethernet say 'vlan4'
like

:-

try

> {
> > Tap**tap = new Tap("vlan4");
> > device->bind("ethernet",
> > static\_cast<es::Stream**>(tap));
}

Step 2:-

Now go to /es/os/net/testsuite/udpEchoServer.cpp
1)Modify the following part of code as (around line 96)
// Register host address (192.168.2.40)

> InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 40) };

the following (in case you have used vlan4 in posix\_init.cpp with default configuration)

// Register host address (192.168.2.40)
> InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 1) };

Now go to /es/os/net/testsuite/udpEchoClient.cpp
1)Modify the following part of code as (around line 86)
// Register host address (192.168.2.40)
> InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 40) };

the following (point to your real IP address)

// Register host address (192.168.2.40)
> InAddr addr = { htonl(10 << 24 | 8 << 16 | 41 << 8 | 193) };(In case your IP address of the real LAN eth0 is 10.8.41.193)

2)Modify the following part of code as (around line 40)
void echo(int dixID)
{
> InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 20) };

the following (point to your vlan4 IPaddress)

void echo(int dixID)
{
> InAddr addr = { htonl(192 << 24 | 168 << 16 | 2 << 8 | 1) };


After all these run udp server and client as linux applications
Make sure you rebuild the es-operating system for all these changes to take effect
cd /es/local/os/net/testsuite
sudo ./udpEchoServer
sudo ./udpEchoClient

The client should successfully get reply from the server. You can debug the actual call routing taking place in the stack using gdb.

If you have any issues feel free to mail the es-developer mailing list.
