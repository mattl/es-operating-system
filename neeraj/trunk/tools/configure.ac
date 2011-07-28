dnl Process this file with autoconf to produce a configure script
AC_INIT([es-tools], [0.1.5])
AC_CONFIG_AUX_DIR(.)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE
AC_PROG_CXX
AC_PROG_CC

case ${target} in
  *-apple-*)
    vendor_type=apple
    ;;
  *)
    ;;
esac

AM_CONDITIONAL(APPLE, test x$vendor_type = xapple)

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
