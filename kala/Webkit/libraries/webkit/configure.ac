AC_INIT([WebKit],[1.1.7],[http://bugs.webkit.org/])
AC_CONFIG_AUX_DIR(.)
AC_CONFIG_HEADERS([autotoolsconfig.h])
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
WEBKIT_MAJOR_VERSION=1
WEBKIT_MINOR_VERSION=1
WEBKIT_MICRO_VERSION=7
AC_SUBST(WEBKIT_MAJOR_VERSION)
AC_SUBST(WEBKIT_MINOR_VERSION)
AC_SUBST(WEBKIT_MICRO_VERSION)

AC_CONFIG_SRCDIR([trunk/WebCore/config.h])

AM_INIT_AUTOMAKE([foreign subdir-objects tar-ustar])

case "$host" in
     *-*-linux*)
       os_linux=yes
       ;;
     *-*-freebsd*)
       os_freebsd=yes
       ;;
     *-*-darwin*)
       os_darwin=yes
       ;;
     *-*-es*)
       os_es=yes
       ;;
esac

case "$host_os" in
     gnu* | linux* | k*bsd*-gnu)
       os_gnu=yes
       ;;
     *)
       os_gnu=no
       ;;
esac

case ${target} in
  *-*-es)
    os_type=es
    ;;
  *)
    os_type=posix
    ;;
esac


# initialize webkit options
# WEBKIT_INIT
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_SYS_LARGEFILE

# C/C++ Language Features
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE

# C/C++ Headers
AC_HEADER_STDC
AC_HEADER_STDBOOL

if test "$os_linux" = "yes"; then
UNICODE_CFLAGS=`icu-config --cppflags`
UNICODE_LIBS=`icu-config --ldflags`
fi
AC_SUBST([UNICODE_CFLAGS])
AC_SUBST([UNICODE_LIBS])

PKG_CHECK_MODULES([LIBXML], [libxml-2.0 >= 2.6])
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)

PKG_CHECK_MODULES([CAIRO], [cairo >= 1.2])
AC_SUBST(CAIRO_CFLAGS)
AC_SUBST(CAIRO_LIBS)

JPEG_LIBS="-ljpeg"
AC_SUBST([JPEG_LIBS])

# check whether to build with debugging enabled
AC_MSG_CHECKING([whether to do a debug build])
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug],
                             [turn on debugging [default=no]]),
              [],[enable_debug="no"])
AC_MSG_RESULT([$enable_debug])

# check whether to enable optimized builds
AC_MSG_CHECKING([whether to enable optimized builds])
AC_ARG_ENABLE(optimizations,
              AC_HELP_STRING([--enable-optimizations],
                             [turn on optimize builds (GCC only)
                             [default=yes]]),
              [enable_optimizations=$enableval],
              [if test "$enable_debug" = "yes"; then enable_optimizations="no"; else enable_optimizations="yes"; fi])
AC_MSG_RESULT([$enable_optimizations])

# check whether to enable dashboard support
AC_MSG_CHECKING([whether to enable Dashboard support])
AC_ARG_ENABLE(dashboard_support,
              AC_HELP_STRING([--enable-dashboard-support],
                             [enable Dashboard support [default=no]]),
              [],[enable_dashboard_support="no"])
AC_MSG_RESULT([$enable_dashboard_support])

# check whether to enable 3D transforms support
AC_MSG_CHECKING([whether to enable support for 3D Transforms])
AC_ARG_ENABLE(3D_transforms,
              AC_HELP_STRING([--enable-3D-transforms],
                             [enable support for 3D transforms [default=no]]),
              [],[enable_3D_transforms="no"])
AC_MSG_RESULT([$enable_3D_transforms])

# check whether to enable channel messaging support
AC_MSG_CHECKING([whether to enable HTML5 Channel Messaging support])
AC_ARG_ENABLE(channel_messaging,
              AC_HELP_STRING([--enable-channel-messaging],
                             [enable HTML5 channel messaging support [default=no]]),
              [],[enable_channel_messaging="no"])
AC_MSG_RESULT([$enable_channel_messaging])

# check whether to enable JavaScript debugger/profiler support
AC_MSG_CHECKING([whether to enable JavaScript debugger/profiler support])
AC_ARG_ENABLE(javascript_debugger,
              AC_HELP_STRING([--enable-javascript-debugger],
                             [enable JavaScript debugger/profiler support [default=no]]),
              [],[enable_javascript_debugger="no"])
