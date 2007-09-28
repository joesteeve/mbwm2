#include "mb-wm-theme-cairo.h"

#include <math.h>

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

#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_EDGE_SIZE 3

static void
mb_wm_theme_cairo_paint_decor (MBWMTheme *theme, MBWMDecor *decor);
static void
mb_wm_theme_cairo_paint_button (MBWMTheme *theme, MBWMDecorButton *button);
static void
mb_wm_theme_cairo_get_decor_dimensions (MBWMTheme *, MBWindowManagerClient *,
					int *, int *, int *, int *);
static void
mb_wm_theme_cairo_get_button_size (MBWMTheme *, MBWindowManagerClient *,
				   int *, int *);
static MBWMDecor *
mb_wm_theme_cairo_create_decor (MBWMTheme *, MBWindowManagerClient *,
				MBWMDecorType);

static void
mb_wm_theme_cairo_class_init (MBWMObjectClass *klass)
{
  MBWMThemeClass *t_class = MB_WM_THEME_CLASS (klass);

  t_class->paint_decor      = mb_wm_theme_cairo_paint_decor;
  t_class->paint_button     = mb_wm_theme_cairo_paint_button;
  t_class->decor_dimensions = mb_wm_theme_cairo_get_decor_dimensions;
  t_class->button_size      = mb_wm_theme_cairo_get_button_size;
  t_class->create_decor     = mb_wm_theme_cairo_create_decor;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMThemeCairo";
#endif
}

static void
mb_wm_theme_cairo_destroy (MBWMObject *obj)
{
}

static void
mb_wm_theme_cairo_init (MBWMObject *obj, va_list vap)
{
  MBWMThemeCairo * theme = MB_WM_THEME_CAIRO (obj);

  PangoFontDescription *desc;
#ifdef USE_GTK
  GtkWidget            *gwin;
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

  /* TODO -- init caps */
}

int
mb_wm_theme_cairo_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMThemeCairoClass),
	sizeof (MBWMThemeCairo),
	mb_wm_theme_cairo_init,
	mb_wm_theme_cairo_destroy,
	mb_wm_theme_cairo_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_THEME, 0);
    }

  return type;
}

static void
decor_resize (MBWindowManager   *wm,
	      MBWMDecor         *decor,
	      void              *userdata)
{
  const MBGeometry *geom;
  MBWMList         *l;
  int               btn_x_start, btn_x_end;

  geom = mb_wm_decor_get_geometry (decor);

  btn_x_end = geom->width - 2;
  btn_x_start = 2;

  l = decor->buttons;
  while (l)
    {
      MBWMDecorButton  *btn = (MBWMDecorButton  *)l->data;

      if (btn->pack == MBWMDecorButtonPackEnd)
	{
	  btn_x_end -= (btn->geom.width + 2);
	  mb_wm_decor_button_move_to (btn, btn_x_end, 2);
	}
      else
	{
	  mb_wm_decor_button_move_to (btn, btn_x_start, 2);
	  btn_x_start += (btn->geom.width + 2);
	}

      l = l->next;
    }

  decor->pack_start_x = btn_x_start;
  decor->pack_end_x   = btn_x_end;
}

static void
decor_repaint (MBWindowManager   *wm,
	       MBWMDecor         *decor,
	       void              *userdata)
{
  mb_wm_theme_paint_decor (wm->theme, decor);
}

static void
construct_buttons (MBWMDecor * decor, MBWindowManagerClient * client)
{
  MBWindowManager       *wm     = client->wmref;
  MBWMDecorButton       *button;

  button = mb_wm_decor_button_stock_new (wm,
					 MBWMDecorButtonClose,
					 MBWMDecorButtonPackEnd,
					 decor,
					 0);

  mb_wm_decor_button_show (button);
  mb_wm_object_unref (MB_WM_OBJECT (button));

#if 0
  /*
   * We probably do not want this in the default client, but for now
   * it is useful for testing purposes
   */
  button = mb_wm_decor_button_stock_new (wm,
					 MBWMDecorButtonFullscreen,
					 MBWMDecorButtonPackEnd,
					 decor,
					 0);

  mb_wm_decor_button_show (button);
  mb_wm_object_unref (MB_WM_OBJECT (button));

  button = mb_wm_decor_button_stock_new (wm,
					 MBWMDecorButtonHelp,
					 MBWMDecorButtonPackEnd,
					 decor,
					 0);

  mb_wm_decor_button_show (button);
  mb_wm_object_unref (MB_WM_OBJECT (button));

  button = mb_wm_decor_button_stock_new (wm,
					 MBWMDecorButtonAccept,
					 MBWMDecorButtonPackEnd,
					 decor,
					 0);

  mb_wm_decor_button_show (button);
  mb_wm_object_unref (MB_WM_OBJECT (button));
#endif

  button = mb_wm_decor_button_stock_new (wm,
					 MBWMDecorButtonMinimize,
					 MBWMDecorButtonPackEnd,
					 decor,
					 0);

  mb_wm_decor_button_show (button);
  mb_wm_object_unref (MB_WM_OBJECT (button));

  button = mb_wm_decor_button_stock_new (wm,
					 MBWMDecorButtonMenu,
					 MBWMDecorButtonPackStart,
					 decor,
					 0);

  mb_wm_decor_button_show (button);
  mb_wm_object_unref (MB_WM_OBJECT (button));
}

