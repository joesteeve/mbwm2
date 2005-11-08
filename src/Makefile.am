SUBDIRS = core

INCLUDES = 					\
	@MBWM_INCS@				\
	@MBWM_CFLAGS@

bin_PROGRAMS = matchbox-window-manager-2

matchbox_window_manager_2_SOURCES =             \
	matchbox-window-manager-2.c

matchbox_window_manager_2_LDADD =               \
	@MBWM_LIBS@				\
        @MBWM_CORE_LIB@	

matchbox_window_manager_2_DEPENDENCIES =	\
        @MBWM_CORE_LIB@	


MAINTAINERCLEANFILES = config.h.in Makefile.in