SUFFIXES = .h .idl

es/reflect.h : $(srcdir)/../esidl/include/reflect.h
	mkdir -p es
	cp $? es

if ES

%.h : %.idl
	esidl -isystem $(srcdir) -include es/object.idl -namespace es -object Object $<

es/object.h : es/object.idl
	esidl -isystem $(srcdir) -object Object $<

else !ES

%.h : %.idl
	../esidl/esidl -isystem $(srcdir) -include es/object.idl -namespace es -object Object $<

es/object.h : es/object.idl
	../esidl/esidl -isystem $(srcdir) -object Object $<

endif !ES

nobase_include_HEADERS = \
	es.h \
	es/any.h \
	es/broker.h \
	es/capability.h \
	es/color.h \
	es/collection.h \
	es/context.h \
	es/dateTime.h \
	es/elf.h \
	es/endian.h \
	es/exception.h \
	es/formatter.h \
	es/handle.h \
	es/hashtable.h \
	es/interfaceData.h \
	es/interlocked.h \
	es/list.h \
	es/md5.h \
	es/nullable.h \
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
	es/orderedMap.h

generated_headers = \
	es/object.h \
	es/base/IAlarm.h \
	es/base/ICache.h \
	es/base/ICallback.h \
	es/base/IFile.h \
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
	es/device/IDisk.h \
	es/device/IDmac.h \
	es/device/IFatFileSystem.h \
	es/device/IFileSystem.h \
	es/device/IIso9660FileSystem.h \
	es/device/INetworkInterface.h \
	es/device/IPic.h \
	es/device/IRemovableMedia.h \
	es/device/IRtc.h \
	es/naming/IBinding.h \
	es/naming/IContext.h \
	es/net/IInternetAddress.h \
	es/net/IInternetConfig.h \
	es/net/IResolver.h \
	es/net/ISocket.h \
	es/util/IIterator.h \
	es/util/ISet.h \
	es/util/IOrderedMap.h \
	w3c/css.h \
	w3c/cssomview.h \
	w3c/dom.h \
	w3c/events.h \
	w3c/html5.h \
	w3c/ls.h \
	w3c/ranges.h \
	w3c/smil.h \
	w3c/stylesheets.h \
	w3c/traversal.h \
	w3c/views.h

# Specify nobase_ first when used in conjunction with either dist_ or nodist_.
nobase_nodist_include_HEADERS = \
	$(generated_headers)
	
nobase_include_HEADERS += \
	es/object.idl \
	es/base/IAlarm.idl \
	es/base/ICache.idl \
	es/base/ICallback.idl \
	es/base/IFile.idl \
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
	es/device/IDisk.idl \
	es/device/IDmac.idl \
	es/device/IFatFileSystem.idl \
	es/device/IFileSystem.idl \
	es/device/IIso9660FileSystem.idl \
	es/device/INetworkInterface.idl \
	es/device/IPic.idl \
	es/device/IRemovableMedia.idl \
	es/device/IRtc.idl \
	es/naming/IBinding.idl \
	es/naming/IContext.idl \
	es/net/IInternetAddress.idl \
	es/net/IInternetConfig.idl \
	es/net/IResolver.idl \
	es/net/ISocket.idl \
	es/util/IIterator.idl \
	es/util/ISet.idl \
	es/util/IOrderedMap.idl \
	w3c/css.idl \
	w3c/cssomview.idl \
	w3c/dom.idl \
	w3c/events.idl \
	w3c/html5.idl \
	w3c/ls.idl \
	w3c/ranges.idl \
	w3c/smil.idl \
	w3c/stylesheets.idl \
	w3c/traversal.idl \
	w3c/views.idl
	
nobase_nodist_include_HEADERS += interface.list es/includeAllInterfaces.h

interface.list : $(generated_headers)
	grep 'static const char\* const name' $^ | awk '{ print substr($$8, 2, length($$8) - 3) }' > $@
	
es/includeAllInterfaces.h : $(generated_headers)
	rm -f $@; \
	for i in $(generated_headers); \
	do \
		printf "#include <%s>\n" $$i >> $@; \
	done 

clean-local:
	rm -rf es w3c interface.list

