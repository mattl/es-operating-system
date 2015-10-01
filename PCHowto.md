# Developing ES on a physical machine #

## Introduction ##

Since version 0.0.9, the ES operating system is able to run not just in QEMU but also on a physical machine. We will now explain how to run ES on a PC using a CompactFlash (CF) card.

## Preparing the target test PC ##

Below are some of the PC configurations we've successfully run ES on:

  * Pentium III/440BX + SB16 + RTL8029AS
  * Pentium 4/865G+ICH5 + ES1370 + RTL8029AS
  * Core 2 Duo/945G+ICH7 + ES1370 + RTL8029AS

Note: You may not be able to run ES on the above PC configurations depending on the BIOS versions, etc.

When developing ES, we use COM1 as a debug output and COM2 for connecting to GDB, so when choosing a motherboard, one with two COM ports is best. (There are some with an external COM1 port at the back and an internal COM2 port on the motherboard itself.)

For the video card, at the moment we are just using VESA BIOS for the video output so an integrated graphics chipset should be fine.

For SMP support, the ES kernel can be run in SMP mode on a Core 2 Duo processor if you enable the "Core Multi-Processing" option in the BIOS. Regarding Pentium 4 processors supporting hyper-threading, the ES kernel currently runs in single processor mode regardless of whether "Hyper-Threading" is enabled or disabled in the BIOS. This is because there is only information for the physical processors in the MPS 1.4 table that the ES kernel consults and the BIOS settings of the logical processors in HT are not reflected in the MPS table. (Logical processor information is stored in the ACPI specification but ES does not support ACPI yet.)

### Keyboard and mouse ###

Please use a PS/2 type keyboard and mouse. USB keyboards and mice are not supported yet. Also, at least a three-button mouse is needed to use Squeak. You can also use a scroll wheel with Squeak so a typical scroll wheel mouse is ideal. (ES itself supports up to a five-button scroll wheel mouse.)

### Sound ###

At the moment, only Creative SoundBlaster 16 and ENSONIQ AudioPCI ES1370 are supported.
Since these sound cards are emulated by QEMU, these are useful when moving directly from QEMU to the physical test environment.
We are planning to add support for the latest sound chips, too.

If you are using an ISA SoundBlaster 16 sound card, please check that the jumpers are set as follows: Base I/O Address = 0x220, Interrupt Channel = 5, 8-bit DMA channel = 1, 16-bit DMA channel = 5, MIDI Port Base I/O Address = 0x330.

### Networking ###

At the moment, only NE2000-compatible RTL8029AS network interface controller is supported.
Since this controller is emulated by QEMU, it is useful when moving directly from QEMU to the physical test environment.
We are planning to add support for the latest network interface controllers, too.

### CF-IDE adapter ###

As ES is being cross-developed, we have to copy the pre-built binary files on to a hard disk or floppy disk to boot ES on a physical PC.
Since it is inefficient and time-consuming to switch a hard disk between the development PC and the target PC or to copy the files one by one to a floppy disk, we use a CF-IDE adapter which enables a CF card to be used as an pseudo IDE hard disk drive. Hence by simply coping the pre-built binary files to the CF card and inserting it into the CF-IDE adapter, we can boot ES on the target test PC.

