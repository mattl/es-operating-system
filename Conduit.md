# The design of the ES operating system's TCP/IP stack #

## Introduction ##

ES's TCP/IP stack is essentially based on the Conduits+ framework laid out in `[`Hüni 95`]`. One of the authors of `[`Hüni 95`]`, professor Ralph Johnson, is one of the "Design Patterns" GoF.

In the 1980s, protocol stack implementation was an extremely difficult problem. In his foreword to `[`Comer 87`]`, professor David D. Clark stated that, with regards to implementing the stack according to the OSI 7 layer reference model: "The layering suggested by the specification must be violated to insure efficient execution. (snip) The implementor must go it alone, trying to blend together network abstraction and operating system abstraction which don't seem to fit well." However, we know that tools for structurally designing and implementing an operating system-level protocol stack such as a stream input-output system `[`Ritchie 84`]` and a multiplexer `[`Pike 84`]` were proposed from around that time. Hüni et al.'s Conduits+ framework is the application of design patterns on these proposals. In the Conduits+ framework, the modules and multiplexers, layered on the stack to compose streams, are called "conduits". Depending on how they are used, conduits are split into four groups: protocol, adapter, mux and factory. The aim of the Conduits+ framework is to make the network stack structure easier to learn and use by applying the Strategy, State, Singleton, Command, Visitor and Prototype patterns to these conduits.

However, as far as the authors are aware, there are so far no examples of a complete TCP/IP stack being built with the Conduits+ framework. Nikander et al. used Java to implement UDP/IP with the Conduits+ framework but as a student project could not find the time to complete the implementation to include TCP/IP `[`Nikander 98`]`.

According to the great book by Wright and Stevens `[`Wright 95`]`, it seems that just reading the code of 4.4BSD-Lite's TCP/IP stack written in C is not that complicated anymore, but even so, updating the TCP/IP stack or experimenting with it surely remains extremely difficult. With ES operating system, we're aiming to implement and openly provide a TCP/IP stack following the RFC 1122 standard and based on the Conduits+ framework.

## TCP/IP stack conduit graph ##

Below is a conduit graph showing ES's IPv4 stack based on the Conduits+ framework. (We have used simple ellipses instead of the overlapping ellipse and rectangle used in `[`Hüni 95`]`.)

