noinst_LIBRARIES = libmatchbox-window-manager-2-core.a

core_h  = mb-wm.h             \
          mb-wm-client.h      \
          mb-wm-client-base.h \
	  mb-wm-window.h      \
          mb-wm-stack.h       \
          mb-wm-util.h        \
          mb-wm-types.h       \
	  mb-wm-atoms.h       \
          mb-wm-core.h        \
          mb-wm-layout.h      \
          mb-wm-props.h       \
          mb-wm-keys.h        \
          xas.h

core_c  = mb-wm-client.c      \
          mb-wm-client-base.c \
	  mb-wm-window.c      \
          mb-wm-stack.c       \
          mb-wm-props.c       \
          mb-wm-util.c        \
	  mb-wm-atoms.c       \
          mb-wm-core.c        \
          mb-wm-layout.c      \
          mb-wm-keys.c        \
          xas.c

libmatchbox_window_manager_2_core_a_SOURCES = $(core_h) $(core_c)

libmatchbox_window_manager_2_core_a_INCLUDES = @MBWM_INCS@ @MBWM_CFLAGS@



MAINTAINERCLEANFILES = config.h.in Makefile.in