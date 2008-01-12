SUFFIXES = .h .idl

if ES

%.h %.ird : %.idl
	esidl -I $(srcdir) $<

else !ES

%.h %.ird : %.idl
	../esidl/esidl -I $(srcdir) $<

endif !ES

nobase_include_HEADERS = \
	es.h \
	es/broker.h \
	es/classFactory.h \
	es/clsid.h \
	es/dateTime.h \
	es/elf.h \
	es/endian.h \
	es/exception.h \
	es/formatter.h \
	es/handle.h \
	es/interlocked.h \
	es/irf.h \
	es/list.h \
	es/md5.h \
	es/ref.h \
	es/reflect.h \
	es/ring.h \
	es/synchronized.h \
	es/timeSpan.h \
	es/types.h \
	es/usage.h \
	es/utf.h \
	es/uuid.h

# Specify nobase_ first when used in conjunction with either dist_ or nodist_.
nobase_nodist_include_HEADERS = \
	es/base/IAlarm.h \
	es/base/ICache.h \
	es/base/ICallback.h \
	es/base/IClassFactory.h \
	es/base/IClassStore.h \
	es/base/IFile.h \
	es/base/IInterface.h \
	es/base/IMonitor.h \
	es/base/IPageable.h \
	es/base/IPageSet.h \
	es/base/IProcess.h \
	es/base/IRuntime.h \
	es/base/IStream.h \
	es/base/IThread.h \
	es/device/IAudioFormat.h \
	es/device/IBeep.h \
	es/device/ICursor.h \
	es/device/IDevice.h \
	es/device/IDiskManagement.h \
	es/device/IDmac.h \
	es/device/IFileSystem.h \
	es/device/IPic.h \
	es/device/IRemovableMedia.h \
	es/device/IRtc.h \
	es/device/IPartition.h \
	es/naming/IBinding.h \
	es/naming/IContext.h \
	es/util/IIterator.h \
	es/util/ISet.h

nobase_nodist_include_HEADERS += \
	es/base/IAlarm.ird \
	es/base/ICache.ird \
	es/base/ICallback.ird \
	es/base/IClassFactory.ird \
	es/base/IClassStore.ird \
	es/base/IFile.ird \
	es/base/IInterface.ird \
	es/base/IMonitor.ird \
	es/base/IPageable.ird \
	es/base/IPageSet.ird \
	es/base/IProcess.ird \
	es/base/IRuntime.ird \
	es/base/IStream.ird \
	es/base/IThread.ird \
	es/device/IAudioFormat.ird \
	es/device/IBeep.ird \
	es/device/ICursor.ird \
	es/device/IDevice.ird \
	es/device/IDiskManagement.ird \
	es/device/IDmac.ird \
	es/device/IFileSystem.ird \
	es/device/IPic.ird \
	es/device/IRemovableMedia.ird \
	es/device/IRtc.ird \
	es/device/IPartition.ird \
	es/naming/IBinding.ird \
	es/naming/IContext.ird \
	es/util/IIterator.ird \
	es/util/ISet.ird

EXTRA_DIST = \
	es/base/IAlarm.idl \
	es/base/ICache.idl \
	es/base/ICallback.idl \
	es/base/IClassFactory.idl \
	es/base/IClassStore.idl \
	es/base/IFile.idl \
	es/base/IInterface.idl \
	es/base/IMonitor.idl \
	es/base/IPageable.idl \
	es/base/IPageSet.idl \
	es/base/IProcess.idl \
	es/base/IRuntime.idl \
	es/base/IStream.idl \
	es/base/IThread.idl \
	es/device/IAudioFormat.idl \
	es/device/IBeep.idl \
	es/device/ICursor.idl \
	es/device/IDevice.idl \
	es/device/IDiskManagement.idl \
	es/device/IDmac.idl \
	es/device/IFileSystem.idl \
	es/device/IPic.idl \
	es/device/IRemovableMedia.idl \
	es/device/IRtc.idl \
	es/device/IPartition.idl \
	es/naming/IBinding.idl \
	es/naming/IContext.idl \
	es/util/IIterator.idl \
	es/util/ISet.idl

clean-local:
	-rm -rf es
