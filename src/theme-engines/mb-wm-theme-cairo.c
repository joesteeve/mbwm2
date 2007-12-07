#include "mb-wm-theme-cairo.h"
#include "mb-wm-theme-xml.h"

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

/*
 * Forward declarations
 */
static void
mb_wm_theme_cairo_paint_decor (MBWMTheme *theme, MBWMDecor *decor);
static void
mb_wm_theme_cairo_paint_button (MBWMTheme *theme, MBWMDecorButton *button);
static void
mb_wm_theme_cairo_get_decor_dimensions (MBWMTheme *, MBWindowManagerClient *,
					int *, int *, int *, int *);
static void
mb_wm_theme_cairo_get_button_size (MBWMTheme *, MBWMDecor *,
				   MBWMDecorButtonType, int *, int *);

static void
mb_wm_theme_cairo_get_button_position (MBWMTheme *, MBWMDecor *,
				       MBWMDecorButtonType, int*, int*);

static MBWMDecor *
mb_wm_theme_cairo_create_decor (MBWMTheme *, MBWindowManagerClient *,
				MBWMDecorType);

/*******************************************************************/

static void
mb_wm_theme_cairo_class_init (MBWMObjectClass *klass)
{
  MBWMThemeClass *t_class = MB_WM_THEME_CLASS (klass);

  t_class->paint_decor      = mb_wm_theme_cairo_paint_decor;
  t_class->paint_button     = mb_wm_theme_cairo_paint_button;
  t_class->decor_dimensions = mb_wm_theme_cairo_get_decor_dimensions;
  t_class->button_size      = mb_wm_theme_cairo_get_button_size;
  t_class->button_position  = mb_wm_theme_cairo_get_button_position;
  t_class->create_decor     = mb_wm_theme_cairo_create_decor;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMThemeCairo";
#endif
}

static void
mb_wm_theme_cairo_destroy (MBWMObject *obj)
{
}

