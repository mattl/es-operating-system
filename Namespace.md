# The ES operating system's namespace #

Processes running in the ES operating system have their own individual hierarchical namespaces.
The Context interface pointer, which allows access to the namespace, can be obtained using the getRoot() method of the CurrentProcess interface.

For C++ programs linked with ES's runtime library libes++.a, the root namespace's Context interface pointer can be obtained by calling the System() function:

```
Context* root = System()->getRoot();
```

For ECMAScript scripts for esjs, the root namespace's Context object can be obtained through the global System object.

```
root = System.root;
```

Interface pointers are hierarchically registered in a namespace such as interface pointers to access files and directories in the filesystem, to access the system's device drivers and to access software component information.

The namespace contained in init, the first process to run when the ES operating system starts, is described below:

```
Name                                     Supported Interface        Description
class                                    Context                    Stores the interface pointers of software components that support the ClassFactory interface.    
    ::es::Monitor                        Monitor::Constructor       Used when a new monitor is created.
    ::es::Process                        Process::Constructor       Used when a new process is created.
    ::es::Cache                          Cache::Constructor         Used when a new memory object is created.
    ::es::PageSet                        PageSet::Constructor       Used when a new page set is created.
    ::es::Alarm                          Alarm::Constructor         Used when a new timer is created. 
    ::es::Partition                      Partition::Constructor     Used when a new partition is created.
    ::es::FatFileSystem                  FatFileSystem::Constructor Used when a new FAT file system is created.    
    ::es::IsoFileSystem                  IsoFileSystem::Constructor Used when a new ISO 9660 file system is created.    
device                                   Context                    Stores each device driver's interface pointer.    
    rtc                                  Rtc                        Stores the date/time interface pointer.    
    cga                                  Stream                     Stores the interface pointer for accessing the CGA text screen.    
    beep                                 Beep                       Stores the interface pointer for accessing the beep tone.    
    framebuffer                          Pageable, Stream           Stores the video framebuffer interface pointer.    
    cursor                               Cursor                     Stores the interface pointer for the mouse cursor.    
    ethernet                             Stream, NetworkInterface   Stores the interface pointer for accessing the ethernet network adapter.    
    keyboard                             Stream                     Stores the keyboard interface pointer.    
    loopback                             Stream, NetworkInterface   Stores the interface pointer for accessing the loopback interface.    
    mouse                                Stream                     Stores the mouse interface pointer.    
    floppy                               Stream                     Stores the interface pointer for accessing floppy disk drives.    
    ata                                  Context                    Stores the interface pointer for accessing ATA devices.    
        channel0                         Context                    Stores the interface pointer for accessing ATA channel 0 devices.    
            device0                      DiskManagement, Partition, 
                                         RemovableMedia, Stream     Stores the interface pointer for accessing ATA channel 0 primary devices.    
            device1                      DiskManagement, Partition, 
                                         RemovableMedia, Stream     Stores the interface pointer for accessing ATA channel 0 secondary devices.    
        channel1                         Context                    Stores the interface pointer for accessing ATA channel 1 devices.    
            device0                      DiskManagement, Partition, 
                                         RemovableMedia, Stream     Stores the interface pointer for accessing ATA channel 1 primary devices.    
            device1                      DiskManagement, Partition, 
                                         RemovableMedia, Stream     Stores the interface pointer for accessing ATA channel 1 secondary devices.    
    soundInput                           AudioFormat, Stream        Stores the interface pointer for accessing PCM audio input devices.    
    soundOutput                          AudioFormat, Stream        Stores the interface pointer for accessing PCM audio output devices.    
file                                     Context, ...               Stores the interface pointer for accessing files in the filesystem.    
network                                  Context                    Stores all network-related interface pointers.    
    config                               InternetConfig             Stores the interface pointer for configuring TCP/IP network connection.    
    resolver                             Resolver                   Stores the interface pointer for TCP/IP network host name resolution.    
    interface                            Context                    Stores interface pointers for TCP/IP network interfaces.    
        1                                Context                    Stores interface pointers for loopback interfaces.    
            interface                    Stream, NetworkInterface   Stores the interface pointer for accessing the loopback interface.    
        2                                Context                    Stores interface pointers for secondary network interfaces.    
            interface                    Stream, NetworkInterface   Stores interface pointers for accessing secondary network interfaces.    
            dhcp                         Service                    Stores the DHCP client service interface pointer.    
```