AC_MSG_RESULT([$enable_javascript_debugger])

# check whether to enable HTML5 Offline Web Applications support
AC_MSG_CHECKING([whether to enable HTML5 offline web applications support])
AC_ARG_ENABLE(offline_web_applications,
              AC_HELP_STRING([--enable-offline-web-applications],
                             [enable HTML5 offline web applications support [default=no]]),
              [],[enable_offline_web_applications="no"])
AC_MSG_RESULT([$enable_offline_web_applications])

# check whether to enable HTML5 client-side session and persitent storage support
AC_MSG_CHECKING([whether to enable HTML5 client-side session and persistent storage support])
AC_ARG_ENABLE(dom_storage,
              AC_HELP_STRING([--enable-dom-storage],
                             [enable HTML5 client-side session and persistent storage support [default=no]]),
              [],[enable_dom_storage="no"])
AC_MSG_RESULT([$enable_dom_storage])

# check whether to build with database support
AC_MSG_CHECKING([whether to enable HTML5 client-side database storage support])
AC_ARG_ENABLE(database,
              AC_HELP_STRING([--enable-database],
                             [enable HTML5 client-side database storage support [default=no]]),
              [],[enable_database="no"])
AC_MSG_RESULT([$enable_database])

# check whether to build with icon database support
AC_MSG_CHECKING([whether to enable icon database support])
AC_ARG_ENABLE(icon_database,
              AC_HELP_STRING([--enable-icon-database],
                             [enable icon database [default=no]]),
              [],[enable_icon_database="no"])
AC_MSG_RESULT([$enable_icon_database])

# check whether to enable HTML5 audio/video support
AC_MSG_CHECKING([whether to enable HTML5 video support])
AC_ARG_ENABLE(video,
              AC_HELP_STRING([--enable-video],
                             [enable HTML5 video support [default=no]]),
              [],[enable_video="no"])
AC_MSG_RESULT([$enable_video])

# check whether to enable XPath support
AC_MSG_CHECKING([whether to enable XPath support])
AC_ARG_ENABLE(xpath,
              AC_HELP_STRING([--enable-xpath],
                             [enable support for XPath [default=no]]),
              [],[enable_xpath="no"])
AC_MSG_RESULT([$enable_xpath])

# check whether to enable XSLT support
AC_MSG_CHECKING([whether to enable XSLT support])
AC_ARG_ENABLE(xslt,
              AC_HELP_STRING([--enable-xslt],
                             [enable support for XSLT [default=no]]),
              [],[enable_xslt="no"])
AC_MSG_RESULT([$enable_xslt])

# check whether to enable geolocation support
AC_MSG_CHECKING([whether to enable geolocation support])
AC_ARG_ENABLE(geolocation,
              AC_HELP_STRING([--enable-geolocation],
                             [enable support for geolocation [default=no]]),
              [],[enable_geolocation="no"])
AC_MSG_RESULT([$enable_geolocation])

# check whether to enable gnomekeyring support
AC_MSG_CHECKING([whether to enable gnomekeyring support])
AC_ARG_ENABLE(gnomekeyring,
              AC_HELP_STRING([--enable-gnomekeyring],
                             [enable support for gnomekeyring [default=no]]),
              [],[enable_gnomekeyring="no"])
AC_MSG_RESULT([$enable_gnomekeyring])

# check whether to enable SVG support
AC_MSG_CHECKING([whether to enable SVG support])
AC_ARG_ENABLE(svg,
              AC_HELP_STRING([--enable-svg],
                             [enable support for SVG [default=no]]),
              [],[enable_svg="no"])
AC_MSG_RESULT([$enable_svg])

# check whether to enable WML support
AC_MSG_CHECKING([whether to enable WML support])
AC_ARG_ENABLE(wml,
              AC_HELP_STRING([--enable-wml],
                             [enable support for WML [default=no]]),
              [],[enable_wml="no"])
AC_MSG_RESULT([$enable_wml])

# check whether to enable Web Workers support
AC_MSG_CHECKING([whether to enable Web Workers support])
AC_ARG_ENABLE(web_workers,
              AC_HELP_STRING([--enable-web-workers],
                             [enable support for Web Workers [default=no]]),
              [],[enable_web_workers="no"])
