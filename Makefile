.PHONY : clean

all : pc/init/fat32.img

requisites :
	if grep Ubuntu\ 8.04 /etc/issue; then \
        echo "Ubuntu 8.04 detected"; \
        sudo apt-get -qq update; \
        sudo apt-get install subversion autoconf automake patch bison flex gcc libc6-dev g++ libpcre3-dev qemu libcairo2-dev libX11-dev ttf-liberation ttf-sazanami-mincho ttf-sazanami-gothic freeglut3-dev; \
	else \
		if grep Fedora /etc/issue; then \
            echo "Fedora detected"; \
            sudo yum -y install subversion autoconf automake patch bison flex gcc-c++ glibc pcre-devel qemu freeglut-devel cairo-devel libX11-devel libXmu-devel libXi-devel sazanami-fonts-mincho sazanami-fonts-gothic; \
        else \
            echo "Your OS is probably not a supported development environment"; \
        fi; \
    fi; \

trunk : requisites
	if [ -z $$GOOGLE_USERNAME ]; then \
		svn checkout http://es-operating-system.googlecode.com/svn/trunk/ trunk; \
	else \
		svn checkout https://es-operating-system.googlecode.com/svn/trunk/ trunk --username $$GOOGLE_USERNAME; \
	fi; \
	for i in trunk trunk/esidl trunk/tools trunk/os trunk/init trunk/cmd trunk/esjs; \
	do (cd $$i; aclocal; autoconf; automake -a --foreign;); \
	done

patches = \
	trunk/patches/binutils-2.18.patch \
	trunk/patches/cairo-1.4.10.patch \
	trunk/patches/expat-2.0.1.patch \
	trunk/patches/fontconfig-2.4.2.patch \
	trunk/patches/freetype-2.3.5.patch \
	trunk/patches/gcc-4.2.1.patch \
	trunk/patches/newlib-1.15.0.patch \
	trunk/patches/pcre-7.2.patch

$(patches) : trunk 

src/binutils-2.18 : trunk/patches/binutils-2.18.patch
	if [ ! -f src/binutils-2.18.tar.bz2 ]; then \
		wget -P src ftp://ftp.gnu.org/gnu/binutils/binutils-2.18.tar.bz2; \
	fi; \
	rm -rf $@; \
	tar -C src -jxf src/binutils-2.18.tar.bz2; \
	patch -p0 -d src < $<; \
	touch $@

src/cairo-1.4.10 : trunk/patches/cairo-1.4.10.patch
	if [ ! -f src/cairo-1.4.10.tar.gz ]; then \
		wget -P src http://cairographics.org/releases/cairo-1.4.10.tar.gz; \
	fi; \
	rm -rf $@; \
	tar -C src -zxf src/cairo-1.4.10.tar.gz; \
	patch -p0 -d src < $<; \
	touch $@

src/expat-2.0.1 : trunk/patches/expat-2.0.1.patch
	if [ ! -f src/expat-2.0.1.tar.gz ]; then \
		wget -P src http://superb-west.dl.sourceforge.net/sourceforge/expat/expat-2.0.1.tar.gz; \
	fi; \
	rm -rf $@; \
	tar -C src -zxf src/expat-2.0.1.tar.gz; \
	patch -p0 -d src < $<; \
	touch $@

src/fontconfig-2.4.2 : trunk/patches/fontconfig-2.4.2.patch
	if [ ! -f src/fontconfig-2.4.2.tar.gz ]; then \
		wget -P src http://fontconfig.org/release/fontconfig-2.4.2.tar.gz; \
	fi; \
	rm -rf $@; \
	tar -C src -zxf src/fontconfig-2.4.2.tar.gz; \
	patch -p0 -d src < $<; \
	touch $@

src/freetype-2.3.5 : trunk/patches/freetype-2.3.5.patch
	if [ ! -f src/freetype-2.3.5.tar.bz2 ]; then \
		wget -P src http://superb-east.dl.sourceforge.net/sourceforge/freetype/freetype-2.3.5.tar.bz2; \
	fi; \
	rm -rf $@; \
	tar -C src -jxf src/freetype-2.3.5.tar.bz2; \
	patch -p0 -d src < $<; \
	touch $@

src/gcc-4.2.1 : trunk/patches/gcc-4.2.1.patch
	if [ ! -f src/gcc-4.2.1.tar.bz2 ]; then \
		wget -P src ftp://ftp.gnu.org/gnu/gcc/gcc-4.2.1/gcc-4.2.1.tar.bz2; \
	fi; \
	rm -rf $@; \
	tar -C src -jxf src/gcc-4.2.1.tar.bz2; \
	patch -p0 -d src < $<; \
	touch $@

src/newlib-1.15.0 : trunk/patches/newlib-1.15.0.patch
	if [ ! -f src/newlib-1.15.0.tar.gz ]; then \
		wget -P src ftp://sources.redhat.com/pub/newlib/newlib-1.15.0.tar.gz; \
	fi; \
	rm -rf $@; \
	tar -C src -zxf src/newlib-1.15.0.tar.gz; \
	patch -p0 -d src < $<; \
	touch $@

src/pcre-7.2 : trunk/patches/pcre-7.2.patch
	if [ ! -f src/pcre-7.2.tar.bz2 ]; then \
		wget -P src ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre/pcre-7.2.tar.bz2; \
	fi; \
	rm -rf $@; \
	tar -C src -jxf src/pcre-7.2.tar.bz2; \
	patch -p0 -d src < $<; \
	touch $@

