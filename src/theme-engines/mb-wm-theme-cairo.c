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

#include <expat.h>

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
static MBWMDecor *
mb_wm_theme_cairo_create_decor (MBWMTheme *, MBWindowManagerClient *,
				MBWMDecorType);

/*
 * Helper structs for xml theme
 */
struct Clr
{
  double r;
  double g;
  double b;

  Bool set;
};

struct Button
{
  MBWMDecorButtonType type;
  MBWMDecorButtonPack packing;

  struct Clr clr_fg;
  struct Clr clr_bg;

  int width;
  int height;
};

static struct Button *
button_new ()
{
  struct Button * b = mb_wm_util_malloc0 (sizeof (struct Button));
  return b;
}

static void
button_free (struct Button * b)
{
  free (b);
}

struct Decor
{
  MBWMDecorType type;

  struct Clr clr_fg;
  struct Clr clr_bg;
  struct Clr clr_bg2;
  struct Clr clr_frame;

  int size;
  int font_size;

  char * font_family;

  MBWMList * buttons;
};

static struct Decor *
decor_new ()
{
  struct Decor * d = mb_wm_util_malloc0 (sizeof (struct Decor));
  return d;
}

static void
decor_free (struct Decor * d)
{
  MBWMList * l;

  if (!d)
    return;

  l = d->buttons;
  while (l)
    {
      struct Button * b = l->data;
      MBWMList * n = l->next;
      button_free (b);
      free (l);

      l = n;
    }

  if (d->font_family)
    free (d->font_family);

  free (d);
}

struct Client
{
  MBWMClientType  type;
  MBWMList       *decors;
};

static struct Client *
client_new ()
{
  struct Client * c = mb_wm_util_malloc0 (sizeof (struct Client));
  return c;
}

static void
client_free (struct Client * c)
{
  MBWMList * l;

  if (!c)
    return;

  l = c->decors;
  while (l)
    {
      struct Decor * d = l->data;
      MBWMList * n = l->next;
      decor_free (d);
      free (l);

      l = n;
    }

  free (c);
}

static void
clr_from_string (struct Clr * clr, const char *s)
{
  int  r, g, b;

  if (!s || *s != '#')
    return;

  sscanf (s+1,"%2x%2x%2x", &r, &g, &b);
  clr->r = (double) r / 255.0;
  clr->g = (double) g / 255.0;
  clr->b = (double) b / 255.0;

  clr->set = True;
}

/*
 * expat callbacks
 */
static void
xml_element_start_cb(void *data, const char *tag, const char **expat_attr)
{
  MBWMThemeCairo *theme = data;

  if (!expat_attr)
    return;

  if (!strcmp (tag, "client"))
    {
      struct Client * c = client_new ();
      const char **p = expat_attr;

      while (*p)
	{
	  if (!strcmp (*p, "type"))
	    {
	      if (!strcmp (*(p+1), "app"))
		c->type = MBWMClientTypeApp;
	      else if (!strcmp (*(p+1), "dialog"))
		c->type = MBWMClientTypeDialog;
	      else if (!strcmp (*(p+1), "panel"))
		c->type = MBWMClientTypePanel;
	      else if (!strcmp (*(p+1), "input"))
		c->type = MBWMClientTypeInput;
	      else if (!strcmp (*(p+1), "desktop"))
		c->type = MBWMClientTypeDesktop;
	    }

	  p += 2;
	}

      if (!c->type)
	client_free (c);
      else
	theme->xml_clients = mb_wm_util_list_prepend (theme->xml_clients, c);

      return;
    }

  if (!strcmp (tag, "decor"))
    {
      struct Decor * d = decor_new ();

      const char **p = expat_attr;

      while (*p)
	{
	  if (!strcmp (*p, "clr-fg"))
	    clr_from_string (&d->clr_fg, *(p+1));
	  else if (!strcmp (*p, "clr-bg"))
	    clr_from_string (&d->clr_bg, *(p+1));
	  else if (!strcmp (*p, "clr-bg2"))
	    clr_from_string (&d->clr_bg2, *(p+1));
	  else if (!strcmp (*p, "clr-frame"))
	    clr_from_string (&d->clr_frame, *(p+1));
	  else if (!strcmp (*p, "type"))
	    {
	      if (!strcmp (*(p+1), "north"))
		d->type = MBWMDecorTypeNorth;
	      else if (!strcmp (*(p+1), "south"))
		d->type = MBWMDecorTypeSouth;
	      else if (!strcmp (*(p+1), "east"))
		d->type = MBWMDecorTypeEast;
	      else if (!strcmp (*(p+1), "west"))
		d->type = MBWMDecorTypeWest;
	    }
	  else if (!strcmp (*p, "size"))
	    {
	      d->size = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "font-size"))
	    {
	      d->font_size = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "font-family"))
	    {
	      d->font_family = strdup (*(p+1));
	    }

	  p += 2;
	}

      if (!d->type || !theme->xml_clients)
	  decor_free (d);
      else
	{
	  struct Client * c = theme->xml_clients->data;
	  c->decors = mb_wm_util_list_prepend (c->decors, d);
	}

      return;
    }

  if (!strcmp (tag, "button"))
    {
      struct Button * b = button_new ();

      const char **p = expat_attr;

      while (*p)
	{
	  if (!strcmp (*p, "clr-fg"))
	    clr_from_string (&b->clr_fg, *(p+1));
	  else if (!strcmp (*p, "clr-bg"))
	    clr_from_string (&b->clr_bg, *(p+1));
	  else if (!strcmp (*p, "type"))
	    {
	      if (!strcmp (*(p+1), "minimize"))
		b->type = MBWMDecorButtonMinimize;
	      else if (!strcmp (*(p+1), "close"))
		b->type = MBWMDecorButtonClose;
	      else if (!strcmp (*(p+1), "menu"))
		b->type = MBWMDecorButtonMenu;
	      else if (!strcmp (*(p+1), "accept"))
		b->type = MBWMDecorButtonAccept;
	      else if (!strcmp (*(p+1), "fullscreen"))
		b->type = MBWMDecorButtonFullscreen;
	      else if (!strcmp (*(p+1), "help"))
		b->type = MBWMDecorButtonHelp;
	    }
	  else if (!strcmp (*p, "packing"))
	    {
	      if (!strcmp (*(p+1), "end"))
		b->packing = MBWMDecorButtonPackEnd;
	      else if (!strcmp (*(p+1), "start"))
		b->packing = MBWMDecorButtonPackStart;
	    }
	  else if (!strcmp (*p, "width"))
	    {
	      b->width = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "height"))
	    {
	      b->height = atoi (*(p+1));
	    }

	  p += 2;
	}

      if (!b->type ||
	  !theme->xml_clients ||
	  !theme->xml_clients->data)
	{
	  button_free (b);
	  return;
	}

      struct Client * c = theme->xml_clients->data;

      if (!c->decors || !c->decors->data)
	{
	  button_free (b);
	  return;
	}

      struct Decor  * d = c->decors->data;
      d->buttons = mb_wm_util_list_append (d->buttons, b);

      return;
    }

}

