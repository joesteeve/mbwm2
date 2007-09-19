noinst_LIBRARIES = libmb-wm-client-app.a  \
	           libmb-wm-client-panel.a \
	           libmb-wm-client-dialog.a \
		   libmb-wm-client-input.a

THEME_LIBS = @MBWM_THEME_LIBS@/libmb-theme.a

INCLUDES     = @MBWM_INCS@ @MBWM_CFLAGS@
LDADD        =$(THEME_LIBS)
DEPENDENCIES = $(THEME_LIBS)             

libmb_wm_client_app_a_SOURCES   = mb-wm-client-app.c mb-wm-client-app.h
libmb_wm_client_panel_a_SOURCES = mb-wm-client-panel.c mb-wm-client-panel.h
libmb_wm_client_dialog_a_SOURCES   = mb-wm-client-dialog.c mb-wm-client-dialog.h
libmb_wm_client_input_a_SOURCES   = mb-wm-client-input.c mb-wm-client-input.h


MAINTAINERCLEANFILES = config.h.in Makefile.in