local : trunk 
	if [ ! -d local ]; then \
		mkdir local; \
		cd local; \
		CFLAGS=-g CXXFLAGS=-g ../trunk/configure; \
		cd ..; \
	fi; \
	cd local; \
	make; \
	sudo make install

opt/binutils : src/binutils-2.18 local
	if [ ! -d $@ ]; then \
		mkdir -p $@; \
	fi; \
	cd $@; \
	../../src/binutils-2.18/configure --target=i386-pc-es --prefix=/opt/es ; \
	make; \
	make install

opt/gcc : src/gcc-4.2.1 src/newlib-1.15.0 opt/binutils
	if [ ! -e src/gcc-4.2.1/newlib ]; then \
		ln -s ../newlib-1.15.0/newlib src/gcc-4.2.1; \
	fi; \
	if [ ! -e src/gcc-4.2.1/libgloss ]; then \
		ln -s ../newlib-1.15.0/libgloss src/gcc-4.2.1; \
	fi; \
	if [ ! -d $@ ]; then \
		mkdir -p $@; \
	fi; \
	cd $@; \
	PATH=/opt/es/bin:$$PATH; \
	../../src/gcc-4.2.1/configure --target=i386-pc-es --enable-threads --enable-languages=c,c++ --with-gnu-as --with-gnu-ld --with-newlib --prefix=/opt/es ; \
	make; \
	make install

opt/pcre : src/pcre-7.2 opt/gcc
	if [ ! -d $@ ]; then \
		mkdir -p $@; \
	fi; \
	cd $@; \
	PATH=/opt/es/bin:$$PATH; \
	../../src/pcre-7.2/configure --disable-shared --enable-utf8 --enable-unicode-properties --host=i386-pc-es --target=i386-pc-es --prefix=/opt/es ; \
	make; \
	make install

opt/freetype : src/freetype-2.3.5 opt/gcc
	if [ ! -d $@ ]; then \
		mkdir -p $@; \
	fi; \
	cd $@; \
	PATH=/opt/es/bin:$$PATH; \
	../../src/freetype-2.3.5/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es; \
	make FTSYS_SRC=../../src/freetype-2.3.5/builds/unix/ftsystem.c
	-cd $@; make install

opt/expat : src/expat-2.0.1 opt/gcc
	if [ ! -d $@ ]; then \
		mkdir -p $@; \
	fi; \
	cd $@; \
	PATH=/opt/es/bin:$$PATH; \
	../../src/expat-2.0.1/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es; \
	make; \
	make installlib

opt/fontconfig : src/fontconfig-2.4.2 opt/expat opt/freetype
	if [ ! -d $@ ]; then \
		mkdir -p $@; \
	fi; \
	cd $@; \
	PATH=/opt/es/bin:$$PATH; \
	../../src/fontconfig-2.4.2/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es --with-arch=i386-pc-es --with-expat-includes=/opt/es/include --with-expat-lib=/opt/es/lib --with-freetype-config=/opt/es/bin/freetype-config --with-default-fonts=/file/fonts --with-cache-dir=/file --with-confdir=/file; \
	make
	-cd $@; make install

opt/cairo : src/cairo-1.4.10 opt/fontconfig
	if [ ! -d $@ ]; then \
		mkdir -p $@; \
	fi; \
	cd $@; \
	PATH=/opt/es/bin:$$PATH; \
	../../src/cairo-1.4.10/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es --disable-png --disable-xlib --disable-ps --disable-svg FREETYPE_CFLAGS='-I/opt/es/include/freetype2 -I/opt/es/include' FREETYPE_LIBS='-L/opt/es/lib -lfreetype' FONTCONFIG_CFLAGS=-I/opt/es/include FONTCONFIG_LIBS='-L/opt/es/lib -lfontconfig'; \
	make; \
	make install

pc : opt/pcre opt/cairo
	if [ ! -d $@ ]; then \
		mkdir -p $@; \
	fi; \
	cd $@; \
	PATH=/opt/es/bin:$$PATH; \
	CFLAGS=-g CXXFLAGS=-g ../trunk/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es; \
	make; \
	make install

pc/cmd/squeak-3.7.1 : pc
	if [ ! -d $@ ]; then \
		mkdir -p $@; \
	fi; \
	if [ ! -f trunk/cmd/squeak-3.7.1/SqueakV3.sources ]; then \
		(cd trunk/cmd/squeak-3.7.1; \
		wget http://ftp.squeak.org/3.7/SqueakV3.sources.gz; \
		gunzip SqueakV3.sources.gz;); \
	fi; \
	if [ ! -f trunk/cmd/squeak-3.7.1/Squeak3.7-5989-full.image ]; then \
		(cd trunk/cmd/squeak-3.7.1; \
		wget http://ftp.squeak.org/3.7/Squeak3.7-5989-full.zip; \
		unzip Squeak3.7-5989-full.zip;); \
	fi; \
	cd $@; \
	PATH=/opt/es/bin:$$PATH; \
	CFLAGS=-g CXXFLAGS=-g ../../../trunk/cmd/squeak-3.7.1/configure --prefix=/opt/es --host=i386-pc-es --target=i386-pc-es; \
	make; \
	make install

pc/init/fat32.img : pc pc/cmd/squeak-3.7.1
	cd pc/init; \
	./es

clean :
	-rm -rf local opt pc

clobber : clean
	-rm -rf src