static int
mb_wm_theme_cairo_init (MBWMObject *obj, va_list vap)
{
  MBWMTheme         *theme = MB_WM_THEME (obj);

#ifdef USE_GTK
  GtkWidget            *gwin;
  /*
   * Plan here is to just get the GTK settings so we can follow
   * colors set by widgets.
  */

  gtk_init (NULL, NULL);

  gwin = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_widget_ensure_style (gwin);
#endif

  return 1;
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
construct_buttons (MBWMThemeCairo * theme, MBWMDecor * decor, MBWMXmlDecor *d)
{
  MBWindowManagerClient *client = decor->parent_client;
  MBWindowManager       *wm     = client->wmref;
  MBWMDecorButton       *button;

  if (d)
    {
      MBWMList * l = d->buttons;
      while (l)
	{
	  MBWMXmlButton * b = l->data;

	  button = mb_wm_decor_button_stock_new (wm,
						 b->type,
						 b->packing,
						 decor,
						 0);

	  mb_wm_decor_button_show (button);
	  mb_wm_object_unref (MB_WM_OBJECT (button));

	  l = l->next;
	}

      return;
    }

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
#endif
}

struct DecorData
{
  Pixmap            xpix;
  cairo_surface_t  *surface;
};

static void
decordata_free (MBWMDecor * decor, void *data)
{
  struct DecorData * dd = data;
  Display * xdpy = decor->parent_client->wmref->xdpy;

  XFreePixmap (xdpy, dd->xpix);
  cairo_surface_destroy (dd->surface);
  free (dd);
}

static struct DecorData *
decordata_new ()
{
  return mb_wm_util_malloc0 (sizeof (struct DecorData));
}

static MBWMDecor *
mb_wm_theme_cairo_create_decor (MBWMTheme             *theme,
				MBWindowManagerClient *client,
				MBWMDecorType          type)
{
  MBWMClientType   c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMDecor       *decor = NULL;
  MBWindowManager *wm = client->wmref;
  MBWMXmlClient   *c;
  struct DecorData *dd;

  if (MB_WM_THEME (theme)->xml_clients &&
      (c = mb_wm_xml_client_find_by_type (MB_WM_THEME (theme)->xml_clients,
					  c_type)))
    {
      MBWMXmlDecor *d;

      d = mb_wm_xml_decor_find_by_type (c->decors, type);

      if (d)
	{
	  dd = decordata_new ();
	  decor = mb_wm_decor_new (wm, type);
	  mb_wm_decor_attach (decor, client);
	  mb_wm_decor_set_theme_data (decor, dd, decordata_free);
	  construct_buttons (MB_WM_THEME_CAIRO (theme), decor, d);
	}

      return decor;
    }

  switch (c_type)
    {
    case MBWMClientTypeApp:
      switch (type)
	{
	case MBWMDecorTypeNorth:
	  dd = decordata_new ();
	  decor = mb_wm_decor_new (wm, type);
	  mb_wm_decor_attach (decor, client);
	  mb_wm_decor_set_theme_data (decor, dd, decordata_free);
	  construct_buttons (MB_WM_THEME_CAIRO (theme), decor, NULL);
	  break;
	default:
	  dd = decordata_new ();
	  decor = mb_wm_decor_new (wm, type);
	  mb_wm_decor_attach (decor, client);
	  mb_wm_decor_set_theme_data (decor, dd, decordata_free);
	}
      break;

    case MBWMClientTypeDialog:
    case MBWMClientTypePanel:
    case MBWMClientTypeDesktop:
    case MBWMClientTypeInput:
    default:
	  dd = decordata_new ();
	  decor = mb_wm_decor_new (wm, type);
	  mb_wm_decor_attach (decor, client);
	  mb_wm_decor_set_theme_data (decor, dd, decordata_free);
    }

  return decor;
}

static void
mb_wm_theme_cairo_get_button_size (MBWMTheme             *theme,
				   MBWMDecor             *decor,
				   MBWMDecorButtonType    type,
				   int                   *width,
				   int                   *height)
{
  MBWindowManagerClient * client = decor->parent_client;
  MBWMClientType  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMThemeCairo *c_theme = MB_WM_THEME_CAIRO (theme);
  MBWMXmlClient * c;
  MBWMXmlDecor  * d;

  /* FIXME -- assumes button on the north decor only */
  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) &&
      (d = mb_wm_xml_decor_find_by_type (c->decors, decor->type)))
    {
      MBWMXmlButton * b = mb_wm_xml_button_find_by_type (d->buttons, type);

      if (b)
	{
	  if (width)
	    *width = b->width;

	  if (height)
	    *height = b->height;

	  return;
	}
    }

  /*
   * These are defaults when no theme description was loaded
   */
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
mb_wm_theme_cairo_get_button_position (MBWMTheme             *theme,
				       MBWMDecor             *decor,
				       MBWMDecorButtonType    type,
				       int                   *x,
				       int                   *y)
{
  MBWindowManagerClient * client = decor->parent_client;
  MBWMClientType  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMThemeCairo *c_theme = MB_WM_THEME_CAIRO (theme);
  MBWMXmlClient * c;
  MBWMXmlDecor  * d;

  /* FIXME -- assumes button on the north decor only */
  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) &&
      (d = mb_wm_xml_decor_find_by_type (c->decors, decor->type)))
    {
      MBWMXmlButton * b = mb_wm_xml_button_find_by_type (d->buttons, type);

      if (b)
	{
	  if (x)
	    if (b->x >= 0)
	      *x = b->x;
	    else
	      *x = 2;

	  if (y)
	    if (b->y >= 0)
	      *y = b->y;
	    else
	      *y = 2;

	  return;
	}
    }

  if (x)
    *x = 2;

  if (y)
    *y = 2;
}