static struct Client *
client_find_by_type (MBWMList *l, MBWMClientType type)
{
  while (l)
    {
      struct Client * c = l->data;
      if (c->type == type)
	return c;

      l = l->next;
    }

  return NULL;
}

static struct Decor *
decor_find_by_type (MBWMList *l, MBWMDecorType type)
{
  while (l)
    {
      struct Decor * d = l->data;
      if (d->type == type)
	return d;

      l = l->next;
    }

  return NULL;
}

static struct Button *
button_find_by_type (MBWMList *l, MBWMDecorButtonType type)
{
  while (l)
    {
      struct Button * b = l->data;
      if (b->type == type)
	return b;

      l = l->next;
    }

  return NULL;
}

#if 0
static void
client_dump (MBWMList * l)
{
  printf ("=== XML Clients =====\n");
  while (l)
    {
      struct Client * c = l->data;
      MBWMList *l2 = c->decors;
      printf ("===== client type %d =====\n", c->type);

      while (l2)
	{
	  struct Decor * d = l2->data;
	  MBWMList *l3 = d->buttons;
	  printf ("======= decor type %d =====\n", d->type);

	  while (l3)
	    {
	      struct Button * b = l3->data;
	      printf ("========= button type %d =====\n", d->type);

	      l3 = l3->next;
	    }

	  l2 = l2->next;
	}

      l = l->next;
    }
  printf ("=== XML Clients End =====\n");
}
#endif

/*******************************************************************/

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
  MBWMThemeCairo * theme = MB_WM_THEME_CAIRO (obj);
  MBWMList *l = theme->xml_clients;

  while (l)
    {
      struct Client * c = l->data;
      MBWMList * n = l->next;
      client_free (c);
      free (l);

      l = n;
    }

}

