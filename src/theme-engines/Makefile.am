if THEME_CAIRO
CAIRO_SRC = mb-wm-theme-cairo.c mb-wm-theme-cairo.h
endif

if THEME_SIMPLE
SIMPLE_SRC = mb-wm-theme-simple.c mb-wm-theme-simple.h
endif

if THEME_PNG
PNG_SRC = mb-wm-theme-png.c mb-wm-theme-png.h
endif

COMMON_SRC = mb-wm-theme.h mb-wm-theme.c

noinst_LIBRARIES = libmb-theme.a

libmb_theme_a_SOURCES = $(COMMON_SRC) $(CAIRO_SRC) $(SIMPLE_SRC) $(PNG_SRC)

INCLUDES = @MBWM_INCS@ @MBWM_CFLAGS@ -DDATADIR=\"$(datadir)\"

MAINTAINERCLEANFILES = config.h.in Makefile.in
