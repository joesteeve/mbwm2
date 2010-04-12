SUBDIRS = theme

INCLUDES = 					       \
	$(MBWM_INCS)				       \
	$(MBWM_CFLAGS)

CLIENT_LIBS =						\
        $(MBWM_CLIENT_BUILDDIR)/libmb-wm-client-panel.la	\
        $(MBWM_CLIENT_BUILDDIR)/libmb-wm-client-dialog.la	\
        $(MBWM_CLIENT_BUILDDIR)/libmb-wm-client-note.la	\
        $(MBWM_CLIENT_BUILDDIR)/libmb-wm-client-app.la	\
        $(MBWM_CLIENT_BUILDDIR)/libmb-wm-client-input.la	\
        $(MBWM_CLIENT_BUILDDIR)/libmb-wm-client-desktop.la	\
        $(MBWM_CLIENT_BUILDDIR)/libmb-wm-client-menu.la

THEME_LIBS = $(MBWM_THEME_BUILDDIR)/libmb-theme.la

if ENABLE_COMPOSITE
COMPMGR_LIBS=$(MBWM_COMPMGR_BUILDDIR)/libmatchbox-window-manager-2-compmgr.la \
	     $(MBWM_CLIENT_BUILDDIR)/libmb-wm-client-override.la
endif

bin_PROGRAMS = matchbox-window-manager-2-maemo

matchbox_window_manager_2_maemo_SOURCES =		\
	maemo-toolbar.c					\
	maemo-input.c					\
	maemo-window-manager.c				\
	matchbox-window-manager-2-maemo.c

matchbox_window_manager_2_maemo_LDFLAGS =		\
	$(MBWM_DEBUG_LDFLAGS)				\
	$(LDFLAGS)

matchbox_window_manager_2_maemo_LDADD =			\
        $(MBWM_CORE_LIB)					\
	$(THEME_LIBS)					\
	$(CLIENT_LIBS)					\
	$(COMPMGR_LIBS)					\
	$(MBWM_LIBS)

matchbox_window_manager_2_maemo_DEPENDENCIES =		\
        $(MBWM_CORE_LIB)					\
	$(THEME_LIBS)					\
        $(CLIENT_LIBS)					\
	$(COMPMGR_LIBS)

MAINTAINERCLEANFILES = config.h.in Makefile.in