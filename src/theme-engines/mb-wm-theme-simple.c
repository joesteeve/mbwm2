#include "mb-wm-theme-simple.h"
#include "mb-wm-theme-xml.h"

#include <X11/Xft/Xft.h>

#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_EDGE_SIZE 3

static void
mb_wm_theme_simple_paint_decor (MBWMTheme *theme, MBWMDecor *decor);
static void
mb_wm_theme_simple_paint_button (MBWMTheme *theme, MBWMDecorButton *button);
static void
mb_wm_theme_simple_get_decor_dimensions (MBWMTheme *, MBWindowManagerClient *,
					 int *, int *, int *, int *);
static void
mb_wm_theme_simple_get_button_size (MBWMTheme *, MBWMDecor *,
				    MBWMDecorButtonType, int *, int *);

static void
mb_wm_theme_simple_get_button_position (MBWMTheme *, MBWMDecor *,
					MBWMDecorButtonType, int*, int*);

static MBWMDecor *
mb_wm_theme_simple_create_decor (MBWMTheme *, MBWindowManagerClient *,
				 MBWMDecorType);


static void
mb_wm_theme_simple_class_init (MBWMObjectClass *klass)
{
  MBWMThemeClass *t_class = MB_WM_THEME_CLASS (klass);

  t_class->paint_decor      = mb_wm_theme_simple_paint_decor;
  t_class->paint_button     = mb_wm_theme_simple_paint_button;
  t_class->decor_dimensions = mb_wm_theme_simple_get_decor_dimensions;
  t_class->button_size      = mb_wm_theme_simple_get_button_size;
  t_class->button_position  = mb_wm_theme_simple_get_button_position;
  t_class->create_decor     = mb_wm_theme_simple_create_decor;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMThemeSimple";
#endif
}

static void
mb_wm_theme_simple_destroy (MBWMObject *obj)
{
}

static int
mb_wm_theme_simple_init (MBWMObject *obj, va_list vap)
{
  MBWMTheme         *theme = MB_WM_THEME (obj);

  /*
   * We do not support shaped windows, so reset the flag to avoid unnecessary
   * ops if the xml theme set this
   */
  theme->shaped = False;

  return 1;
}

int
mb_wm_theme_simple_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMThemeSimpleClass),
	sizeof (MBWMThemeSimple),
	mb_wm_theme_simple_init,
	mb_wm_theme_simple_destroy,
	mb_wm_theme_simple_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_THEME, 0);
    }

  return type;
}

static void
construct_buttons (MBWMThemeSimple * theme,
		   MBWMDecor * decor, MBWMXmlDecor *d)
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
  XftDraw          *xftdraw;
  XftColor          clr;
  XftFont          *font;
};

static void
decordata_free (MBWMDecor * decor, void *data)
{
  struct DecorData * dd = data;
  Display * xdpy = decor->parent_client->wmref->xdpy;

  XFreePixmap (xdpy, dd->xpix);

  XftDrawDestroy (dd->xftdraw);

  if (dd->font)
    XftFontClose (xdpy, dd->font);

  free (dd);
}

static struct DecorData *
decordata_new ()
{
  return mb_wm_util_malloc0 (sizeof (struct DecorData));
}

static MBWMDecor *
mb_wm_theme_simple_create_decor (MBWMTheme             *theme,
				 MBWindowManagerClient *client,
				 MBWMDecorType          type)
{
  MBWMClientType   c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMDecor       *decor = NULL;
  MBWindowManager *wm = client->wmref;
  MBWMXmlClient   *c;

  if (MB_WM_THEME (theme)->xml_clients &&
      (c = mb_wm_xml_client_find_by_type (MB_WM_THEME (theme)->xml_clients,
					  c_type)))
    {
      MBWMXmlDecor *d;

      d = mb_wm_xml_decor_find_by_type (c->decors, type);

      if (d)
	{
	  decor = mb_wm_decor_new (wm, type);
	  mb_wm_decor_attach (decor, client);
	  construct_buttons (MB_WM_THEME_SIMPLE (theme), decor, d);
	}

      return decor;
    }

  switch (c_type)
    {
    case MBWMClientTypeApp:
      switch (type)
	{
	case MBWMDecorTypeNorth:
	  decor = mb_wm_decor_new (wm, type);
	  mb_wm_decor_attach (decor, client);
	  construct_buttons (MB_WM_THEME_SIMPLE (theme), decor, NULL);
	  break;
	default:
	  decor = mb_wm_decor_new (wm, type);
	  mb_wm_decor_attach (decor, client);
	}
      break;

    case MBWMClientTypeDialog:
    case MBWMClientTypePanel:
    case MBWMClientTypeDesktop:
    case MBWMClientTypeInput:
    default:
	  decor = mb_wm_decor_new (wm, type);
	  mb_wm_decor_attach (decor, client);
    }

  return decor;
}

