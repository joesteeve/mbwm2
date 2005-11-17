AC_PREREQ(2.59)
AC_INIT([matchbox-window-manager-2], 0.0.0, [mallum@handhelds.org])
AC_CONFIG_SRCDIR([src/core/mb-wm-core.c])

AM_INIT_AUTOMAKE()
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AM_PROG_LIBTOOL

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])
AC_C_CONST
AC_CHECK_FUNCS([memset strdup strncasecmp])

PKG_CHECK_MODULES(MBWM, x11,,
	         AC_MSG_ERROR([*** You need to install modern xlibs ***]))

MBWM_INCS='-I$(top_srcdir)/src/core -I$(top_srcdir)/src/client-types'
MBWM_CORE_LIB='$(top_builddir)/src/core/libmatchbox-window-manager-2-core.a'
MBWM_CLIENT_LIBS='$(top_builddir)/src/client-types'

AC_SUBST([MBWM_CFLAGS])
AC_SUBST([MBWM_LIBS])
AC_SUBST([MBWM_CORE_LIB])
AC_SUBST([MBWM_INCS])
AC_SUBST([MBWM_CLIENT_LIBS])

AC_OUTPUT([
Makefile
src/Makefile  
src/core/Makefile
src/client-types/Makefile
src/managers/Makefile
])
