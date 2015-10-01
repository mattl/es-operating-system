# Building the support libraries #

ES operating system uses many open source libraries for 2D graphics, regular expression support, etc. You need to build these libraries for cross development in the following order before building ES operating system.

In the following steps, we will set up the ES cross libraries in '/opt/es'. If you need to set up them in the other place, please modify the `--prefix` and other options accordingly.

# PCRE #

The [Perl Compatible Regular Expressions (PCRE) library](http://www.pcre.org/) is used for the regular expression support.

```
$ cd src
$ wget ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre/pcre-7.8.tar.bz2
$ tar -jxvf pcre-7.8.tar.bz2
$ patch -p0 -d . < ../trunk/patches/pcre-7.8.patch
$ cd ../opt
$ mkdir pcre
$ cd pcre
$ ../../src/pcre-7.8/configure --prefix=/opt/es --disable-shared --enable-utf8 --enable-unicode-properties --host=i386-pc-es --target=i386-pc-es
$ make
$ make install
```

# FreeType 2 #

[!FreeType 2](http://www.freetype.org/) is used as a software font engine. Please download `freetype-2.3.5.tar.bz2` from http://sourceforge.net/project/showfiles.php?group_id=3157 into your `src` directory to install.

```
$ cd ../../src/

$ tar -jxvf freetype-2.3.5.tar.bz2
$ patch -p0 -d . < ../trunk/patches/freetype-2.3.5.patch
$ cd ../opt
$ mkdir freetype
$ cd freetype
$ ../../src/freetype-2.3.5/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es
$ make FTSYS_SRC=../../src/freetype-2.3.5/builds/unix/ftsystem.c
$ make install
```

# Expat #

[The Expat XML Parser](http://expat.sourceforge.net/) is used as an XML parser for fontconfig, which is installed next.
Please download `expat-2.0.1.tar.gz` from http://sourceforge.net/project/showfiles.php?group_id=10127&package_id=10780&release_id=513851 into your `src` directory to install.

```
$ cd ../../src/

$ tar -zxvf expat-2.0.1.tar.gz
$ patch -p0 -d . < ../trunk/patches/expat-2.0.1.patch
$ cd ../opt
$ mkdir expat
$ cd expat
$ ../../src/expat-2.0.1/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es
$ make
$ make installlib
```

# Fontconfig #

[Fontconfig](http://fontconfig.org/wiki/) is used for font customization and configuration for cairo, which is installed next.

```
$ cd ../../src/
$ wget http://fontconfig.org/release/fontconfig-2.4.2.tar.gz
$ tar -zxvf fontconfig-2.4.2.tar.gz
$ patch -p0 -d . < ../trunk/patches/fontconfig-2.4.2.patch
$ cd ../opt
$ mkdir fontconfig
$ cd fontconfig
$ ../../src/fontconfig-2.4.2/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es --with-arch=i386-pc-es --with-expat-includes=/opt/es/include --with-expat-lib=/opt/es/lib --with-freetype-config=/opt/es/bin/freetype-config --with-default-fonts=/file/fonts --with-cache-dir=/file --with-confdir=/file
$ make
$ make install
```

Note in the above configure command, the following three options specify the path to the directory in the target PC that runs the ES operating system:

| Option | Value | Description |
|:-------|:------|:------------|
| --with-default-fonts | /file/fonts | Use fonts from /file when config is busted |
| --with-cache-dir | /file | Use /file to store cache files |
| --with-confdir | /file | Use /file to store configuration files |

# Cairo #

[Cairo](http://cairographics.org/) is used for the 2D graphics.

```
$ cd ../../src/
$ wget http://cairographics.org/releases/cairo-1.4.10.tar.gz
$ tar -zxvf cairo-1.4.10.tar.gz
$ patch -p0 -d . < ../trunk/patches/cairo-1.4.10.patch
$ cd ../opt
$ mkdir cairo
$ cd cairo
$ ../../src/cairo-1.4.10/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es --disable-png --disable-xlib --disable-ps --disable-svg FREETYPE_CFLAGS='-I/opt/es/include/freetype2 -I/opt/es/include' FREETYPE_LIBS='-L/opt/es/lib -lfreetype' FONTCONFIG_CFLAGS=-I/opt/es/include FONTCONFIG_LIBS='-L/opt/es/lib -lfontconfig'
$ make
$ make install
cd ../..
```

## Your support libraries now set up! ##

Please move on to [build ES operating system](BuildingES.md).