static void
mb_wm_theme_cairo_init (MBWMObject *obj, va_list vap)
{
  MBWMTheme         *theme = MB_WM_THEME (obj);
  char               buf[256];
  FILE              *file = NULL;
  XML_Parser         par = NULL;

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

  if (!theme->path)
    return;

  if (!(file = fopen (theme->path, "r")) || !(par = XML_ParserCreate(NULL)))
    goto done;

  XML_SetElementHandler (par, xml_element_start_cb, NULL);
  XML_SetUserData(par, (void *) theme);

  while (fgets (buf, sizeof (buf), file) &&
	 XML_Parse(par, buf, strlen(buf), 0));

  XML_Parse(par, NULL, 0, 1);

 done:
  if (file)
    fclose (file);

  if (par)
    XML_ParserFree (par);
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
construct_buttons (MBWMThemeCairo * theme, MBWMDecor * decor)
{
  MBWindowManagerClient *client = decor->parent_client;
  MBWindowManager       *wm     = client->wmref;
  MBWMDecorButton       *button;
  MBWMClientType         c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  struct Client         *c;
  struct Decor          *d;

  if ((c = client_find_by_type (theme->xml_clients, c_type)) &&
      (d = decor_find_by_type (c->decors, decor->type)))
    {
      MBWMList * l = d->buttons;
      while (l)
	{
	  struct Button * b = l->data;

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
	  construct_buttons (MB_WM_THEME_CAIRO (theme), decor);
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
				   MBWMDecor             *decor,
				   MBWMDecorButtonType    type,
				   int                   *width,
				   int                   *height)
{
  MBWindowManagerClient * client = decor->parent_client;
  MBWMClientType  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMThemeCairo *c_theme = MB_WM_THEME_CAIRO (theme);
  struct Client * c;
  struct Decor  * d;

  /* FIXME -- assumes button on the north decor only */
  if ((c = client_find_by_type (c_theme->xml_clients, c_type)) &&
      (d = decor_find_by_type (c->decors, decor->type)))
    {
      struct Button * b = button_find_by_type (d->buttons, type);

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
mb_wm_theme_cairo_get_decor_dimensions (MBWMTheme             *theme,
					MBWindowManagerClient *client,
					int                   *north,
					int                   *south,
					int                   *west,
					int                   *east)
{
  MBWMThemeCairo *c_theme = MB_WM_THEME_CAIRO (theme);
  MBWMClientType c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  struct Client * c;
  struct Decor  * d;

  if ((c = client_find_by_type (c_theme->xml_clients, c_type)))
    {
      if (north)
	if ((d = decor_find_by_type (c->decors, MBWMDecorTypeNorth)))
	  *north = d->size;
	else
	  *north = FRAME_TITLEBAR_HEIGHT;

      if (south)
	if ((d = decor_find_by_type (c->decors, MBWMDecorTypeSouth)))
	  *south = d->size;
	else
	  *south = FRAME_EDGE_SIZE;

      if (west)
	if ((d = decor_find_by_type (c->decors, MBWMDecorTypeWest)))
	  *west = d->size;
	else
	  *west = FRAME_EDGE_SIZE;

      if (east)
	if ((d = decor_find_by_type (c->decors, MBWMDecorTypeEast)))
	  *east = d->size;
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
  Pixmap                 xpxmp;
  MBWindowManager       *wm = theme->wm;
  cairo_pattern_t       *pattern;
  cairo_matrix_t         matrix;
  cairo_surface_t       *surface;
  cairo_t               *cr;
  double                 x, y, w, h;
  struct Clr             clr_bg;
  struct Clr             clr_fg;
  struct Clr             clr_frame;
  struct Clr             clr_bg2;
  MBWMClientType         c_type;
  MBWMThemeCairo        *c_theme = MB_WM_THEME_CAIRO (theme);
  struct Client * c;
  struct Decor  * d;
  int                    font_size = 0;
  const char            *font_family = "Sans Serif";

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

  type   = mb_wm_decor_get_type (decor);
  geom   = mb_wm_decor_get_geometry (decor);
  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if ((c = client_find_by_type (c_theme->xml_clients, c_type)) &&
      (d = decor_find_by_type (c->decors, decor->type)))
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
  struct Clr             clr_bg;
  struct Clr             clr_fg;
  MBWMClientType         c_type;
  MBWMThemeCairo        *c_theme = MB_WM_THEME_CAIRO (theme);
  struct Client * c;
  struct Decor  * d;
  struct Button * b;

  clr_fg.r = 1.0;
  clr_fg.g = 1.0;
  clr_fg.b = 1.0;

  clr_bg.r = 0.0;
  clr_bg.g = 0.0;
  clr_bg.b = 0.0;

  decor = button->decor;
  client = mb_wm_decor_get_parent (decor);
  xwin = button->xwin;
  if (client == NULL || xwin == None)
    return;

  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if ((c = client_find_by_type (c_theme->xml_clients, c_type)) &&
      (d = decor_find_by_type (c->decors, decor->type)) &&
      (b = button_find_by_type (d->buttons, button->type)))
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

  xpxmp = XCreatePixmap (wm->xdpy, xwin, wi, hi,
			 DefaultDepth(wm->xdpy, wm->xscreen));

  surface = cairo_xlib_surface_create (wm->xdpy,
				       xpxmp,
				       DefaultVisual(wm->xdpy, wm->xscreen),
				       wi, hi);
  cr = cairo_create(surface);

  cairo_set_line_width (cr, 0.04);
  cairo_set_source_rgb (cr, clr_bg.r, clr_bg.g, clr_bg.b);
  cairo_rectangle( cr, 0.0, 0.0, w, h);
  cairo_fill (cr);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, 3.0);
  cairo_set_source_rgb (cr, clr_fg.r, clr_fg.g, clr_fg.b);

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
