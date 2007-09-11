INCLUDES = 					       \
	@MBWM_INCS@				       \
	@MBWM_CFLAGS@

CLIENT_LIBS =                                          \
        @MBWM_CLIENT_LIBS@/libmb-wm-client-panel.a     \
        @MBWM_CLIENT_LIBS@/libmb-wm-client-dialog.a    \
        @MBWM_CLIENT_LIBS@/libmb-wm-client-app.a       

THEME_LIBS = @MBWM_THEME_LIBS@/libmb-theme.a

bin_PROGRAMS = matchbox-window-manager-2-simple

matchbox_window_manager_2_simple_SOURCES =             \
	matchbox-window-manager-2-simple.c

matchbox_window_manager_2_simple_LDFLAGS =             \
	$(MBWM_DEBUG_LDFLAGS)                          \
	$(LDFLAGS)

matchbox_window_manager_2_simple_LDADD =               \
	@MBWM_LIBS@				       \
        @MBWM_CORE_LIB@	                               \
	$(THEME_LIBS)                                  \
	$(CLIENT_LIBS)

matchbox_window_manager_2_simple_DEPENDENCIES =	       \
        @MBWM_CORE_LIB@	                               \
	$(THEME_LIBS)                                  \
        $(CLIENT_LIBS)

MAINTAINERCLEANFILES = config.h.in Makefile.in