AC_MSG_RESULT([$enable_web_workers])

# turn off svg features if --disable-svg is requested
if test "$enable_svg" = "no"; then
   enable_svg_animation=no
   enable_svg_filters=no
   enable_svg_fonts=no
   enable_svg_foreign_object=no
   enable_svg_as_image=no
   enable_svg_use_element=no
fi

# check whether to enable support for SVG animation
AC_MSG_CHECKING([whether to enable support for SVG animation])
AC_ARG_ENABLE(svg_animation,
              AC_HELP_STRING([--enable-svg-animation],
                             [enable support for SVG animation (experimental) [default=yes]]),
              [],[enable_svg_animation="yes"])
AC_MSG_RESULT([$enable_svg_animation])

# check whether to enable support for SVG filters
AC_MSG_CHECKING([whether to enable support for SVG filters])
AC_ARG_ENABLE(svg_filters,
              AC_HELP_STRING([--enable-svg-filters],
                             [enable support for SVG filters (experimental) [default=no]]),
              [],[enable_svg_filters="no"])
AC_MSG_RESULT([$enable_svg_filters])

# check whether to enable support for SVG fonts
AC_MSG_CHECKING([whether to enable support for SVG fonts])
AC_ARG_ENABLE(svg_fonts,
              AC_HELP_STRING([--enable-svg-fonts],
                             [enable support for SVG fonts (experimental) [default=yes]]),
              [],[enable_svg_fonts="yes"])
AC_MSG_RESULT([$enable_svg_fonts])

# check whether to enable foreign objects support for SVG
AC_MSG_CHECKING([whether to enable support for SVG foreign objects])
AC_ARG_ENABLE(svg_foreign_object,
              AC_HELP_STRING([--enable-svg-foreign-object],
                             [enable support for SVG foreign objects (experimental) [default=yes]]),
              [],[enable_svg_foreign_object="yes"])
AC_MSG_RESULT([$enable_svg_foreign_object])

# check whether to enable SVG As Image support
AC_MSG_CHECKING([whether to enable SVG as Image support])
AC_ARG_ENABLE(svg_as_image,
              AC_HELP_STRING([--enable-svg-as-image],
                             [enable SVG as Image support (experimental) [default=yes]]),
              [],[enable_svg_as_image="yes"])
AC_MSG_RESULT([$enable_svg_as_image])

# check whether to enable SVG USE element support
AC_MSG_CHECKING([whether to enable support for SVG use element])
AC_ARG_ENABLE(svg_use_element,
              AC_HELP_STRING([--enable-svg-use-element],
                             [enable SVG use element support (experimental) [default=yes]]),
              [],[enable_svg_use_element="yes"])
AC_MSG_RESULT([$enable_svg_use_element])

# check for SVG features, enabling SVG if necessary
if test "$enable_svg_animation" = "yes" || \
   test "$enable_svg_filters" = "yes" || \
   test "$enable_svg_fonts" = "yes" || \
   test "$enable_svg_foreign_object" = "yes" || \
   test "$enable_svg_as_image" = "yes" || \
   test "$enable_svg_use_element" = "yes"; then
   svg_flags=yes
   if test "$enable_svg" = "no"; then
       AC_MSG_WARN([SVG feature(s) requested but SVG is disabled.. Enabling SVG support])
       enable_svg=yes
   fi
fi

# check whether to enable code coverage
AC_MSG_CHECKING([whether to enable code coverage support])
AC_ARG_ENABLE(coverage,
              AC_HELP_STRING([--enable-coverage],
                             [enable code coverage support [default=no]]),
              [],[enable_coverage="no"])
AC_MSG_RESULT([$enable_coverage])

# check whether to enable FastMalloc
AC_MSG_CHECKING([whether to enable optimized memory allocator])
AC_ARG_ENABLE(fast_malloc,
              AC_HELP_STRING([--enable-fast-malloc],
                             [enable optimized memory allocator default=yes, default=no for debug builds]),
              [],[if test "$enable_debug" = "yes"; then enable_fast_malloc="no"; else enable_fast_malloc="no"; fi])
AC_MSG_RESULT([$enable_fast_malloc])