static void
mb_wm_theme_cairo_get_decor_dimensions (MBWMTheme             *theme,
					MBWindowManagerClient *client,
					int                   *north,
					int                   *south,
					int                   *west,
					int                   *east)
{
  MBWMThemeCairo *c_theme = MB_WM_THEME_CAIRO (theme);
  MBWMClientType c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMXmlClient * c;
  MBWMXmlDecor  * d;

  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)))
    {
      if (north)
	if ((d = mb_wm_xml_decor_find_by_type (c->decors, MBWMDecorTypeNorth)))
	  *north = d->height;
	else
	  *north = FRAME_TITLEBAR_HEIGHT;

      if (south)
	if ((d = mb_wm_xml_decor_find_by_type (c->decors, MBWMDecorTypeSouth)))
	  *south = d->height;
	else
	  *south = FRAME_EDGE_SIZE;

      if (west)
	if ((d = mb_wm_xml_decor_find_by_type (c->decors, MBWMDecorTypeWest)))
	  *west = d->width;
	else
	  *west = FRAME_EDGE_SIZE;

      if (east)
	if ((d = mb_wm_xml_decor_find_by_type (c->decors, MBWMDecorTypeEast)))
	  *east = d->width;
	else
	  *east = FRAME_EDGE_SIZE;

      return;
    }

  /*
   * These are defaults when no theme description was loaded
   */
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
  MBWindowManager       *wm = theme->wm;
  cairo_pattern_t       *pattern;
  cairo_matrix_t         matrix;
  cairo_t               *cr;
  double                 x, y, w, h;
  MBWMColor             clr_bg;
  MBWMColor             clr_fg;
  MBWMColor             clr_frame;
  MBWMColor             clr_bg2;
  MBWMClientType         c_type;
  MBWMThemeCairo        *c_theme = MB_WM_THEME_CAIRO (theme);
  MBWMXmlClient * c;
  MBWMXmlDecor  * d;
  int                    font_size = 0;
  const char            *font_family = "Sans Serif";
  struct DecorData      *dd;

  clr_fg.r = 1.0;
  clr_fg.g = 1.0;
  clr_fg.b = 1.0;

  clr_bg.r = 0.2;
  clr_bg.g = 0.2;
  clr_bg.b = 0.2;

  clr_bg2.r = 0.5;
  clr_bg2.g = 0.5;
  clr_bg2.b = 0.5;

  clr_frame.r = 0.0;
  clr_frame.g = 0.0;
  clr_frame.b = 0.0;

  client = mb_wm_decor_get_parent (decor);
  xwin = mb_wm_decor_get_x_window (decor);

  if (client == NULL || xwin == None)
    return;

  dd = mb_wm_decor_get_theme_data (decor);

  type   = mb_wm_decor_get_type (decor);
  geom   = mb_wm_decor_get_geometry (decor);
  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) &&
      (d = mb_wm_xml_decor_find_by_type (c->decors, decor->type)))
    {
      if (d->clr_fg.set)
	{
	  clr_fg.r = d->clr_fg.r;
	  clr_fg.g = d->clr_fg.g;
	  clr_fg.b = d->clr_fg.b;
	}

      if (d->clr_bg.set)
	{
	  clr_bg.r = d->clr_bg.r;
	  clr_bg.g = d->clr_bg.g;
	  clr_bg.b = d->clr_bg.b;
	}

      if (d->clr_bg2.set)
	{
	  clr_bg2.r = d->clr_bg2.r;
	  clr_bg2.g = d->clr_bg2.g;
	  clr_bg2.b = d->clr_bg2.b;
	}

      if (d->clr_frame.set)
	{
	  clr_frame.r = d->clr_frame.r;
	  clr_frame.g = d->clr_frame.g;
	  clr_frame.b = d->clr_frame.b;
	}

      if (d->font_size)
	font_size = d->font_size;

      if (d->font_family)
	font_family = d->font_family;
    }

  if (dd->xpix == None)
    dd->xpix = XCreatePixmap (wm->xdpy, xwin, geom->width, geom->height,
			      DefaultDepth(wm->xdpy, wm->xscreen));

  if (!dd->surface)
    dd->surface = cairo_xlib_surface_create  (wm->xdpy,
					  dd->xpix,
					  DefaultVisual(wm->xdpy, wm->xscreen),
					  geom->width,
					  geom->height);

  cr = cairo_create (dd->surface);

  cairo_set_line_width (cr, 1.0);


  w = geom->width; h = geom->height; x = geom->x; y = geom->y;

  cairo_set_line_width (cr, 0.04);

  cairo_set_source_rgb (cr, clr_frame.r, clr_frame.g, clr_frame.b);

  cairo_rectangle( cr, 0, 0, w, h);

  cairo_fill (cr);

  cairo_set_source_rgb (cr, clr_bg.r, clr_bg.g, clr_bg.b);

  cairo_rectangle( cr, 1, 1, w-2, h-2);


  cairo_fill (cr);

  if (mb_wm_decor_get_type(decor) == MBWMDecorTypeNorth)
    {
      cairo_font_extents_t font_extents;
      int pack_start_x = mb_wm_decor_get_pack_start_x (decor);

      pattern = cairo_pattern_create_linear (0, 0, 0, h);

      cairo_pattern_add_color_stop_rgb (pattern, 0.0,
					clr_bg2.r, clr_bg2.g, clr_bg2.b);
      cairo_pattern_add_color_stop_rgb (pattern, 0.45,
					clr_bg.r, clr_bg.g, clr_bg.b);
      cairo_pattern_add_color_stop_rgb (pattern, 0.55,
					clr_bg.r, clr_bg.g, clr_bg.b);
      cairo_pattern_add_color_stop_rgb (pattern, 1.0,
					clr_bg2.r, clr_bg2.g, clr_bg2.b);

      cairo_set_source (cr, pattern);

      cairo_rectangle( cr, 2, 2, w-4, h-3);

      cairo_fill (cr);

      cairo_set_source_rgb (cr, clr_fg.r, clr_fg.g, clr_fg.b);

      cairo_select_font_face (cr,
			      font_family,
			      CAIRO_FONT_SLANT_NORMAL,
			      CAIRO_FONT_WEIGHT_NORMAL);

      if (font_size)
	cairo_set_font_size (cr, font_size);
      else
	cairo_set_font_size (cr, h - (h/6));

      cairo_font_extents (cr, &font_extents);

      cairo_move_to (cr,
		     mb_wm_client_frame_west_width (client) + pack_start_x,
		     (h - (font_extents.ascent + font_extents.descent)) / 2
		     + font_extents.ascent);

      cairo_show_text (cr, mb_wm_client_get_name(client));

      cairo_pattern_destroy (pattern);
    }

  cairo_set_source_rgb (cr, clr_bg.r, clr_bg.g, clr_bg.b);

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

  XSetWindowBackgroundPixmap (wm->xdpy, xwin, dd->xpix);
  XClearWindow (wm->xdpy, xwin);
}

