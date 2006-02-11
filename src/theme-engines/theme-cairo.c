#include "mb-wm-theme.h"

#ifdef USE_GTK
#ifndef GTK_DISABLE_DEPRECATED
#define GTK_DISABLE_DEPRECATED
#endif
#include <gtk/gtk.h>
#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>
#endif

#include <cairo.h>
#include <cairo-xlib.h>

#if CAIRO_VERSION < CAIRO_VERSION_ENCODE(1, 1, 0)
#define CAIRO_EXTEND_PAD CAIRO_EXTEND_NONE
#endif

#include <pango/pango-context.h>
#include <pango/pangocairo.h>

typedef struct ThemeCairo
{
  


} 
ThemeCairo;

static ThemeCairo *CairoTheme = NULL;

void
mb_wm_theme_paint_decor (MBWindowManager   *wm,
			 MBWMDecor         *decor)
{
  MBWMDecorType          type;
  const MBGeometry      *geom;
  MBWindowManagerClient *client;
  Window                 xwin;
  Pixmap                 xpxmp;

  cairo_pattern_t       *pattern;
  cairo_matrix_t         matrix;
  cairo_surface_t       *surface;
  cairo_t               *cr;
  double                 x, y, w, h;


  client = mb_wm_decor_get_parent (decor);
  xwin = mb_wm_decor_get_x_window (decor);

  if (client == NULL || xwin == None) return;

  type = mb_wm_decor_get_type (decor);
  geom = mb_wm_decor_get_geometry (decor);

  xpxmp = XCreatePixmap (wm->xdpy, xwin, geom->width, geom->height, 
			DefaultDepth(wm->xdpy, wm->xscreen));
  
  surface = cairo_xlib_surface_create  (wm->xdpy,
					xpxmp,
					DefaultVisual(wm->xdpy, wm->xscreen),
					geom->width,
					geom->height);
  cr = cairo_create(surface);

  cairo_set_line_width (cr, 1.0);

  switch (type)
    {
    case MBWMDecorTypeNorth:
      break;
    case MBWMDecorTypeSouth:
    case MBWMDecorTypeWest:
    case MBWMDecorTypeEast:
      break;
    default:
      break;
    }

  w = geom->width; h = geom->height; x = geom->x; y = geom->y;

  /*
  pattern = cairo_pattern_create_radial (0.0, 0.0, 0.0, 0.0, 0.0, geom->width);

  cairo_matrix_init_scale (&matrix, 1.0, w / h);
  cairo_matrix_translate (&matrix, -(x + w), -(y + h));

  cairo_pattern_set_matrix (pattern, &matrix);

  cairo_pattern_add_color_stop_rgba (pattern, 0.0, 0.5, 0.1, 0.1, 0.5);

  cairo_pattern_add_color_stop_rgba (pattern, 1.0, 1.0, 0.5, 1.0, 0.5);

  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);

  cairo_set_source (cr, pattern);
  cairo_fill (cr);
  */

  cairo_set_line_width (cr, 0.04);
  cairo_set_source_rgb (cr, 0.9, 0.9, 0.8);

  cairo_stroke (cr);

  pattern = cairo_pattern_create_linear (0, 0, 
					 w > h ? 0 : w, 
					 h > w ? 0 : h);

  cairo_pattern_add_color_stop_rgb (pattern, 1, 0.2, 0.2, 0.3 );
  cairo_pattern_add_color_stop_rgb (pattern, 0, 0.7, 0.7, 0.8 );

  cairo_set_source (cr, pattern);

  cairo_rectangle( cr, 0, 0, w, h); 
  cairo_fill (cr);

  if (mb_wm_decor_get_type(decor) == MBWMDecorTypeNorth)
    {
      cairo_font_extents_t font_extents;

      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

      cairo_font_extents (cr, &font_extents);

      cairo_select_font_face (cr,
			      "Sans Serif",
			      CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);
  
      cairo_set_font_size (cr, h - (h/8));

      cairo_move_to (cr, 
		     mb_wm_client_frame_west_width (client), 
		     (h - (font_extents.ascent + font_extents.descent)) / 2
		     + font_extents.ascent);

      cairo_show_text (cr, mb_wm_client_get_name(client));
    }

  cairo_pattern_destroy (pattern);
  cairo_destroy (cr);

  XSetWindowBackgroundPixmap (wm->xdpy, xwin, xpxmp);
  XClearWindow (wm->xdpy, xwin);
  
  XFreePixmap (wm->xdpy, xpxmp);
}

void
mb_wm_theme_paint_button (MBWindowManager   *wm,
			  MBWMDecorButton   *button)
{

}

Bool
mb_wm_theme_switch (MBWindowManager   *wm,
		    const char        *detail)
{

}

Bool
mb_wm_theme_init (MBWindowManager   *wm)
{
  PangoFontDescription *desc;
#ifdef USE_GTK
  GtkWindow            *gwin;
#endif

#ifdef USE_GTK
  /* 
   * Plan here is to just get the GTK settings so we can follow
   * colors set by widgets.  
  */

  gtk_init (NULL, NULL);

  gwin = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_ensure_style (gwin);

  
#endif

  /*
   font->pgo_context = pango_xft_get_context (font->dpy, DefaultScreen(dpy));
   font->pgo_fontmap = pango_xft_get_font_map (font->dpy, DefaultScreen(dpy));
   font->fontdes     = pango_font_description_new ();
   
   pango_context = gtk_widget_create_pango_context (style_window);
   
   desc = pango_font_description_from_string ("Sans Bold 10");
   
   pango_context_set_font_description (pango_context, desc);
  */

  return TRUE;
}