AC_MSG_CHECKING([whether to enable JIT compilation])
AC_ARG_ENABLE([jit],
              AC_HELP_STRING([--enable-jit],
                             [Enable JIT compilation default=no]),
              [],[enable_jit="no"])
if test "$enable_jit" = "yes"; then
    case "$host_cpu" in
        i*86|x86_64)
            AC_DEFINE([ENABLE_JIT], [1], [Define to enable JIT])
            AC_DEFINE([ENABLE_YARR], [1], [Define to enable YARR])
            AC_DEFINE([ENABLE_YARR_JIT], [1], [Define to enable YARR JIT])
            AC_DEFINE([ENABLE_JIT_OPTIMIZE_CALL], [1], [Define to enable optimizing calls])
            AC_DEFINE([ENABLE_JIT_OPTIMIZE_PROPERTY_ACCESS], [1], [Define to enable optimized property access])
            AC_DEFINE([ENABLE_JIT_OPTIMIZE_ARITHMETIC], [1], [Define to enable optimized arithmetic])
            case "$host_cpu" in
                i*86)
                    AC_DEFINE([WTF_USE_JIT_STUB_ARGUMENT_VA_LIST], [1], [Use stub va_list])
                ;;
                x86_64)
                    AC_DEFINE([WTF_USE_JIT_STUB_ARGUMENT_REGISTER], [1], [Use stub register])
                    AC_DEFINE([WTF_USE_ALTERNATE_JSIMMEDIATE], [1], [Use alternate JSImmediate])
                ;;
            esac
        ;;
        *)
            enable_jit="no (CPU '$host_cpu' not supported)"
        ;;
    esac
fi
AC_MSG_RESULT([$enable_jit])

# determine the font backend
AC_MSG_CHECKING([the font backend to use])
AC_ARG_WITH(font_backend,
            AC_HELP_STRING([--with-font-backend=@<:@freetype/pango@:>@],
                           [Select font backend [default=freetype]]),
            [],[with_font_backend="freetype"])

case "$with_font_backend" in
     freetype|pango) ;;
     *) AC_MSG_ERROR([Invalid font backend: must be freetype or pango.]) ;;
esac
AC_MSG_RESULT([$with_font_backend])

# Add '-g' flag to gcc if it's debug build
if test "$enable_debug" = "yes"; then
   CXXFLAGS="$CXXFLAGS -g"
   CFLAGS="$CFLAGS -g"
else
   AC_DEFINE([NDEBUG], [1], [Define to disable debugging features])
fi

# Add the appropriate 'O' level for optimized builds
if test "$enable_optimizations" = "yes"; then
   CXXFLAGS="$CXXFLAGS -O2"
   CFLAGS="$CFLAGS -O2"
else
   CXXFLAGS="$CXXFLAGS -O0"
   CFLAGS="$CFLAGS -O0"
fi

# check for HTML features
if test "$enable_video" = "yes"; then
    html_flags=yes
fi

# OS conditionals
AM_CONDITIONAL([OS_WIN32],[test "$os_win32" = "yes"])
AM_CONDITIONAL([OS_UNIX],[test "$os_win32" = "no"])
AM_CONDITIONAL([OS_LINUX],[test "$os_linux" = "yes"])
AM_CONDITIONAL([OS_GNU],[test "$os_gnu" = "yes"])
AM_CONDITIONAL([OS_FREEBSD],[test "$os_freebsd" = "yes"])
AM_CONDITIONAL([OS_ES],[test "$os_es" = "yes"])

AM_CONDITIONAL(ES, test x$os_type = xes)
AM_CONDITIONAL(POSIX, test x$os_type = xposix)

# Font backend conditionals
AM_CONDITIONAL([USE_FREETYPE], [test "$with_font_backend" = "freetype"])
AM_CONDITIONAL([USE_PANGO], [test "$with_font_backend" = "pango"])

