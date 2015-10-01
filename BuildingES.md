# Building the ES operating system for x86 PC #

Once your [development environment](DeveloperSetup.md) and [support libraries](BuildingSupportLibraries.md) are set up correctly, you can build the ES operating system for x86 PC.

In the following steps, we will use the ES cross libraries in '/opt/es'. If you have set up them in the other place, please modify you --prefix and other options accordingly.

## ES operating system for PC ##

```
$ mkdir pc
$ cd pc
$ CFLAGS=-g CXXFLAGS=-g ../trunk/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es
$ make
```

## Squeak port for ES ##

You can execute [Squeak](http://www.squeak.org/) on ES. To build Squeak port for ES, please execute the following steps:

```
$ cd cmd
$ mkdir squeak-3.7.1
$ cd squeak-3.7.1
$ CFLAGS=-g CXXFLAGS=-g ../../../trunk/cmd/squeak-3.7.1/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es
$ make
$ cd ../../../trunk/cmd/squeak-3.7.1/
$ wget http://ftp.squeak.org/3.7/Squeak3.7-5989-full.zip
$ unzip Squeak3.7-5989-full.zip
$ wget http://ftp.squeak.org/3.7/SqueakV3.sources.gz
$ gunzip SqueakV3.sources.gz
$ cd ../..
```

## ES boot disk image for PC ##

To generate a disk image that can be used with [QEMU](http://fabrice.bellard.free.fr/qemu/), execute the script named `es` in the `init` directory.

```
$ cd init
$ ./es
```

The resulting file, named `fat32.img`, is a bootable ES operating system disk image. Boot the ES operating system on QEMU:

```
$ qemu -hda fat32.img -serial stdio -soundhw sb16
```


---


**Note**: Depends on the operating systems and/or the qemu versions, you may need to specify dummy CD ROM image by specifying `-cdrom dummy.iso` option to qemu.

**Note**: If you are using Ubuntu 7.10, please add the `-net none` option to run ES on QEMU as below:

```
$ qemu -hda fat32.img -serial stdio -soundhw sb16 -net none
```

QEMU 0.9.0 comes with Ubuntu 7.10 seems to have a problem with the RTL8029AS Ethernet controller emulation.
