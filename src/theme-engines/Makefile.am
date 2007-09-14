COMMON_SRC = mb-wm-theme.h mb-wm-theme.c

if USE_CAIRO
  libmb_theme_a_SOURCES =	$(COMMON_SRC) \
				mb-wm-theme-cairo.c \
				mb-wm-theme-cairo.h
else
  libmb_theme_a_SOURCES = 	$(COMMON_SRC) \
				mb-wm-theme-simple.c \
				mb-wm-theme-simple.h
endif

noinst_LIBRARIES = libmb-theme.a 

INCLUDES = @MBWM_INCS@ @MBWM_CFLAGS@



MAINTAINERCLEANFILES = config.h.in Makefile.in