# WebKit feature conditionals
AM_CONDITIONAL([ENABLE_DEBUG],[test "$enable_debug" = "yes"])
AM_CONDITIONAL([ENABLE_DASHBOARD_SUPPORT],[test "$enable_dashboard_support" = "yes"])
AM_CONDITIONAL([ENABLE_3D_TRANSFORMS],[test "$enable_3D_transforms" = "yes"])
AM_CONDITIONAL([ENABLE_CHANNEL_MESSAGING],[test "$enable_channel_messaging" = "yes"])
AM_CONDITIONAL([ENABLE_JAVASCRIPT_DEBUGGER],[test "$enable_javascript_debugger" = "yes"])
AM_CONDITIONAL([ENABLE_OFFLINE_WEB_APPLICATIONS],[test "$enable_offline_web_applications" = "yes"])
AM_CONDITIONAL([ENABLE_DOM_STORAGE],[test "$enable_dom_storage" = "yes"])
AM_CONDITIONAL([ENABLE_DATABASE],[test "$enable_database" = "yes"])
AM_CONDITIONAL([ENABLE_ICONDATABASE],[test "$enable_icon_database" = "yes"])
AM_CONDITIONAL([ENABLE_XPATH],[test "$enable_xpath" = "yes"])
AM_CONDITIONAL([ENABLE_XSLT],[test "$enable_xslt" = "yes"])
AM_CONDITIONAL([ENABLE_GEOLOCATION], [test "$enable_geolocation" = "yes"])
AM_CONDITIONAL([ENABLE_VIDEO],[test "$enable_video" = "yes"])
AM_CONDITIONAL([ENABLE_SVG],[test "$enable_svg" = "yes"])
AM_CONDITIONAL([ENABLE_SVG_ANIMATION],[test "$enable_svg_animation" = "yes"])
AM_CONDITIONAL([ENABLE_SVG_FILTERS],[test "$enable_svg_filters" = "yes"])
AM_CONDITIONAL([ENABLE_SVG_FONTS],[test "$enable_svg_fonts" = "yes"])
AM_CONDITIONAL([ENABLE_SVG_FOREIGN_OBJECT],[test "$enable_svg_foreign_object" = "yes"])
AM_CONDITIONAL([ENABLE_SVG_AS_IMAGE],[test "$enable_svg_as_image" = "yes"])
AM_CONDITIONAL([ENABLE_SVG_USE],[test "$enable_svg_use_element" = "yes"])
AM_CONDITIONAL([ENABLE_COVERAGE],[test "$enable_coverage" = "yes"])
AM_CONDITIONAL([ENABLE_FAST_MALLOC],[test "$enable_fast_malloc" = "yes"])
AM_CONDITIONAL([ENABLE_WML],[test "$enable_wml" = "yes"])
AM_CONDITIONAL([ENABLE_WORKERS],[test "$enable_web_workers" = "yes"])
AM_CONDITIONAL([SVG_FLAGS],[test "$svg_flags" = "yes"])
AM_CONDITIONAL([HTML_FLAGS],[test "$html_flags" = "yes"])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

echo "
WebKit was configured with the following options:

Build configuration:
 Enable debugging (slow)                                  : $enable_debug
 Enable GCC build optimization                            : $enable_optimizations
 Code coverage support                                    : $enable_coverage
 Unicode backend                                          : $unicode_backend
 Font backend                                             : $with_font_backend
 Optimized memory allocator                               : $enable_fast_malloc
Features:
 3D Transforms                                            : $enable_3D_transforms
 JIT compilation                                          : $enable_jit
 Dashboard support                                        : $enable_dashboard_support
 Geolocation support                                      : $enable_geolocation
 GNOME Keyring support                                    : $enable_gnomekeyring
 JavaScript debugger/profiler support                     : $enable_javascript_debugger
 HTML5 offline web applications support                   : $enable_offline_web_applications
 HTML5 channel messaging support                          : $enable_channel_messaging
 HTML5 client-side session and persistent storage support : $enable_dom_storage
 HTML5 client-side database storage support               : $enable_database
 HTML5 video element support                              : $enable_video
 Icon database support                                    : $enable_icon_database
 SVG support                                              : $enable_svg
 SVG animation support                                    : $enable_svg_animation
 SVG filters support                                      : $enable_svg_filters
 SVG fonts support                                        : $enable_svg_fonts
 SVG foreign object support                               : $enable_svg_foreign_object
 SVG as image support                                     : $enable_svg_as_image
 SVG use element support                                  : $enable_svg_use_element
 WML support                                              : $enable_wml
 Web Workers support                                      : $enable_web_workers
 XPATH support                                            : $enable_xpath
 XSLT support                                             : $enable_xslt
"

