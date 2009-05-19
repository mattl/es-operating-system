dnl Process this file with autoconf to produce a configure script
AC_INIT([es-init], [0.1.4])
AC_CONFIG_AUX_DIR(.)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AS
AC_PROG_RANLIB

case ${target} in
  i[[34567]]86-*-*)
	cpu_type=i386
	;;
  x86_64-*-*)
	cpu_type=x86_64
	;;
  *)
	echo 'unknown cpu type';
	exit 1
	;;
esac

AM_CONDITIONAL(I386, test x$cpu_type = xi386)
AM_CONDITIONAL(X86_64, test x$cpu_type = xx86_64)

case ${target} in
  *-*-es)
    os_type=es
    ;;
  *-*-mingw32)
    os_type=win32
    ;;
  *)
    os_type=posix
    ;;
esac

AM_CONDITIONAL(ES, test x$os_type = xes)
AM_CONDITIONAL(WIN32, test x$os_type = xwin32)
AM_CONDITIONAL(POSIX, test x$os_type = xposix)

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
