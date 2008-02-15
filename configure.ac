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

AC_ARG_ENABLE(compositing-manager,
  [  --enable-compositing-manager@<:@=BACKEND@:>@  Enable compositing manager,
                          optionally choosing compositor backend. Available
                          backends are: 'default' (XRender based backend),
                                        'clutter' (Clutter based backend).],
  [comp_mgr=$enableval], [comp_mgr=no])

AC_ARG_ENABLE(glib-main-loop,
  [  --enable-glib-main-loop      Enable use of glib main loop],
  [gmloop=$enableval], [gmloop=no])

if test "x$use_cairo" = "xyes"; then
  needed_pkgs="$needed_pkgs pangocairo "
else
  needed_pkgs="$needed_pkgs xft "
fi

if test "x$use_gtk" = "xyes"; then
  needed_pkgs="$needed_pkgs gtk+-2.0 "
fi

case $comp_mgr in
     yes | default ) comp_mgr="yes (default)"
                     needed_pkgs="$needed_pkgs xcomposite xdamage " ;;

     clutter )       comp_mgr="yes (clutter)"
                     needed_pkgs="$needed_pkgs clutter-0.5 xcomposite xdamage "
	             gmloop="yes" ;;

     * )             comp_mgr = no ;;
esac

if test "x$want_debug" = "xyes"; then
  MBWM_DEBUG_CFLAGS="-O0 $MBWM_DEBUG_CFLAGS -DMBWM_WANT_DEBUG"
  MBWM_DEBUG_LDFLAGS="$MBWM_DEBUG_LDFLAGS -rdynamic"
fi

if test "x$gmloop" = "xyes"; then
  needed_pkgs="$needed_pkgs glib-2.0 "
  AC_DEFINE(USE_GLIB_MAINLOOP, 1, [Use glib main loop])
fi

PKG_CHECK_MODULES(MBWM, $needed_pkgs)

AM_CONDITIONAL(THEME_CAIRO, [test "x$use_cairo" = "xyes"])
if test "$use_cairo" = yes; then
  AC_DEFINE(USE_CAIRO, 1, [Use cairo for theme rendering])
fi

AM_CONDITIONAL(THEME_PNG, [test "x$png_theme" = "xyes"])
if test "$png_theme" = yes; then
  AC_DEFINE(THEME_PNG, 1, [Build PNG-image based theme engine])
  PKG_CHECK_MODULES(THEME_PNG, xft libpng12)
fi

# only build simple theme if cairo theme disabled
AM_CONDITIONAL(THEME_SIMPLE, [test "x$use_cairo" != "xyes"])

AM_CONDITIONAL(USE_GTK, test "x$use_gtk" = "xyes")
if test "$use_gtk" = yes; then
  AC_DEFINE(USE_GTK, 1, [GTK Integration])
fi

AM_CONDITIONAL(ENABLE_COMPOSITE, [test "x$comp_mgr" != "xno"])
if test "$comp_mgr" != no; then
  AC_DEFINE(ENABLE_COMPOSITE, 1, [Enable composite manager code])
fi

AM_CONDITIONAL(USE_CLUTTER, [test "x$comp_mgr" = "xyes (clutter)"])
if test "$comp_mgr" = "yes (clutter)"; then
  AC_DEFINE(USE_CLUTTER, 1, [Use clutter for compositing])
fi

AC_ARG_ENABLE(simple-manager,
  [  --disable-simple-manager   Do not build simple window manager],
  [simple_manager=$enableval], [simple_manager=yes])

AC_ARG_ENABLE(maemo-manager,
  [  --enable-maemo-manager   Build maemo window manager],
  [maemo_manager=$enableval], [maemo_manager=no])

if test $simple_manager = yes; then
   MANAGERS="$MANAGERS simple"
fi

if test $maemo_manager = yes; then
   MANAGERS="$MANAGERS maemo"
fi

AC_SUBST(MANAGERS)

PKG_CHECK_MODULES(XFIXES, xfixes >= 4.0, have_xfixes=yes, have_xfixes=no)

if test x$have_xfixes = xyes; then
  AC_DEFINE(HAVE_XFIXES, [1], [Use XFixes ext to really hide cursor])
fi

PKG_CHECK_MODULES(XEXT, xext, have_xext=yes, have_xext=no)

if test x$have_xext = xyes; then
  AC_DEFINE(HAVE_XEXT, [1], [Use Xext ext])
fi

PKG_CHECK_MODULES(XCURSOR, xcursor, have_xcursor=yes, have_xcursor=no)

if test x$have_xcursor = xyes; then
  AC_DEFINE(HAVE_XCURSOR, [1], [Use XCursor to sync pointer themes])
fi

MBWM_INCS='-I$(top_srcdir)/src/core -I$(top_srcdir)/src/client-types -I$(top_srcdir)/src/theme-engines -I$(top_srcdir)/src/comp-mgr'
MBWM_CORE_LIB='$(top_builddir)/src/core/libmatchbox-window-manager-2-core.a'
MBWM_CLIENT_LIBS='$(top_builddir)/src/client-types'
MBWM_THEME_LIBS='$(top_builddir)/src/theme-engines'
MBWM_COMPMGR_LIBS='$(top_builddir)/src/comp-mgr'
MBWM_CFLAGS="$MBWM_CFLAGS $MBWM_DEBUG_CFLAGS $THEME_PNG_CFLAGS $XFIXES_CFLAGS $XEXT_CFLAGS $XCURSOR_CFLAGS"
MBWM_LIBS="$MBWM_LIBS $THEME_PNG_LIBS $XFIXES_LIBS $XEXT_LIBS $XCURSOR_LIBS"

AC_SUBST([MBWM_CFLAGS])
AC_SUBST([MBWM_LIBS])
AC_SUBST([MBWM_CORE_LIB])
AC_SUBST([MBWM_INCS])
AC_SUBST([MBWM_CLIENT_LIBS])
AC_SUBST([MBWM_THEME_LIBS])
AC_SUBST([MBWM_COMPMGR_LIBS])
AC_SUBST([MBWM_DEBUG_CFLAGS])
AC_SUBST([MBWM_DEBUG_LDFLAGS])

AC_OUTPUT([
Makefile
src/Makefile
src/core/Makefile
src/client-types/Makefile
src/comp-mgr/Makefile
src/managers/Makefile
src/managers/simple/Makefile
src/managers/maemo/Makefile
src/managers/maemo/theme/Makefile
src/theme-engines/Makefile
data/Makefile
data/themes/Makefile
data/themes/Default/Makefile
doc/Makefile
])

echo "
           Matchbox Window Manager II
           ==========================

    Directories:
        source code location:   ${srcdir}
        install prefix      :   ${prefix}

    Extension:
	Xfixes              :   ${have_xfixes}
	Xext                :   ${have_xext}
	Xcursor             :   ${have_xcursor}

    Themes:
	Cairo theme         :   ${use_cairo}
	PNG theme           :   ${png_theme}
	GTK integration     :   ${use_gtk}

    Managers:
	Simple manager      :	${simple_manager}
	Maemo manager       :	${maemo_manager}

	Compositing manager :   ${comp_mgr}

    Miscel:
        Glib main loop      :   ${gmloop}
	Debugging output    :   ${want_debug}
"
