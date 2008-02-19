
core_h    = mb-wm-comp-mgr.h
core_c    = mb-wm-comp-mgr.c

if USE_CLUTTER
clutter_h = mb-wm-comp-clutter.h
clutter_c = mb-wm-comp-mgr-clutter.c
else
default_h = mb-wm-comp-mgr-default.h
default_c = mb-wm-comp-mgr-default.c
endif

pkgincludedir = $(includedir)/@MBWM2_INCDIR@/comp-mgr

if ENABLE_LIBMATCHBOX
pkginclude_HEADERS = *.h
noinst_LTLIBRARIES = libmatchbox-window-manager-2-compmgr.la
libmatchbox_window_manager_2_compmgr_la_SOURCES = $(core_h) $(core_c)
libmatchbox_window_manager_2_compmgr_la_CFLAGS = @MBWM_INCS@ @MBWM_CFLAGS@
else
noinst_LIBRARIES = libmatchbox-window-manager-2-compmgr.a

libmatchbox_window_manager_2_compmgr_a_SOURCES = $(core_h) $(core_c)	   \
						 $(default_h) $(default_c) \
						 $(clutter_h) $(clutter_c)

libmatchbox_window_manager_2_compmgr_a_CFLAGS = @MBWM_INCS@ @MBWM_CFLAGS@
endif

MAINTAINERCLEANFILES = Makefile.in
