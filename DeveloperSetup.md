# Setting up the development environment #

## Prerequisites ##

  * autoconf 2.61 (or later)
  * automake 1.10.1 (or later)
  * patch
  * bison 2.1 (or later)
  * flex 2.5 (or later)
  * gcc 4.3 (or later)
  * libX11-devel, libXmu-devel, and libXi-devel
  * cairo-devel
  * freeglut-devel
  * gmp-devel 4.1 (or later. necessary to build gcc 4.3)
  * mpfr-devel 2.3.0 (or later. necessary to build gcc 4.3)

For development tools, the following libraries are required:

  * glibc 2.2 or newer, glibc 2.4 or newer recommended: several features don't work with glibc 2.3 or lower.
  * pcre 7.2 or newer configured with the `--enable-utf8` and `--enable-unicode-properties` options.

_Older versions might cause problems._

**Note**: If you are using the latest Linux distribution like [Fedora](http://fedoraproject.org/), you will just need to install the missing packages, if any, from the official repositories. You can test the ES operating system with [QEMU](http://fabrice.bellard.free.fr/qemu/), and you can also test the ES kernel components including the [TCP/IP stack](Conduit.md) or the esjs ECMAScript interpreter as ordinary Linux application programs.

**Note**: If you are using Ubuntu 8.04, the following packages will be enough to set up the development environment: subversion autoconf automake patch bison flex gcc libc6-dev g++ libpcre3-dev qemu libcairo2-dev libX11-dev ttf-liberation ttf-sazanami-mincho ttf-sazanami-gothic.

**Note**: If you are using the Mac OS X Leopard with XCode, you will be able to build the minimal ES cross-build tools. You can test the ES operating system with [Q](http://www.kju-app.org/kju/).

### Fonts ###

Currently, ES uses the Sazanami fonts (http://sourceforge.jp/projects/efont/) and the liberation fonts (https://www.redhat.com/promo/fonts/) for graphics. If these fonts are not installed in your host operating system, please install them, too (with yum, or apt-get, etc.)

### config.sub ###

Please add 'es' to the operating system list in Automake's `config.sub` as below:

```
# Decode manufacturer-specific aliases for certain operating systems.
     :
     :
     :
              | -skyos* | -haiku* | -rdos* | -toppers* | -es*)
```

Or you may reinstall automake 1.10 from the source code applying the [patch](http://code.google.com/p/trunk/source/browse/trunk/patches/automake-1.10.patch).

### /opt/es ###

In the following steps, we will set up the ES cross tools in '/opt/es'. Please make sure to create this directory with the appropriate permission. If you prefer to install ES to the other directory, please modify the --prefix and other options accordingly.

```
sudo mkdir -p /opt/es
```

Also please make sure the path to the cross tool chain is included in the PATH environment variable.

```
$ export PATH=/opt/es/bin:$PATH
```

## Getting the source code ##

First, Check out the source code from Google Code.

```
$ mkdir es
$ cd es
$ mkdir trunk
$ svn checkout http://es-operating-system.googlecode.com/svn/trunk/ trunk
```

## Building development tools ##

Second, you need to build development tools including the [esidl](esidl.md) IDL compiler.

```
$ mkdir local
$ cd local
$ CFLAGS=-g CXXFLAGS=-g ../trunk/configure
$ make
$ sudo make install
```

By default, tools like [esidl](esidl.md), vcopy, vformat, and vlist are copied to `/usr/local/bin`.

## Building the cross tool chain ##

Third, you need to build the cross tool chain building from GNU Binutils, GNU Compiler Collection (GCC), and Newlib.

### Binutils ###

```
$ cd ..
$ mkdir src
$ cd src
$ wget ftp://ftp.gnu.org/gnu/binutils/binutils-2.19.tar.bz2
$ tar -jxvf binutils-2.19.tar.bz2
$ patch -p0 -d . < ../trunk/patches/binutils-2.19.patch
$ cd ..
$ mkdir opt
$ cd opt
$ mkdir binutils
$ cd binutils
$ ../../src/binutils-2.19/configure --target=i386-pc-es --prefix=/opt/es
$ make
$ make install
$ i386-pc-es-as -V
GNU assembler version 2.19 (i386-pc-es) using BFD version (GNU Binutils) 2.19
```

### GCC and Newlib ###

```
$ cd ../../src/
$ wget ftp://ftp.gnu.org/gnu/gcc/gcc-4.3.1/gcc-4.3.1.tar.bz2
$ tar -jxvf gcc-4.3.1.tar.bz2
$ patch -p0 -d . < ../trunk/patches/gcc-4.3.1.patch
$ wget ftp://sources.redhat.com/pub/newlib/newlib-1.16.0.tar.gz
$ tar -zxvf newlib-1.16.0.tar.gz
$ patch -p0 -d . < ../trunk/patches/newlib-1.16.0.patch
$ cd gcc-4.3.1
$ ln -s ../newlib-1.16.0/newlib .
$ ln -s ../newlib-1.16.0/libgloss .
$ cd ../../opt
$ mkdir gcc
$ cd gcc
$ ../../src/gcc-4.3.1/configure --target=i386-pc-es --enable-threads --enable-languages=c,c++ --with-gnu-as --with-gnu-ld --with-newlib --with-gmp=/usr --with-mpfr=/usr --disable-shared --prefix=/opt/es
$ make
$ make install
$ i386-pc-es-gcc -v
Using built-in specs.
Target: i386-pc-es
Configured with: ../../src/gcc-4.3.1/configure es
Thread model: es
gcc version 4.3.1 (GCC) 
cd ../..
```

### Your development environment is now set up! ###

Please move on to the [support libraries setup](BuildingSupportLibraries.md).


---


The following is the list of files edited manually for ES. Other files in the patches are generated by aclocal, autoconf, and automake.

```
binutils-2.19/bfd/config.bfd
binutils-2.19/config.sub
binutils-2.19/configure.ac
binutils-2.19/gas/configure.tgt
binutils-2.19/ld/configure.tgt
```

```
gcc-4.3.1/config.sub
gcc-4.3.1/configure.ac
gcc-4.3.1/gcc/config/es.h
gcc-4.3.1/gcc/config/gthr-es.c
gcc-4.3.1/gcc/config/i386/es-elf.h
gcc-4.3.1/gcc/config/i386/t-es-i386
gcc-4.3.1/gcc/config/t-es
gcc-4.3.1/gcc/config.gcc
gcc-4.3.1/gcc/configure.ac
gcc-4.3.1/gcc/gthr-es.h
gcc-4.3.1/libgcc/config.host
gcc-4.3.1/libstdc++-v3/configure.ac
```

```
newlib-1.16.0/config.sub
newlib-1.16.0/configure.ac
newlib-1.16.0/libgloss/configure.in
newlib-1.16.0/libgloss/i386-es/Makefile.in
newlib-1.16.0/libgloss/i386-es/access.c
newlib-1.16.0/libgloss/i386-es/close.c
newlib-1.16.0/libgloss/i386-es/configure.in
newlib-1.16.0/libgloss/i386-es/crt0.S
newlib-1.16.0/libgloss/i386-es/es.c
newlib-1.16.0/libgloss/i386-es/es.ld
newlib-1.16.0/libgloss/i386-es/es-syscall.h
newlib-1.16.0/libgloss/i386-es/exit.c
newlib-1.16.0/libgloss/i386-es/fstat.c
newlib-1.16.0/libgloss/i386-es/getcwd.c
newlib-1.16.0/libgloss/i386-es/getdents.c
newlib-1.16.0/libgloss/i386-es/getpid.c
newlib-1.16.0/libgloss/i386-es/gettimeofday.c
newlib-1.16.0/libgloss/i386-es/kill.c
newlib-1.16.0/libgloss/i386-es/link.c
newlib-1.16.0/libgloss/i386-es/lseek.c
newlib-1.16.0/libgloss/i386-es/mkdir.c
newlib-1.16.0/libgloss/i386-es/mmap.c
newlib-1.16.0/libgloss/i386-es/nanosleep.c
newlib-1.16.0/libgloss/i386-es/open.c
newlib-1.16.0/libgloss/i386-es/pthread.c
newlib-1.16.0/libgloss/i386-es/read.c
newlib-1.16.0/libgloss/i386-es/rename.c
newlib-1.16.0/libgloss/i386-es/rmdir.c
newlib-1.16.0/libgloss/i386-es/sbrk.c
newlib-1.16.0/libgloss/i386-es/stat.c
newlib-1.16.0/libgloss/i386-es/times.c
newlib-1.16.0/libgloss/i386-es/unlink.c
newlib-1.16.0/libgloss/i386-es/write.c
newlib-1.16.0/newlib/configure.host
newlib-1.16.0/newlib/libc/include/machine/setjmp.h
newlib-1.16.0/newlib/libc/include/machine/types.h
newlib-1.16.0/newlib/libc/include/pthread.h
newlib-1.16.0/newlib/libc/include/sys/config.h
newlib-1.16.0/newlib/libc/include/sys/features.h
newlib-1.16.0/newlib/libc/include/sys/reent.h
newlib-1.16.0/newlib/libc/include/sys/time.h
newlib-1.16.0/newlib/libc/include/sys/types.h
newlib-1.16.0/newlib/libc/stdlib/__atexit.c
newlib-1.16.0/newlib/libc/stdlib/malloc.c
newlib-1.16.0/newlib/libc/stdlib/mallocr.c
newlib-1.16.0/newlib/libc/string/Makefile.am
newlib-1.16.0/newlib/libc/sys/configure.in
newlib-1.16.0/newlib/libc/sys/es/Makefile.am
newlib-1.16.0/newlib/libc/sys/es/configure.in
newlib-1.16.0/newlib/libc/sys/es/getreent.c
newlib-1.16.0/newlib/libc/sys/es/linkr.c
newlib-1.16.0/newlib/libc/sys/es/sys/dirent.h
newlib-1.16.0/newlib/libc/sys/es/sys/lock.h
newlib-1.16.0/newlib/libc/sys/es/sys/mman.h
newlib-1.16.0/newlib/libc/sys/es/syslink.c
newlib-1.16.0/newlib/libc/syscalls/syslink.c
newlib-1.16.0/newlib/libc/syscalls/sysunlink.c
```