static void
mb_wm_theme_simple_get_button_size (MBWMTheme             *theme,
				    MBWMDecor             *decor,
				    MBWMDecorButtonType    type,
				    int                   *width,
				    int                   *height)
{
  MBWindowManagerClient * client = decor->parent_client;
  MBWMClientType  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
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
	*width = FRAME_TITLEBAR_HEIGHT-4;

      if (height)
	*height = FRAME_TITLEBAR_HEIGHT-4;
    }
}

static void
mb_wm_theme_simple_get_button_position (MBWMTheme             *theme,
					MBWMDecor             *decor,
					MBWMDecorButtonType    type,
					int                   *x,
					int                   *y)
{
  MBWindowManagerClient * client = decor->parent_client;
  MBWMClientType  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
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
mb_wm_theme_simple_get_decor_dimensions (MBWMTheme             *theme,
					 MBWindowManagerClient *client,
					 int                   *north,
					 int                   *south,
					 int                   *west,
					 int                   *east)
{
  MBWMThemeSimple *c_theme = MB_WM_THEME_SIMPLE (theme);
  MBWMClientType c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMXmlClient * c;
  MBWMXmlDecor  * d;

  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)))
    {
      if (north)
	if ((d = mb_wm_xml_decor_find_by_type (c->decors,MBWMDecorTypeNorth)))
	  *north = d->height;
	else
	  *north = FRAME_TITLEBAR_HEIGHT;

      if (south)
	if ((d = mb_wm_xml_decor_find_by_type (c->decors,MBWMDecorTypeSouth)))
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

static unsigned long
pixel_from_clr (Display * dpy, int screen, MBWMColor * clr)
{
  XColor xcol;

  xcol.red   = (int)(clr->r * (double)0xffff);
  xcol.green = (int)(clr->b * (double)0xffff);
  xcol.blue  = (int)(clr->b * (double)0xffff);
  xcol.flags = DoRed|DoGreen|DoBlue;

  XAllocColor (dpy, DefaultColormap (dpy, screen), &xcol);

  return xcol.pixel;
}

static XftFont *
xft_load_font (MBWMDecor * decor, const char * family, int size)
{
  char desc[512];
  XftFont *font;
  Display * xdpy = decor->parent_client->wmref->xdpy;
  int       xscreen = decor->parent_client->wmref->xscreen;

  snprintf (desc, sizeof (desc), "%s-%i",
	    family ? family : "Sans",
	    size ? size : FRAME_TITLEBAR_HEIGHT / 2);

  font = XftFontOpenName (xdpy, xscreen, desc);

  return font;
}

static void
mb_wm_theme_simple_paint_decor (MBWMTheme *theme, MBWMDecor *decor)
{
  MBWMDecorType          type;
  const MBGeometry      *geom;
  MBWindowManagerClient *client;
  Window                 xwin;
  MBWindowManager       *wm = theme->wm;
  MBWMColor             clr_bg;
  MBWMColor             clr_fg;
  MBWMColor             clr_frame;
  MBWMClientType         c_type;
  MBWMXmlClient         *c = NULL;
  MBWMXmlDecor          *d = NULL;
  int                    font_size = 0;
  const char            *font_family = "Sans Serif";
  struct DecorData      *dd;
  int x, y, w, h;
  GC                     gc;
  Display               *xdpy = wm->xdpy;
  int                    xscreen = wm->xscreen;
  const char            *title;

  clr_fg.r = 1.0;
  clr_fg.g = 1.0;
  clr_fg.b = 1.0;

  clr_bg.r = 0.5;
  clr_bg.g = 0.5;
  clr_bg.b = 0.5;

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

  if (!dd)
    {
      XRenderColor rclr;

      dd = malloc (sizeof (struct DecorData));
      dd->xpix = XCreatePixmap(xdpy, xwin,
			       decor->geom.width, decor->geom.height,
			       DefaultDepth(xdpy, xscreen));

      dd->xftdraw = XftDrawCreate (xdpy, dd->xpix,
				   DefaultVisual (xdpy, xscreen),
				   DefaultColormap (xdpy, xscreen));

      rclr.red   = (int)(clr_fg.r * (double)0xffff);
      rclr.green = (int)(clr_fg.g * (double)0xffff);
      rclr.blue  = (int)(clr_fg.b * (double)0xffff);
      rclr.alpha = 0xffff;

      XftColorAllocValue (xdpy, DefaultVisual (xdpy, xscreen),
			  DefaultColormap (xdpy, xscreen),
			  &rclr, &dd->clr);

      dd->font = xft_load_font (decor, font_family, font_size);

      XSetWindowBackgroundPixmap(xdpy, xwin, dd->xpix);

      mb_wm_decor_set_theme_data (decor, dd, decordata_free);
    }

  gc = XCreateGC (xdpy, dd->xpix, 0, NULL);

  XSetLineAttributes (xdpy, gc, 1, LineSolid, CapProjecting, JoinMiter);
  XSetForeground (xdpy, gc, pixel_from_clr (xdpy, xscreen, &clr_frame));
  XSetBackground (xdpy, gc, pixel_from_clr (xdpy, xscreen, &clr_bg));

  w = geom->width; h = geom->height; x = geom->x; y = geom->y;

  XFillRectangle (xdpy, dd->xpix, gc, 0, 0, w, h);

  XSetForeground (xdpy, gc, pixel_from_clr (xdpy, xscreen, &clr_bg));
  XFillRectangle (xdpy, dd->xpix, gc, 0, 0, w, h);

  if (mb_wm_decor_get_type(decor) == MBWMDecorTypeNorth &&
      (title = mb_wm_client_get_name (client)))
    {
      XRectangle rec;

      int pack_start_x = mb_wm_decor_get_pack_start_x (decor);
      int pack_end_x = mb_wm_decor_get_pack_end_x (decor);
      int west_width = mb_wm_client_frame_west_width (client);
      int y = (decor->geom.height -
	       (dd->font->ascent + dd->font->descent)) / 2
	+ dd->font->ascent;

      rec.x = 0;
      rec.y = 0;
      rec.width = pack_end_x - 2;
      rec.height = d ? d->height : FRAME_TITLEBAR_HEIGHT;

      XftDrawSetClipRectangles (dd->xftdraw, 0, 0, &rec, 1);

      XftDrawStringUtf8(dd->xftdraw,
			&dd->clr,
			dd->font,
			west_width + pack_start_x + 2, y,
			title, strlen (title));
    }

  XFreeGC (xdpy, gc);

  XClearWindow (xdpy, xwin);
}

static void
mb_wm_theme_simple_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
  MBWMDecor             *decor;
  MBWindowManagerClient *client;
  Window                 xwin;
  MBWindowManager       *wm = theme->wm;
  int                    x, y, w, h;
  MBWMColor              clr_bg;
  MBWMColor              clr_fg;
  MBWMClientType         c_type;
  MBWMXmlClient         *c = NULL;
  MBWMXmlDecor          *d = NULL;
  MBWMXmlButton         *b = NULL;
  struct DecorData * dd;
  GC                     gc;
  Display               *xdpy = wm->xdpy;
  int                    xscreen = wm->xscreen;

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

  if (client == NULL || xwin == None || dd->xpix == None)
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

  w = button->geom.width;
  h = button->geom.height;
  x = button->geom.x;
  y = button->geom.y;

  gc = XCreateGC (xdpy, dd->xpix, 0, NULL);

  XSetLineAttributes (xdpy, gc, 1, LineSolid, CapRound, JoinRound);



  if (button->state == MBWMDecorButtonStateInactive)
    {
      XSetForeground (xdpy, gc, pixel_from_clr (xdpy, xscreen, &clr_bg));
    }
  else
    {
      /* FIXME -- think of a better way of doing this */
      MBWMColor clr;
      clr.r = clr_bg.r + 0.2;
      clr.g = clr_bg.g + 0.2;
      clr.b = clr_bg.b + 0.2;

      XSetForeground (xdpy, gc, pixel_from_clr (xdpy, xscreen, &clr));
    }

  XFillRectangle (xdpy, dd->xpix, gc, x, y, w+1, h+1);

  XSetLineAttributes (xdpy, gc, 3, LineSolid, CapRound, JoinRound);
  XSetForeground (xdpy, gc, pixel_from_clr (xdpy, xscreen, &clr_fg));

  if (button->type == MBWMDecorButtonClose)
    {
      XDrawLine (xdpy, dd->xpix, gc, x + 3, y + 3, x + w - 3, y + h - 3);
      XDrawLine (xdpy, dd->xpix, gc, x + 3, y + h - 3, x + w - 3, y + 3);
    }
  else if (button->type == MBWMDecorButtonFullscreen)
    {
      XDrawLine (xdpy, dd->xpix, gc, x + 3, y + 3, x + 3, y + h - 3);
      XDrawLine (xdpy, dd->xpix, gc, x + 3, y + h - 3, x + w - 3, y + h - 3);
      XDrawLine (xdpy, dd->xpix, gc, x + w - 3, y + h - 3, x + w - 3, y + 3);
      XDrawLine (xdpy, dd->xpix, gc, x + w - 3, y + 3, x + 3, y + 3);
    }
  else if (button->type == MBWMDecorButtonMinimize)
    {
      XDrawLine (xdpy, dd->xpix, gc, x + 3, y + h - 5, x + w - 3, y + h - 5);
    }
  else if (button->type == MBWMDecorButtonHelp)
    {
      char desc[512];
      XftFont *font;
      XRenderColor rclr;
      XftColor clr;
      XRectangle rec;

      snprintf (desc, sizeof (desc), "%s-%i:bold",
	    d && d->font_family ? d->font_family : "Sans", h*3/4);

      font = XftFontOpenName (xdpy, xscreen, desc);

      rclr.red   = (int)(clr_fg.r * (double)0xffff);
      rclr.green = (int)(clr_fg.g * (double)0xffff);
      rclr.blue  = (int)(clr_fg.b * (double)0xffff);
      rclr.alpha = 0xffff;

      XftColorAllocValue (xdpy, DefaultVisual (xdpy, xscreen),
			  DefaultColormap (xdpy, xscreen),
			  &rclr, &clr);

      rec.x = x;
      rec.y = y;
      rec.width = w;
      rec.height = h;

      XftDrawSetClipRectangles (dd->xftdraw, 0, 0, &rec, 1);

      XftDrawStringUtf8 (dd->xftdraw, &clr, font,
			 x + 4,
			 y + (h - (font->ascent + font->descent))/2 +
			 font->ascent,
			 "?", 1);

      XftFontClose (xdpy, font);
    }
  else if (button->type == MBWMDecorButtonMenu)
    {
      XSetLineAttributes (xdpy, gc, 3, LineSolid, CapRound, JoinMiter);
      XDrawLine (xdpy, dd->xpix, gc, x + 3, y + 5, x + w/2, y + h - 5);
      XDrawLine (xdpy, dd->xpix, gc, x + w/2, y + h - 5, x + w - 3, y + 5);
    }
  else if (button->type == MBWMDecorButtonAccept)
    {
      XDrawArc (xdpy, dd->xpix, gc, x + 4, y + 4, w - 8, h - 8, 0, 64 * 360);
    }

  XFreeGC (xdpy, gc);

  XClearWindow (wm->xdpy, xwin);
}
