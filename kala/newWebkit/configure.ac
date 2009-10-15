dnl Process this file with autoconf to produce a configure script
AC_INIT([es], [0.1.5])
AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE
AC_PROG_YACC
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LEX
AC_CONFIG_FILES(Makefile include/Makefile)
AC_CONFIG_SUBDIRS(esidl tools os init cmd esjs)

case ${target} in
  *-*-es)
	os_type=es
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

AM_CONDITIONAL(ES, test x$os_type = xes)

AC_SUBST(ESIDL)
AC_SUBST(TOOLS)
AC_SUBST(INIT)
AC_SUBST(CMD)

AC_OUTPUT
