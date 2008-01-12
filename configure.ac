dnl Process this file with autoconf to produce a configure script
AC_INIT(es, 0.0.1)
AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE
AC_PROG_YACC
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LEX
AC_CONFIG_FILES(Makefile include/Makefile)
AC_CONFIG_SUBDIRS(esidl tools os init cmd)

case ${target} in
  *-*-es)
	ESIDL=
	TOOLS=
	INIT="init"
	CMD="cmd"
	;;
  *)
	ESIDL="esidl"
	TOOLS="tools"
	INIT=
	CMD=
	;;
esac

AC_SUBST(ESIDL)
AC_SUBST(TOOLS)
AC_SUBST(INIT)
AC_SUBST(CMD)

AC_OUTPUT
