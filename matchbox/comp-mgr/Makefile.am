
core_h    = mb-wm-comp-mgr.h
core_c    = mb-wm-comp-mgr.c

if COMP_MGR_BACKEND
if ENABLE_CLUTTER_COMPOSITE_MANAGER
clutter_h = mb-wm-comp-clutter.h
clutter_c = mb-wm-comp-mgr-clutter.c
else
xrender_h = mb-wm-comp-mgr-xrender.h
xrender_c = mb-wm-comp-mgr-xrender.c
endif
endif

pkgincludedir = $(includedir)/@MBWM2_INCDIR@/comp-mgr

if ENABLE_LIBMATCHBOX
pkginclude_HEADERS = mb-wm-comp-mgr-clutter.h  mb-wm-comp-mgr.h  mb-wm-comp-mgr-xrender.h
endif
noinst_LTLIBRARIES = libmatchbox-window-manager-2-compmgr.la
libmatchbox_window_manager_2_compmgr_la_SOURCES = $(core_h) $(core_c) \
						  $(xrender_h) $(xrender_c) \
						  $(clutter_h) $(clutter_c)
libmatchbox_window_manager_2_compmgr_la_CFLAGS = @MBWM_INCS@ @MBWM_CFLAGS@

MAINTAINERCLEANFILES = Makefile.in

