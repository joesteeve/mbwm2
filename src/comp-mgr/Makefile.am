noinst_LIBRARIES = libmatchbox-window-manager-2-compmgr.a

core_h    = mb-wm-comp-mgr.h

if USE_CLUTTER
clutter_h = mb-wm-comp-clutter.h
else
default_h = mb-wm-comp-mgr-default.h
endif

core_c    = mb-wm-comp-mgr.c


if USE_CLUTTER
clutter_c = mb-wm-comp-mgr-clutter.c
else
default_c = mb-wm-comp-mgr-default.c
endif

libmatchbox_window_manager_2_compmgr_a_SOURCES = $(core_h) $(core_c)	   \
						 $(default_h) $(default_c) \
						 $(clutter_h) $(clutter_c)

libmatchbox_window_manager_2_compmgr_a_CFLAGS = @MBWM_INCS@ @MBWM_CFLAGS@

MAINTAINERCLEANFILES = Makefile.in