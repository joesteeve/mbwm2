noinst_LIBRARIES = libmb-wm-client-app.a   \
	           libmb-wm-client-panel.a 

THEME_LIBS = @MBWM_THEME_LIBS@/libmb-theme.a

INCLUDES     = @MBWM_INCS@ @MBWM_CFLAGS@
LDADD        =$(THEME_LIBS)
DEPENDENCIES = $(THEME_LIBS)             

libmb_wm_client_app_a_SOURCES   = mb-wm-client-app.c mb-wm-client-app.h
libmb_wm_client_panel_a_SOURCES = mb-wm-client-panel.c mb-wm-client-panel.h

MAINTAINERCLEANFILES = config.h.in Makefile.in
