#include "mb-wm-theme-png.h"

#include <math.h>

#ifdef USE_GTK
#ifndef GTK_DISABLE_DEPRECATED
#define GTK_DISABLE_DEPRECATED
#endif
#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>
#endif

static void
mb_wm_theme_png_paint_decor (MBWMTheme *theme, MBWMDecor *decor);

static void
mb_wm_theme_png_paint_button (MBWMTheme *theme, MBWMDecorButton *button);

static Bool
mb_wm_theme_png_switch (MBWMTheme *theme, const char *detail);

static void
mb_wm_theme_png_class_init (MBWMObjectClass *klass)
{
  MBWMThemeClass *t_class = MB_WM_THEME_CLASS (klass);

  t_class->paint_decor  = mb_wm_theme_png_paint_decor;
  t_class->paint_button = mb_wm_theme_png_paint_button;
  t_class->theme_switch = mb_wm_theme_png_switch;
}

static void
mb_wm_theme_png_destroy (MBWMObject *obj)
{
}

static void
mb_wm_theme_png_init (MBWMObject *obj, va_list vap)
{
  MBWMThemePng * theme = MB_WM_THEME_PNG (obj);

}

int
mb_wm_theme_png_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMThemePngClass),
	sizeof (MBWMThemePng),
	mb_wm_theme_png_init,
	mb_wm_theme_png_destroy,
	mb_wm_theme_png_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_THEME, 0);
    }

  return type;
}

static void
mb_wm_theme_png_paint_decor (MBWMTheme *theme,
			     MBWMDecor *decor)
{
}

static void
mb_wm_theme_png_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
}

static Bool
mb_wm_theme_png_switch (MBWMTheme   *theme,
			  const char  *detail)
{
}

