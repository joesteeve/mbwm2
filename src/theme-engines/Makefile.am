if USE_CAIRO
  libmb_theme_a_SOURCES = mb-wm-theme.h theme-cairo.c
else
  libmb_theme_a_SOURCES = mb-wm-theme.h theme-simple.c
endif

noinst_LIBRARIES = libmb-theme.a 

INCLUDES = @MBWM_INCS@ @MBWM_CFLAGS@



MAINTAINERCLEANFILES = config.h.in Makefile.in