static MBWMDecor *
mb_wm_theme_cairo_create_decor (MBWMTheme             *theme,
				MBWindowManagerClient *client,
				MBWMDecorType          type)
{
  MBWMClientType   c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMDecor       *decor = NULL;
  MBWindowManager *wm = client->wmref;

  switch (c_type)
    {
    case MBWMClientTypeApp:
      switch (type)
	{
	case MBWMDecorTypeNorth:
	  decor = mb_wm_decor_new (wm, type, decor_resize, decor_repaint,
				   client);
	  mb_wm_decor_attach (decor, client);
	  construct_buttons (decor, client);
	  break;
	default:
	  decor = mb_wm_decor_new (wm, type,
				   decor_resize, decor_repaint, client);
	  mb_wm_decor_attach (decor, client);
	}
      break;

    case MBWMClientTypeDialog:
    case MBWMClientTypePanel:
    case MBWMClientTypeDesktop:
    case MBWMClientTypeInput:
    default:
	  decor = mb_wm_decor_new (wm, type,
				   decor_resize, decor_repaint, client);
	  mb_wm_decor_attach (decor, client);
    }

  return decor;
}

static void
mb_wm_theme_cairo_get_button_size (MBWMTheme             *theme,
				   MBWindowManagerClient *client,
				   int                   *width,
				   int                   *height)
{
  MBWMClientType c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  switch (c_type)
    {
    case MBWMClientTypeApp:
    case MBWMClientTypeDialog:
    case MBWMClientTypePanel:
    case MBWMClientTypeDesktop:
    case MBWMClientTypeInput:
    default:
      if (width)
	*width = FRAME_TITLEBAR_HEIGHT-2;

      if (height)
	*height = FRAME_TITLEBAR_HEIGHT-2;
    }
}

static void
mb_wm_theme_cairo_get_decor_dimensions (MBWMTheme             *theme,
					MBWindowManagerClient *client,
					int                   *north,
					int                   *south,
					int                   *west,
					int                   *east)
{
  MBWMClientType c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  switch (c_type)
    {
    case MBWMClientTypeApp:
      if (north)
	*north = FRAME_TITLEBAR_HEIGHT;

      if (south)
	*south = FRAME_EDGE_SIZE;

      if (west)
	*west = FRAME_EDGE_SIZE;

      if (east)
	*east = FRAME_EDGE_SIZE;
      break;

    case MBWMClientTypeDialog:
    case MBWMClientTypePanel:
    case MBWMClientTypeDesktop:
    case MBWMClientTypeInput:
    default:
      if (north)
	*north = 0;

      if (south)
	*south = 0;

      if (west)
	*west = 0;

      if (east)
	*east = 0;
    }
}

static void
mb_wm_theme_cairo_paint_decor (MBWMTheme *theme,
			       MBWMDecor *decor)
{
  MBWMDecorType          type;
  const MBGeometry      *geom;
  MBWindowManagerClient *client;
  Window                 xwin;
  Pixmap                 xpxmp;
  MBWindowManager       *wm = theme->wm;
  cairo_pattern_t       *pattern;
  cairo_matrix_t         matrix;
  cairo_surface_t       *surface;
  cairo_t               *cr;
  double                 x, y, w, h;

  client = mb_wm_decor_get_parent (decor);
  xwin = mb_wm_decor_get_x_window (decor);

  if (client == NULL || xwin == None)
    return;

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


  w = geom->width; h = geom->height; x = geom->x; y = geom->y;

  cairo_set_line_width (cr, 0.04);

  cairo_set_source_rgb (cr, 0, 0, 0);

  cairo_rectangle( cr, 0, 0, w, h);

  cairo_fill (cr);

  cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);

  cairo_rectangle( cr, 1, 1, w-2, h-2);


  cairo_fill (cr);

  if (mb_wm_decor_get_type(decor) == MBWMDecorTypeNorth)
    {
      cairo_font_extents_t font_extents;
      int pack_start_x = mb_wm_decor_get_pack_start_x (decor);

      pattern = cairo_pattern_create_linear (0, 0, 0, h);

      cairo_pattern_add_color_stop_rgb (pattern, 0,   0.5, 0.5, 0.5 );
      cairo_pattern_add_color_stop_rgb (pattern, 0.5, 0.25, 0.25, 0.25 );
      cairo_pattern_add_color_stop_rgb (pattern, 0.51, 0.25, 0.25, 0.25 );
      cairo_pattern_add_color_stop_rgb (pattern, 1,   0.4, 0.4, 0.4 );

      cairo_set_source (cr, pattern);

      cairo_rectangle( cr, 2, 2, w-4, h-3);

      cairo_fill (cr);

      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

      cairo_select_font_face (cr,
			      "Sans Serif",
			      CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);

      cairo_set_font_size (cr, h - (h/6));

      cairo_font_extents (cr, &font_extents);

      cairo_move_to (cr,
		     mb_wm_client_frame_west_width (client) + pack_start_x,
		     (h - (font_extents.ascent + font_extents.descent)) / 2
		     + font_extents.ascent);

      cairo_show_text (cr, mb_wm_client_get_name(client));

      cairo_pattern_destroy (pattern);
    }

  cairo_set_source_rgb (cr, 0.7, 0.7, 0.7);

  /* tweak borders a little so they all 'fit' */

  switch (type)
    {
    case MBWMDecorTypeNorth:
      cairo_rectangle( cr, 1, h-1, 1, 1);
      cairo_rectangle( cr, w-2, h-1, 1, 1);
      break;
    case MBWMDecorTypeSouth:
      cairo_rectangle( cr, 1, 0, 1, 1);
      cairo_rectangle( cr, w-2, 0, 1, 1);
      break;
    case MBWMDecorTypeWest:
    case MBWMDecorTypeEast:
      cairo_rectangle( cr, 1, 0, 1, 1);
      cairo_rectangle( cr, 1, h-1, 1, 1);
      break;
    default:
      break;
    }

  cairo_fill (cr);

  cairo_destroy (cr);

  XSetWindowBackgroundPixmap (wm->xdpy, xwin, xpxmp);
  XClearWindow (wm->xdpy, xwin);

  XFreePixmap (wm->xdpy, xpxmp);
}

