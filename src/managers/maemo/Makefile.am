INCLUDES = 					       \
	@MBWM_INCS@				       \
	@MBWM_CFLAGS@

CLIENT_LIBS =						\
        @MBWM_CLIENT_LIBS@/libmb-wm-client-panel.a	\
        @MBWM_CLIENT_LIBS@/libmb-wm-client-dialog.a	\
        @MBWM_CLIENT_LIBS@/libmb-wm-client-app.a	\
        @MBWM_CLIENT_LIBS@/libmb-wm-client-input.a	\
        @MBWM_CLIENT_LIBS@/libmb-wm-client-desktop.a


THEME_LIBS = @MBWM_THEME_LIBS@/libmb-theme.a

bin_PROGRAMS = matchbox-window-manager-2-maemo

matchbox_window_manager_2_maemo_SOURCES =             \
	maemo-toolbar.c                               \
	maemo-window-manager.c                        \
	matchbox-window-manager-2-maemo.c

matchbox_window_manager_2_maemo_LDFLAGS =             \
	$(MBWM_DEBUG_LDFLAGS)                         \
	$(LDFLAGS)

matchbox_window_manager_2_maemo_LDADD =               \
	@MBWM_LIBS@				      \
        @MBWM_CORE_LIB@	                              \
	$(THEME_LIBS)                                 \
	$(CLIENT_LIBS)

matchbox_window_manager_2_maemo_DEPENDENCIES =	      \
        @MBWM_CORE_LIB@	                              \
	$(THEME_LIBS)                                 \
        $(CLIENT_LIBS)

MAINTAINERCLEANFILES = config.h.in Makefile.in