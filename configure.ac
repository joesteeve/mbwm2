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

AC_ARG_ENABLE(cairo-theme,
  [  --disable-cairo-theme   Disable Cairo graphics rendering support],
  [use_cairo=$enableval], [use_cairo=yes])

AC_ARG_ENABLE(png-theme,
  [  --enable-png-theme      Enable PNG-based theme],
  [png_theme=$enableval], [png_theme=no])

AC_ARG_ENABLE(gtk,
  [  --disable-gtk           Disable GTK integration support],
  [use_gtk=$enableval], [use_gtk=yes])

AC_ARG_ENABLE(debug,
  [  --enable-debug          Enable verbose debugging output],
  [want_debug=$enableval], [want_debug=no])

if test "x$use_cairo" = "xyes"; then
  needed_pkgs="$needed_pkgs pangocairo "
fi

if test "x$use_gtk" = "xyes"; then
  needed_pkgs="$needed_pkgs gtk+-2.0 "
fi

if test "x$want_debug" = "xyes"; then
  MBWM_DEBUG_CFLAGS="-O0 $MBWM_DEBUG_CFLAGS -DMBWM_WANT_DEBUG"
  MBWM_DEBUG_LDFLAGS="$MBWM_DEBUG_LDFLAGS -rdynamic"
fi

PKG_CHECK_MODULES(MBWM, $needed_pkgs)

AM_CONDITIONAL(THEME_CAIRO, [test "x$use_cairo" = "xyes"])
if test "$use_cairo" = yes; then
  AC_DEFINE(USE_CAIRO, 1, [Use cairo for theme rendering])
fi

AM_CONDITIONAL(THEME_PNG, [test "x$png_theme" = "xyes"])
if test "$png_theme" = yes; then
  AC_DEFINE(THEME_PNG, 1, [Build PNG-image based theme engine])
fi

# only build simple theme if cairo theme disabled
AM_CONDITIONAL(THEME_SIMPLE, [test "x$use_cairo" != "xyes"])

AM_CONDITIONAL(USE_GTK, test "x$use_gtk" = "xyes")
if test "$use_gtk" = yes; then
  AC_DEFINE(USE_GTK, 1, [GTK Integration])
fi

MBWM_INCS='-I$(top_srcdir)/src/core -I$(top_srcdir)/src/client-types -I$(top_srcdir)/src/theme-engines'
MBWM_CORE_LIB='$(top_builddir)/src/core/libmatchbox-window-manager-2-core.a'
MBWM_CLIENT_LIBS='$(top_builddir)/src/client-types'
MBWM_THEME_LIBS='$(top_builddir)/src/theme-engines'
MBWM_CFLAGS="$MBWM_CFLAGS $MBWM_DEBUG_CFLAGS"

AC_SUBST([MBWM_CFLAGS])
AC_SUBST([MBWM_LIBS])
AC_SUBST([MBWM_CORE_LIB])
AC_SUBST([MBWM_INCS])
AC_SUBST([MBWM_CLIENT_LIBS])
AC_SUBST([MBWM_THEME_LIBS])
AC_SUBST([MBWM_DEBUG_CFLAGS])
AC_SUBST([MBWM_DEBUG_LDFLAGS])

AC_OUTPUT([
Makefile
src/Makefile  
src/core/Makefile
src/client-types/Makefile
src/managers/Makefile
src/managers/simple/Makefile
src/theme-engines/Makefile
data/Makefile
data/themes/Makefile
data/themes/Default/Makefile
])

echo "
           Matchbox Window Manager II
           ==========================

        install prefix:         ${prefix}
        source code location:   ${srcdir}

	Cairo theme:            ${use_cairo}
	PNG theme:              ${png_theme}
	GTK integration:	${use_gtk}
	Debugging output:       ${want_debug}
"