![http://es-operating-system.googlecode.com/svn/html/Conduit/inet4.png](http://es-operating-system.googlecode.com/svn/html/Conduit/inet4.png)

We usually see only very simplified conduit graphs in papers, but we can see how this can be scaled to cover the entire TCP/IP stack. In the conduit graph, the adapters receive packets and data from applications and the network, and the adapters store packets and data in messengers that carry commands. The visitors transmit messengers in the conduit graph issuing the commands as they move conduits like muxes and protocols. The commands issued by the visitors are processed by receivers corresponding to these conduits. In the receiver class, the State pattern is used and processes packets and data depending on the state of the TCP, ARP and IGMP protocols. Note in IPv6, ARP and IGMP have been integrated into ICMPv6 so the conduit graph is even simpler than for IPv4 (implementation of IPv6 is already underway in ES).

## Ethernet drivers ##

At present, the ES operating system supports LAN cards with Realtek's NE2000-compatible RTL8029AS Ethernet controller chip. RTL8029AS was a very popular Ethernet controller as it is emulated in QEMU.

There are many modern Ethernet controllers that can handle processes on the controller-side that are currently done in the CPU in ES, but operations that Ethernet drivers provide are basically as follows:

  * Sending packets.
  * Receiving packets.
  * Getting MAC addresses assigned to the controller.
  * Adding/removing multicast addresses that want to receive packets.
  * Getting the link status (whether the LAN cable is connected to a switch or hub).
  * Enabling/disabling promiscuous mode (unfiltered reception mode).

In the ES operating system, these tasks are defined as the [NetworkInterface](http://code.google.com/p/es-operating-system/source/browse/trunk/include/es/device/INetworkInterface.idl) interface. To use Ethernet controller chips other than RTL8029AS, you will need to implement the NetworkInterface interface for them.

See also: [dp8390d.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/os/kernel/pc/dp8390d.cpp)

## DIX conduit ##

![http://es-operating-system.googlecode.com/svn/html/Conduit/dix.png](http://es-operating-system.googlecode.com/svn/html/Conduit/dix.png)

Depending on the received packet type in the Ethernet packet (frame) headers, dixTypeMux moves received packets to dixInProtocol (IPv4) or dixArpProtocol (ARP). dixInProtocol and dixArpProtocol add the appropriate Ethernet packet headers to the packets being transmitted to the local network.

## ARP conduits ##

![http://es-operating-system.googlecode.com/svn/html/Conduit/arp.png](http://es-operating-system.googlecode.com/svn/html/Conduit/arp.png)

At the Ethernet level, packets are transmitted to the MAC address specified in their Ethernet packet headers. The role of the ARP protocol is to resolve the mappings between IP addresses and Ethernet MAC addresses. The very basic mechanism of ARP is to broadcast an inquiry to see whether there is a node that has been assigned the IP address trying to communicate. If there is the recipient of the inquiry who has been assigned the requested IP address then it reports its own MAC address to the original sender. Since is is not very efficient to request a MAC address with an ARP request broadcast every time sending an IP packet, ARP caches mappings between MAC addresses and IP addresses that it has inquired about.

Another feature of ARP is to obtain link-local IP addresses. Link-local IP addresses are addresses that can be assigned dynamically to the appliances on a LAN without using DHCP; so information appliances on a home LAN can communicate each other using TCP/IP without any manual network configuration. In obtaining link-local IP addresses, an appliance broadcasts an inquiry to see whether the link-local IP address it wants to use is already in use or not, and if no reply is received (i.e. not used), the IP address is assigned to that appliance.

### States of onlink's Inet4Address object ###

ArpProtocol conduit looks for the source protocol address of the received ARP packet and, if the Inet4Address object for that address is already created, changes its state to Reachable. ArpMux moves received ARP packets to ArpAdapter inspecting their target protocol addresses. ArpAdapter receivers are the Inet4Address objects, and they process received ARP packets based on their state.

![http://es-operating-system.googlecode.com/svn/html/Conduit/arp_std.png](http://es-operating-system.googlecode.com/svn/html/Conduit/arp_std.png)

| State | Description | RFC |
|:------|:------------|:----|
| Init  | The MAC address corresponding to the IP address is not yet known. | 826 |
| Incomplete | The MAC address corresponding to the IP address is being requested. | 826 |
| Reachable | The MAC address corresponding to the IP address is known. | 826 |
| Probe | The MAC address corresponding to the IP address is being re-checked. | 826 |
| Tentative | Checking whether a local address can be used. | 3927 |
| Preferred | A local address can be used. | 3927 |
| Deprecated | A local address that is due to be discontinued. | 3927 |

**Note**: In the Inet4Address object there are other states relating to multicast addresses and destination addresses which we will cover later.

See also: [arp.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/os/net/src/arp.cpp), RFC 826 and RFC 3927.

## IP conduits ##

![http://es-operating-system.googlecode.com/svn/html/Conduit/ip.png](http://es-operating-system.googlecode.com/svn/html/Conduit/ip.png)

InProtocol conduit looks for the protocol number of received IP packets and sends the packets to InMux. If the received IP packets are fragmented packets, it sets IPPROTO\_FRAGMENT as a pseudo-protocol number to the packets so that they can be processed by the higher fragment reassembly conduits. (IPPROTO\_FRAGMENT is originally a protocol number for IPv6.) In the case that the destination address of an IP packet being sent is not the loopback address or the on-link IP address, InProtocol conduit configures the messenger to send the packet to a router. It also inspects the packet length and if it is larger than the path MTU, fragments it before sending it on to the lower InScopeMux.

InMux inspects the protocol number of the received IP packet and sends it to the higher conduits like UDP, TCP, etc.

InScopeMux inspects the scope ID of the packet to be sent and moves the packet to the loopback interface or Ethernet interface.

See also: [inet4.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/os/net/src/inet4.cpp), RFC 791.

## ICMP conduits ##

![http://es-operating-system.googlecode.com/svn/html/Conduit/icmp.png](http://es-operating-system.googlecode.com/svn/html/Conduit/icmp.png)

The ICMP conduits handle ICMP error messages and ICMP echo messages.

IcmpMux looks at the TYPE field of received ICMP messages and sends the messages to a conduit corresponding to the TYPE.

EchoRequestMux processes received ICMP echo requests (8). EchoRequestAdapter conduits forked from EchoRequestMux are installed when the Inet4Address object's state is changed to Preferred for each local address. The EchoRequestAdapter returns ICMP echo replies.

EchoReplyMux processes received ICMP echo replies (0). EchoReplyAdapter conduits forked from EchoReplyMux are installed when the Inet4Address object's isReachable method is called. When the EchoReplyAdapter receives an ICMP echo reply, the isReachable method completes and returns true.

UnreachProtocol, TimeExceededProtocol, etc. handle ICMP error messages. When ICMP error messages are received, it is re-sent as an error message to InProtocol conduit, if necessary, based on the copy of the problematic IP header stored in the ICMP error message. This time, the error message is sent to the conduit that originally sent the packet (TCP, UDP, etc.) and not the ICMP conduit.

See also: [icmp4.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/os/net/src/icmp4.cpp), RFC 792.

## Fragment reassembly conduits ##

![http://es-operating-system.googlecode.com/svn/html/Conduit/reass.png](http://es-operating-system.googlecode.com/svn/html/Conduit/reass.png)

Fragmented packets are all sent to the fragment reassembly conduit from InMux. A ReassAdapter is installed when an IP fragment with new IP packet numbers arrives. When the fragment reassembly by ReassAdapter is complete, the reassembled packet is sent from InProtocol conduit to the higher conduits. A ReassAdapter is uninstalled either when fragment reassembly is completed or when reassembly fails due to a timeout.

**Note**: In the IPv4 RFC, fragment reassembly is documented at the IP layer but it was easier to implement this at a higher layer similar to IPv6 where it is regarded as a single, independent protocol process. So we violated the suggested layering here as pointed out by Prof. Clark. But it is also interesting to see IPv6 protocol has refined those details.

See also: [inet4reass.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/os/net/src/inet4reass.cpp), RFC 815.

## UDP conduits ##

![http://es-operating-system.googlecode.com/svn/html/Conduit/udp.png](http://es-operating-system.googlecode.com/svn/html/Conduit/udp.png)

The UDP conduits process UDP datagrams. UDP-related processes peculiar to IPv4 are handled in the lower UdpProtocol. Meanwhile, UDP-related processes common to IPv4 and IPv6 are handled in the upper DatagramProtocol.

See also: [datagram.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/os/net/src/datagram.cpp),
[udp.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/os/net/src/udp.cpp), RFC 768.

## TCP conduits ##

![http://es-operating-system.googlecode.com/svn/html/Conduit/tcp.png](http://es-operating-system.googlecode.com/svn/html/Conduit/tcp.png)

The TCP conduits process TCP segments. TCP-related processes peculiar to IPv4 are handled in the lower TcpProtocol. Meanwhile, TCP-related processes common to IPv4 and IPv6 are handled in the upper StreamProtocol. StreamProtocol deals with TCP using the State pattern.

See also: [stream.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/os/net/src/stream.cpp),
[tcp.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/os/net/src/tcp.cpp), RFC 793.

## IGMP conduits ##

![http://es-operating-system.googlecode.com/svn/html/Conduit/igmp.png](http://es-operating-system.googlecode.com/svn/html/Conduit/igmp.png)

### Inet4Address object states for multicast addresses ###

When you create an address object specifying a multicast address using the IResolver interface, an Inet4Address object with NonMember state is created. To join a multicast group, the IMulticastSocket interface's joinGroup method is called with the multicast address's Inet4Address object as an argument. In doing this, a new igmpAdapter is installed in the IGMP conduits and the Inet4Address object sends an IGMP REPORT message and then changes its state to DelayingMember. Usually an Inet4Address object with DelayingMember state timeouts once to send another IGMP REPORT message and changes its state to IdleMember. To leave a multicast group, the IMulticastSocket interface's leaveGroup method is called. In doing this, the Inet4Address object sends an IGMP LEAVE message, changes its state to IdleMember and then igmpAdapter is uninstalled from the IGMP conduits.

There are cases when a multicast router will send an IGMP QUERY message to check whether there is a node that has joined a multicast group in the LAN. When the QUERY is received, the Inet4Address object changes its state to DelayingMember. Basically, QUERY needs to be replied to with REPORT but the router only needs to know whether there is at least a single node that has joined a group in the LAN so it is a waste of traffic if all nodes reply to QUERY. Because of this, in RFC 2236 a node belonging to a group that receives QUERY does not send REPORT immediately, but rather goes on standby for a certain length of time and if another node sends REPORT in that time, it then changes its state to IdleMember without sending REPORT. If it can't confirm that REPORT has been sent while on standby, it will send REPORT itself and then change its state to IdleMember. Since standby time is decided randomly per node, in most cases only a single node will send REPORT in response to QUERY.

![http://es-operating-system.googlecode.com/svn/html/Conduit/igmp_std.png](http://es-operating-system.googlecode.com/svn/html/Conduit/igmp_std.png)

| State | Description |
|:------|:------------|
| NonMember | Not belonging to the group. |
| DelayingMember | Checking whether it belongs to the group or not. |
| IdleMember | Belonging to the group. |

**Note**: There are several versions of IGMP and if a node receives an older version of a QUERY from the router, it must dynamically lower the the version of IGMP it uses, however in ES only IGMPv2 is currently implemented.

See also: [igmp.cpp](http://code.google.com/p/es-operating-system/source/browse/trunk/os/net/src/igmp.cpp), RFC 2236.

## Inet4Address object ##

An Inet4Address object is one that keeps information related to an IP address. A new Inet4Address object can be created by calling the IResolver interface's getHostByAddress and getHostByName methods.


---


## References ##

`[`Comer 87`]` D. Comer, "Operating System Design - Volume II: Internetworking with Xinu," Prentice-Hall, 1986.

`[`Hüni 95`]` H. Hüni, R. E. Johnson, R. Engel, "A Framework for Network Protocol Software," In ''Proc. OOPSLA '95, SIGPLAN Notices'', pp. 358-369, Oct. 1995.

`[`Nikander 98`]` P. Nikander, et al., "Java Conduits Beans (Jacob) project," http://www.tml.tkk.fi/Research/TeSSA/Old_pages/Jacob/jacob3.html.

`[`Pike 84`]` R. Pike, "The Blit: A Multiplexed Graphics Terminal," AT&T Bell Laboratories Technical Journal, Vol 63, No. 8, Part 2, pp. 1607-1632, Oct. 1984.

`[`Ritchie 84`]` D. M. Ritchie, "A Stream Input Output System," AT&T Bell Laboratories Technical Journal, Vol 63, No. 8, Part 2, pp. 1897-1910, Oct. 1984.

`[`Wright 95`]` G. R. Wright, W. R. Stevens, "TCP/IP Illustrated, Volume 2:The Implementation", Addison-Wesley, 1995.