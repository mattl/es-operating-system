SUFFIXES = .h .idl

if ES

%.h %.ent : %.idl
	esidl -C -I$(srcdir) -include es/base/IInterface.idl -object Interface $<

es/base/IInterface.h es/base/IInterface.ent : es/base/IInterface.idl
	esidl -C -I$(srcdir) $<

else !ES

%.h %.ent : %.idl
	PATH=../esidl:$(PATH) ../esidl/esidl -C -I$(srcdir) -include es/base/IInterface.idl -object Interface $<

es/base/IInterface.h es/base/IInterface.ent : es/base/IInterface.idl
	PATH=../esidl:$(PATH) ../esidl/esidl -C -I$(srcdir) $<

endif !ES

nobase_include_HEADERS = \
	es.h \
	es/broker.h \
	es/capability.h \
	es/color.h \
	es/collection.h \
	es/context.h \
	es/dateTime.h \
	es/elf.h \
	es/ent.h \
	es/endian.h \
	es/exception.h \
	es/formatter.h \
	es/handle.h \
	es/hashtable.h \
	es/interlocked.h \
	es/list.h \
	es/md5.h \
	es/objectTable.h \
	es/ref.h \
	es/reflect.h \
	es/ring.h \
	es/rpc.h \
	es/synchronized.h \
	es/timer.h \
	es/timeSpan.h \
	es/tree.h \
	es/types.h \
	es/usage.h \
	es/utf.h \
	es/uuid.h \
	es/net/arp.h \
	es/net/dhcp.h \
	es/net/dns.h \
	es/net/dix.h \
	es/net/icmp.h \
	es/net/igmp.h \
	es/net/inet4.h \
	es/net/inet6.h \
	es/net/tcp.h \
	es/net/udp.h \
	es/variant.h \
	es/orderedMap.h

# Specify nobase_ first when used in conjunction with either dist_ or nodist_.
nobase_nodist_include_HEADERS = \
	es/base/IAlarm.h \
	es/base/ICache.h \
	es/base/ICallback.h \
	es/base/IFile.h \
	es/base/IInterface.h \
	es/base/IInterfaceStore.h \
	es/base/IMonitor.h \
	es/base/IPageable.h \
	es/base/IPageSet.h \
	es/base/IProcess.h \
	es/base/IRuntime.h \
	es/base/ISelectable.h \
	es/base/IService.h \
	es/base/IStream.h \
	es/base/IThread.h \
	es/device/IAudioFormat.h \
	es/device/IBeep.h \
	es/device/ICursor.h \
	es/device/IDevice.h \
	es/device/IDiskManagement.h \
	es/device/IDmac.h \
	es/device/IFatFileSystem.h \
	es/device/IFileSystem.h \
	es/device/IIso9660FileSystem.h \
	es/device/INetworkInterface.h \
	es/device/IPic.h \
	es/device/IRemovableMedia.h \
	es/device/IRtc.h \
	es/device/IPartition.h \
	es/naming/IBinding.h \
	es/naming/IContext.h \
	es/net/IInternetAddress.h \
	es/net/IInternetConfig.h \
	es/net/IResolver.h \
	es/net/ISocket.h \
	es/util/IIterator.h \
	es/util/ISet.h \
	es/util/ICanvasRenderingContext2D.h \
	es/util/IOrderedMap.h

nobase_nodist_include_HEADERS += \
	es/base/IAlarm.ent \
	es/base/ICache.ent \
	es/base/ICallback.ent \
	es/base/IFile.ent \
	es/base/IInterface.ent \
	es/base/IInterfaceStore.ent \
	es/base/IMonitor.ent \
	es/base/IPageable.ent \
	es/base/IPageSet.ent \
	es/base/IProcess.ent \
	es/base/IRuntime.ent \
	es/base/ISelectable.ent \
	es/base/IService.ent \
	es/base/IStream.ent \
	es/base/IThread.ent \
	es/device/IAudioFormat.ent \
	es/device/IBeep.ent \
	es/device/ICursor.ent \
	es/device/IDevice.ent \
	es/device/IDiskManagement.ent \
	es/device/IDmac.ent \
	es/device/IFatFileSystem.ent \
	es/device/IFileSystem.ent \
	es/device/IIso9660FileSystem.ent \
	es/device/INetworkInterface.ent \
	es/device/IPic.ent \
	es/device/IRemovableMedia.ent \
	es/device/IRtc.ent \
	es/device/IPartition.ent \
	es/naming/IBinding.ent \
	es/naming/IContext.ent \
	es/net/IInternetAddress.ent \
	es/net/IInternetConfig.ent \
	es/net/IResolver.ent \
	es/net/ISocket.ent \
	es/util/IIterator.ent \
	es/util/ISet.ent \
	es/util/ICanvasRenderingContext2D.ent \
	es/util/IOrderedMap.ent

nobase_include_HEADERS += \
	es/base/IAlarm.idl \
	es/base/ICache.idl \
	es/base/ICallback.idl \
	es/base/IFile.idl \
	es/base/IInterface.idl \
	es/base/IInterfaceStore.idl \
	es/base/IMonitor.idl \
	es/base/IPageable.idl \
	es/base/IPageSet.idl \
	es/base/IProcess.idl \
	es/base/IRuntime.idl \
	es/base/ISelectable.idl \
	es/base/IService.idl \
	es/base/IStream.idl \
	es/base/IThread.idl \
	es/device/IAudioFormat.idl \
	es/device/IBeep.idl \
	es/device/ICursor.idl \
	es/device/IDevice.idl \
	es/device/IDiskManagement.idl \
	es/device/IDmac.idl \
	es/device/IFatFileSystem.idl \
	es/device/IFileSystem.idl \
	es/device/IIso9660FileSystem.idl \
	es/device/INetworkInterface.idl \
	es/device/IPic.idl \
	es/device/IRemovableMedia.idl \
	es/device/IRtc.idl \
	es/device/IPartition.idl \
	es/naming/IBinding.idl \
	es/naming/IContext.idl \
	es/net/IInternetAddress.idl \
	es/net/IInternetConfig.idl \
	es/net/IResolver.idl \
	es/net/ISocket.idl \
	es/util/IIterator.idl \
	es/util/ISet.idl \
	es/util/ICanvasRenderingContext2D.idl \
	es/util/IOrderedMap.idl

clean-local:
	-rm -rf es