![http://es-operating-system.googlecode.com/svn/html/PCHowto/cf_ide.jpg](http://es-operating-system.googlecode.com/svn/html/PCHowto/cf_ide.jpg)

Connect the CF-IDE to the target PC's motherboard with an IDE cable. If your motherboard has more than one IDE connector, please use the primary IDE connector.

It's best to choose a CF card with the highest possible data transfer rate as this will improve Squeak's startup time by several seconds.

### Connecting and setting up the "cross" serial cables ###

Connect the COM1 and COM2 ports on the target test PC to the development PC. Set the COM1 and COM2 ports to 115200 bps, non parity, 8 bit, 1 stop bit.

```
$ stty -F /dev/ttyS0 115200 -parenb cs8 -cstopb
$ stty -F /dev/ttyS1 115200 -parenb cs8 -cstopb
```

COM1 is only used for displaying debugging output so we may enter the following in the terminal to see the output:

```
$ cat < /dev/ttyS0
```

## Steps to boot ES on the target test PC ##

Insert the CF card in the development PC. Please get a USB CF card reader/writer if your PC does not have a CF card slot. On Linux, the CF card should be assigned to /dev/sda or similar. With multi-card readers this may be sdb, sdc, etc. so please check using the dmesg command.

```
$ dmesg
SCSI device sdb: 1000944 512-byte hdwr sectors (512 MB)
sdb: Write Protect is off
sdb: Mode Sense: 00 23 00 00
sdb: assuming drive cache: write through
```

In the above example we can see that a 512MB CF card is in /dev/sdb. Change the permissions to allow read and write access to /dev/sdb.

```
$ sudo chmod 666 /dev/sdb
```

We then format the CF card. If you have finished building and install the ES cross-development tools then you can use the vformat command.

```
$ vformat /dev/sdb 
CHS: 0 0 0
Signature       0xaa55
BS_jmpBoot      eb 3c 90
BS_OEMName      NINTENDO
BPB_BytsPerSec  512
BPB_SecPerClus  16
BPB_RsvdSecCnt  1
BPB_NumFATs     2
BPB_RootEntCnt  512
BPB_TotSec16    0
BPB_Media       248
BPB_FATSz16     245
BPB_SecPerTrk   0
BPB_NumHeads    0
BPB_HiddSec     0
BPB_TotSec32    1000944
Volume is FAT16 (62526)
BS_DrvNum       0x0
BS_Reserved1    0
BS_BootSig      41
BS_VolID        0
BS_VolLab       NO NAME    
BS_FilSysType   FAT16   
freeCount: 62526 (500208KB)
nxtFree:   2
partition size: 512483328 bytes
```

On modern motherboards the above should work fine but older motherboards may need the CHS parameter. In that case, please set the CHS values as arguments (the example below is for a 512MB card with CHS=993/16/63).

```
$ vformat /dev/sdb 993 16 63
CHS: 993 16 63
Signature 0xaa55
BS_jmpBoot eb 3c 90
BS_OEMName NINTENDO
BPB_BytsPerSec 512
BPB_SecPerClus 16
BPB_RsvdSecCnt 1
BPB_NumFATs 2
BPB_RootEntCnt 512
BPB_TotSec16 0
BPB_Media 248
BPB_FATSz16 245
BPB_SecPerTrk 0
BPB_NumHeads 0
BPB_HiddSec 0
BPB_TotSec32 1000944
Volume is FAT16 (62526)
BS_DrvNum 0x0
BS_Reserved1 0
BS_BootSig 41
BS_VolID 0
BS_VolLab NO NAME 
BS_FilSysType FAT16 
freeCount: 62526 (500208KB)
nxtFree: 2
partition size: 512483328 bytes
```

The CF card has now been formatted as FAT with the ES boot sector.

**Note**: ES currently uses the so-called super-floppy format, and does not work with partition tables. Please be careful as CF cards that have been formatted in the super-floppy format can not be mounted in digital cameras, Linux, etc.

Next, copy es.ldr, es.img, and other required files to the CF card using the vcopy command from the ES cross-development tools.

```
$ vcopy /dev/sdb os/bootsect/es.ldr
$ vcopy /dev/sdb init/es.img
$ vcopy /dev/sdb cmd/squeak-3.7.1/squeak.elf
$ vcopy /dev/sdb ../trunk/cmd/squeak-3.7.1/SqueakV3.sources
$ vcopy /dev/sdb ../trunk/cmd/squeak-3.7.1/Squeak3.7-5989-full.changes
$ vcopy /dev/sdb ../trunk/cmd/squeak-3.7.1/Squeak3.7-5989-full.image
$ vcopy /dev/sdb esjs/esjs.elf
$ vcopy /dev/sdb ../trunk/esjs/scripts/cat.js
$ vcopy /dev/sdb ../trunk/esjs/scripts/cd.js
$ vcopy /dev/sdb ../trunk/esjs/scripts/clear.js
$ vcopy /dev/sdb ../trunk/esjs/scripts/date.js
$ vcopy /dev/sdb ../trunk/esjs/scripts/echo.js
$ vcopy /dev/sdb ../trunk/esjs/scripts/edit.js
$ vcopy /dev/sdb ../trunk/esjs/scripts/figure.js
$ vcopy /dev/sdb ../trunk/esjs/scripts/gradient.js
$ vcopy /dev/sdb ../trunk/esjs/scripts/ls.js
$ vcopy /dev/sdb ../trunk/esjs/scripts/rm.js
$ vcopy /dev/sdb ../trunk/esjs/scripts/shell.js
$ vcopy /dev/sdb cmd/hello.elf
$ vcopy /dev/sdb cmd/eventManager.elf
$ vcopy /dev/sdb cmd/console.elf
$ vcopy /dev/sdb ../trunk/init/fonts.conf
$ vcopy /dev/sdb ../trunk/init/fonts.dtd
$ vcopy /dev/sdb ../trunk/init/40-generic.conf conf.d/
$ vcopy /dev/sdb `find /usr/share/fonts -name sazanami-mincho.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name sazanami-gothic.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationMono-Bold.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationMono-BoldItalic.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationMono-Italic.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationMono-Regular.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationSans-Bold.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationSans-BoldItalic.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationSans-Italic.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationSans-Regular.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationSerif-Bold.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationSerif-BoldItalic.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationSerif-Italic.ttf` fonts/
$ vcopy /dev/sdb `find /usr/share/fonts -name LiberationSerif-Regular.ttf` fonts/
$ vcopy /dev/sdb ../trunk/init/1b68cf4fced22970e89022fed6868d03-x86.cache-2
```

**Note**: When using the vformat and vcopy commands, please ensure the access light on the CF card reader/writer has turned off. The same thing applies when you remove the CF card. If you remove the CF card while it is still being accessed, you may not be able to read or write to the card without rebooting your linux.

When the files have finished being copied, remove the CF card from the card reader/writer.

## Configuring the target test PC's BIOS ##

Insert the CF card in the CF-IDE adapter and switch on the target test PC's power. When booting for the first time, enter the BIOS settings screen. It depends on your BIOS type but in most cases, pressing `[`DEL`]` or `[`F1`]` at startup displays the BIOS settings screen.

Please check that the CF card is seen as an IDE hard disk in the BIOS. On old motherboards LBA mode may be disabled so try to enable it. Alternatively, set the CHS values when you format the CF card with vformat.

On Core 2 Duo processors, the ES kernel can run in SMP mode with "Core Multi-Processing" enabled in the BIOS.

In the COM1 and COM2 settings screens, enabling COM1 will make COM1 display debugging output. Enabling COM2 starts the GDB stub in the ES kernel so if you are not using GDB, please disable it.

**Note**: If you enable COM2 without running GDB, the ES kernel will try to communicate with GDB and eventually stop.

### Using GDB ###

In order to debug the ES kernel with GDB, enable COM2 in the BIOS settings screen and start GDB on a development PC in advance.

```
$ gdb es.elf
(gdb) target remote /dev/ttyS1
```

Do the following if you are using the GUI-based kdbg instead of GDB:

```
$ kdbg -r /dev/ttyS1 es.elf
```

By default, the ES kernel is automatically interrupted in esInit and control is passed to GDB.

When you have finished the BIOS setting, save it and reboot the PC. If everything goes well, ES should boot and the command prompt should appear.

![http://es-operating-system.googlecode.com/svn/html/PCHowto/espc.jpg](http://es-operating-system.googlecode.com/svn/html/PCHowto/espc.jpg)