static void
mb_wm_theme_cairo_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
  MBWMDecor             *decor;
  MBWindowManagerClient *client;
  Window                 xwin;
  Pixmap                 xpxmp;
  MBWindowManager       *wm = theme->wm;
  cairo_surface_t       *surface;
  cairo_t               *cr;
  int                    xi, yi, wi, hi;
  double                 x, y, w, h;
  cairo_font_extents_t   font_extents;

  decor = button->decor;
  client = mb_wm_decor_get_parent (decor);
  xwin = button->xwin;

  if (client == NULL || xwin == None)
    return;

  wi = button->geom.width;
  hi = button->geom.height;
  xi = button->geom.x;
  yi = button->geom.y;

  w = wi;
  h = hi;
  x = xi;
  y = yi;

  xpxmp = XCreatePixmap (wm->xdpy, xwin, wi, hi,
			 DefaultDepth(wm->xdpy, wm->xscreen));

  surface = cairo_xlib_surface_create (wm->xdpy,
				       xpxmp,
				       DefaultVisual(wm->xdpy, wm->xscreen),
				       wi, hi);
  cr = cairo_create(surface);

  cairo_set_line_width (cr, 0.04);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_rectangle( cr, 0.0, 0.0, w, h);
  cairo_fill (cr);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, 3.0);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

  if (button->type == MBWMDecorButtonClose)
    {
      cairo_move_to (cr, 3.0, 3.0);
      cairo_line_to (cr, w-3.0, h-3.0);
      cairo_stroke (cr);

      cairo_move_to (cr, 3.0, h-3.0);
      cairo_line_to (cr, w-3.0, 3.0);
      cairo_stroke (cr);
    }
  else if (button->type == MBWMDecorButtonFullscreen)
    {
      cairo_move_to (cr, 3.0, 3.0);
      cairo_line_to (cr, 3.0, h-3.0);
      cairo_line_to (cr, w-3.0, h-3.0);
      cairo_line_to (cr, w-3.0, 3.0);
      cairo_line_to (cr, 3.0, 3.0);
      cairo_stroke (cr);
    }
  else if (button->type == MBWMDecorButtonMinimize)
    {
      cairo_move_to (cr, 3.0, h-5.0);
      cairo_line_to (cr, w-3.0, h-5.0);
      cairo_stroke (cr);
    }
  else if (button->type == MBWMDecorButtonHelp)
    {
      cairo_select_font_face (cr,
			      "Sans Serif",
			      CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_BOLD);

      cairo_set_font_size (cr, h);
      cairo_font_extents (cr, &font_extents);


      cairo_move_to (cr,
		     4.0,
		     (h - (font_extents.ascent + font_extents.descent)) / 2
		     + font_extents.ascent);

      cairo_show_text (cr, "?");
    }
  else if (button->type == MBWMDecorButtonMenu)
    {
      cairo_move_to (cr, 3.0, 5.0);
      cairo_line_to (cr, w/2.0, h-7.0);
      cairo_stroke (cr);

      cairo_move_to (cr, w/2.0, h-7.0);
      cairo_line_to (cr, w-3.0, 5.0);
      cairo_stroke (cr);
    }
  else if (button->type == MBWMDecorButtonAccept)
    {
      cairo_arc (cr, w/2.0, h/2.0, (w-8.0)/2.0, 0.0, 2.0 * M_PI);
      cairo_stroke (cr);
    }

  cairo_destroy (cr);

  XSetWindowBackgroundPixmap (wm->xdpy, xwin, xpxmp);
  XClearWindow (wm->xdpy, xwin);
  XFreePixmap (wm->xdpy, xpxmp);
}