static void
mb_wm_theme_cairo_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
  MBWMDecor             *decor;
  MBWindowManagerClient *client;
  Window                 xwin;
  MBWindowManager       *wm = theme->wm;
  cairo_t               *cr;
  int                    xi, yi, wi, hi;
  double                 x, y, w, h;
  cairo_font_extents_t   font_extents;
  MBWMColor              clr_bg;
  MBWMColor              clr_fg;
  MBWMClientType         c_type;
  MBWMThemeCairo        *c_theme = MB_WM_THEME_CAIRO (theme);
  MBWMXmlClient         *c = NULL;
  MBWMXmlDecor          *d = NULL;
  MBWMXmlButton         *b = NULL;
  struct DecorData * dd;
  clr_fg.r = 1.0;
  clr_fg.g = 1.0;
  clr_fg.b = 1.0;

  clr_bg.r = 0.0;
  clr_bg.g = 0.0;
  clr_bg.b = 0.0;

  decor = button->decor;
  client = mb_wm_decor_get_parent (decor);
  xwin = decor->xwin;
  dd = mb_wm_decor_get_theme_data (decor);

  if (client == NULL || xwin == None || dd->xpix == None || !dd->surface)
    return;

  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) &&
      (d = mb_wm_xml_decor_find_by_type (c->decors, decor->type)) &&
      (b = mb_wm_xml_button_find_by_type (d->buttons, button->type)))
    {
      clr_fg.r = b->clr_fg.r;
      clr_fg.g = b->clr_fg.g;
      clr_fg.b = b->clr_fg.b;

      clr_bg.r = b->clr_bg.r;
      clr_bg.g = b->clr_bg.g;
      clr_bg.b = b->clr_bg.b;
    }

  wi = button->geom.width;
  hi = button->geom.height;
  xi = button->geom.x;
  yi = button->geom.y;

  w = wi;
  h = hi;
  x = xi;
  y = yi;

  cr = cairo_create (dd->surface);

  cairo_set_line_width (cr, 0.04);
  cairo_rectangle( cr, x, y, w, h);

  if (button->state == MBWMDecorButtonStateInactive)
    {
      cairo_set_source_rgb (cr, clr_bg.r, clr_bg.g, clr_bg.b);
    }
  else
    {
      if (d && d->clr_bg.set)
	{
	  cairo_set_source_rgba (cr, d->clr_bg.r, d->clr_bg.g, d->clr_bg.b,
				 0.25);
	}
      else
	{
	  cairo_set_source_rgba (cr, 0.7, 0.7, 0.7, 0.25);
	}

      cairo_fill (cr);
      cairo_set_source_rgba (cr, clr_bg.r, clr_bg.g, clr_bg.b, 0.5);
    }

  cairo_fill (cr);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, 3.0);
  cairo_set_source_rgb (cr, clr_fg.r, clr_fg.g, clr_fg.b);

  if (button->type == MBWMDecorButtonClose)
    {
      cairo_move_to (cr, x + 3.0, y + 3.0);
      cairo_line_to (cr, x + w-3.0, y + h-3.0);
      cairo_stroke (cr);

      cairo_move_to (cr, x + 3.0, y + h-3.0);
      cairo_line_to (cr, x + w-3.0, y + 3.0);
      cairo_stroke (cr);
    }
  else if (button->type == MBWMDecorButtonFullscreen)
    {
      cairo_move_to (cr, x + 3.0, y + 3.0);
      cairo_line_to (cr, x + 3.0, y + h-3.0);
      cairo_line_to (cr, x + w-3.0, y + h-3.0);
      cairo_line_to (cr, x + w-3.0, y + 3.0);
      cairo_line_to (cr, x + 3.0, y + 3.0);
      cairo_stroke (cr);
    }
  else if (button->type == MBWMDecorButtonMinimize)
    {
      cairo_move_to (cr, x + 3.0, y + h-5.0);
      cairo_line_to (cr, x + w-3.0, y + h-5.0);
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
		     x + 4.0,
		     y + (h - (font_extents.ascent + font_extents.descent)) / 2
		     + font_extents.ascent);

      cairo_show_text (cr, "?");
    }
  else if (button->type == MBWMDecorButtonMenu)
    {
      cairo_move_to (cr, x + 3.0, y + 5.0);
      cairo_line_to (cr, x + w/2.0, y +h-7.0);
      cairo_stroke (cr);

      cairo_move_to (cr, x + w/2.0, y + h-7.0);
      cairo_line_to (cr, x + w-3.0, y + 5.0);
      cairo_stroke (cr);
    }
  else if (button->type == MBWMDecorButtonAccept)
    {
      cairo_arc (cr, x + w/2.0, y + h/2.0, (w-8.0)/2.0, 0.0, 2.0 * M_PI);
      cairo_stroke (cr);
    }

  cairo_destroy (cr);

  XClearWindow (wm->xdpy, xwin);
}
