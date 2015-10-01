# Setting up the ES development environment using the setup script #

To build the ES operating system, a variety of development tools including
binutils, GCC, Newlib, and other runtime libraries need to be built
as the first step.
The following steps allow you to set up ES operating system development kit
using the 'setup' shell script.
The 'setup' script will download the required files from the Internet, and
generate build tools, runtime libraries, and ES operating system disk image.
Note once the 'setup' script is started, it tries to install the prerequisites
like gcc, bison, etc., using 'yum' or 'apt-get' and you must have privilege
to execute the 'sudo' command.

Note: The following Linux distributions are currently used to build ES operating system:

  * Fedora 15 (Lovelock)
  * Ubuntu 11.04 (Natty Narwhal)

The ES operating system can be built from the ES source tarball, or from the
ES subversion trunk. The latter might fail or create an unstable binaries
because of the on-going changes made to the repository.

## From the source tarball ##

```
$ cd
$ mkdir es
$ cd es
$ wget http://es-operating-system.googlecode.com/files/es-x.y.z.tar.gz
$ tar -zxvf es-x.y.z.tar.gz
$ es-x.y.z/setup
// snip
usage: qemu -hda pc/init/fat32.img -serial stdio
```

## From the subversion repository ##

```
$ cd
$ mkdir es
$ cd es
$ wget http://es-operating-system.googlecode.com/svn/trunk/setup
$ chmod +x setup
$ ./setup
// snip
usage: qemu -hda pc/init/fat32.img -serial stdio
```

When your setup has created an ES disk image successfuly, it shows how to start ES with QEMU.

Both steps should have installed the ES SDK under ~/es/sdk.
So that you can use the ES SDK binaries later, please add ~/es/sdk/bin to your PATH:

```
$ export PATH=`pwd`/sdk/bin:$PATH
```

You should also add 'es' to the operating system list in Automake's config.sub file
as described in the DeveloperSetup page.

Once your setup is completed, the following directory structures will be created in your home directory:

```
~/es/
+---local/                  # ES build directory for local tools
+---opt/                    # cross-tool build directory
    +---binutils/
    +---cairo/
    +---expat/
    +---fontconfig/
    +---freetype/
    +---gcc/
    +---pcre/
+---pc/                     # ES build directory for target (pc)
+---sdk/                    # ES SDK
    +---bin/
    +---i386-pc-es/
    +---include/
    +---lib
+---src/                    # open source program source directory
    +---binutils-2.19/
    +---cairo-1.8.6/
    +---expat-2.0.1/
    +---fontconfig-2.4.2/
    +---freetype-2.3.5/
    +---gcc-4.3.1/
    +---icu/
    +---jpeg7/
    +---libpng-1.2.40/
    +---libxml2-2.7.3/
    +---newlib-1.16.0/
    +---pcre-8.01/
    +---pixman-0.15.10/
+---trunk/                  # ES source directory
    +---cmd                 # application programs
    +---esidl               # esidl Web IDL compiler
        +---cplusplus       # additional C++ classes for DOM access
        +---dom             # Web IDL source files from W3C/WHATWG
        +---include
        +---java            # additional Java interfaces for DOM access
        +---npapi           # DOM access library over NPAPI
           +---sample       # a sample plugin/application using DOM access library
        +---src
        +---testsuite
    +---esjs                # esjs ECMAScript interpreter
    +---include
        +---es              # ES class libraries and template libraries
            +---base        # the common ES interface definitions
            +---device      # the device driver interface definitions
            +---naming      # the namespace interface definitions
            +---util        # miscellaneous interface definitions
    +---init                # the ES kernel image is created here
    +---os
        +---bootsect        # ES boot sectors and kernel loader
        +---fs              # file subsystems
            +---fat         # FAT file subsystem
            +---iso9660     # ISO 9660 file subsystem
        +---kernel          # ES kernel modules
            +---include
            +---pc          # PC specific portion
            +---port        # portable portion
            +---posix       # POSIX specific portion for the ES kernel emulation
            +---testsuite
        +---libes++         # ES class libraries and template libraries
        +---net             # TCP/IP protocol stack
            +---include
            +---src
            +---testsuite
    +---patches             # patch files applied to the cross-development tools and libraries needed by ES
    +---tools               # command line tools for cross development
```

## And please participate in the ES operating system community ##

For the build problems, please send your build log to our discussion group
[es-operating-system@googlegroups.com](http://groups.google.com/group/es-operating-system) so we can investigate.


---


**Note (Ubuntu older than 8.04)**: The TrueType fonts listed in the DeveloperSetup page must also be installed somewhere
under /usr/share/fonts directory if they are not installed yet.
Unless you are using Fedora or Ubuntu 8.04 or later, you are most likely to do this step manually.

**Note (Ubuntu 7.10)**: If you are using Ubuntu 7.10, please specify the `-net none` option to run ES on QEMU as below:

```
$ qemu -hda fat32.img -serial stdio -soundhw sb16 -net none
```

QEMU 0.9.0 comes with Ubuntu 7.10 seems to have a problem with the RTL8029AS Ethernet controller emulation.