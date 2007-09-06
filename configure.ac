AC_PREREQ(2.59)
AC_INIT([matchbox-window-manager-2], 0.0.0, [mallum@handhelds.org])
AC_CONFIG_SRCDIR([src/core/mb-window-manager.c])

AM_INIT_AUTOMAKE()
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AM_PROG_LIBTOOL

AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])
AC_C_CONST
AC_CHECK_FUNCS([memset strdup strncasecmp])

needed_pkgs="x11 "

AC_ARG_ENABLE(cairo,
  [  --disable-cairo         Disable Cairo graphics rendering support],
  [use_cairo=$enableval], [use_cairo=yes])

AC_ARG_ENABLE(gtk,
  [  --disable-gtk           Disable GTK integration support],
  [use_gtk=$enableval], [use_gtk=yes])


if test "x$use_cairo" = "xyes"; then
  needed_pkgs="$needed_pkgs pangocairo "
fi

if test "x$use_gtk" = "xyes"; then
  needed_pkgs="$needed_pkgs gtk+-2.0 "
fi

PKG_CHECK_MODULES(MBWM, $needed_pkgs)

AM_CONDITIONAL(USE_CAIRO, test "x$use_cairo" = "xyes")
if test "$use_cairo" = yes; then
  AC_DEFINE(USE_CAIRO, 1, [Use cairo for theme rendering])
fi

AM_CONDITIONAL(USE_GTK, test "x$use_gtk" = "xyes")
if test "$use_gtk" = yes; then
  AC_DEFINE(USE_GTK, 1, [GTK Integration])
fi

MBWM_INCS='-I$(top_srcdir)/src/core -I$(top_srcdir)/src/client-types -I$(top_srcdir)/src/theme-engines'
MBWM_CORE_LIB='$(top_builddir)/src/core/libmatchbox-window-manager-2-core.a'
MBWM_CLIENT_LIBS='$(top_builddir)/src/client-types'
MBWM_THEME_LIBS='$(top_builddir)/src/theme-engines'

AC_SUBST([MBWM_CFLAGS])
AC_SUBST([MBWM_LIBS])
AC_SUBST([MBWM_CORE_LIB])
AC_SUBST([MBWM_INCS])
AC_SUBST([MBWM_CLIENT_LIBS])
AC_SUBST([MBWM_THEME_LIBS])

AC_OUTPUT([
Makefile
src/Makefile  
src/core/Makefile
src/client-types/Makefile
src/managers/Makefile
src/theme-engines/Makefile
])

echo "
           Matchbox Window Manager II
           ==========================

        install prefix:         ${prefix}
        source code location:   ${srcdir}

	Cairo support:          ${use_cairo}
	GTK integration:	${use_gtk}
"