/*
 *  Matchbox Window Manager II - A lightweight window manager not for the
 *                               desktop.
 *
 *  Authored By Tomas Frydrych <tf@o-hand.com>
 *
 *  Copyright (c) 2005 OpenedHand Ltd - http://o-hand.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include "mb-wm-theme.h"
#include "mb-wm-theme-xml.h"

#include <sys/stat.h>
#include <expat.h>

static void
xml_element_start_cb (void *data, const char *tag, const char **expat_attr);

static void
xml_element_end_cb (void *data, const char *tag);

static void
xml_stack_free (MBWMList *stack);

static void
mb_wm_theme_destroy (MBWMObject *obj)
{
  MBWMTheme *theme = MB_WM_THEME (obj);

  if (theme->path)
    free (theme->path);

  MBWMList *l = theme->xml_clients;

  while (l)
    {
      MBWMXmlClient * c = l->data;
      MBWMList * n = l->next;
      mb_wm_xml_client_free (c);
      free (l);

      l = n;
    }
}

static int
mb_wm_theme_init (MBWMObject *obj, va_list vap)
{
  MBWMTheme        *theme = MB_WM_THEME (obj);
  MBWindowManager  *wm = NULL;
  MBWMObjectProp    prop;
  MBWMList         *xml_clients = NULL;
  char             *path = NULL;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropWm:
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	case MBWMObjectPropThemePath:
	  path = va_arg(vap, char *);
	  break;
	case MBWMObjectPropThemeXmlClients:
	  xml_clients = va_arg(vap, MBWMList *);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  theme->wm = wm;
  theme->xml_clients = xml_clients;

  if (path)
    theme->path = strdup (path);

  return 1;
}

int
mb_wm_theme_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMThemeClass),
	sizeof (MBWMTheme),
	mb_wm_theme_init,
	mb_wm_theme_destroy,
	NULL
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

void
mb_wm_theme_get_button_size (MBWMTheme             *theme,
			     MBWMDecor             *decor,
			     MBWMDecorButtonType    type,
			     int                   *width,
			     int                   *height)
{
  MBWMThemeClass *klass;

  MBWM_ASSERT (decor && decor->parent_client);

  if (!theme || !decor || !decor->parent_client)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->button_size)
    klass->button_size (theme, decor, type, width, height);
}

/*
 * If the parent decor uses absolute postioning, the returned values
 * are absolute. If the decor does packing, these values are added to
 * calculated button position.
 */
void
mb_wm_theme_get_button_position (MBWMTheme             *theme,
				 MBWMDecor             *decor,
				 MBWMDecorButtonType    type,
				 int                   *x,
				 int                   *y)
{
  MBWMThemeClass *klass;

  MBWM_ASSERT (decor && decor->parent_client);

  if (!theme || !decor || !decor->parent_client)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->button_position)
    klass->button_position (theme, decor, type, x, y);
  else
    {
      if (x)
	*x = 2;

      if (y)
	*y = 2;
    }
}

void
mb_wm_theme_get_decor_dimensions (MBWMTheme             *theme,
				  MBWindowManagerClient *client,
				  int                   *north,
				  int                   *south,
				  int                   *west,
				  int                   *east)
{
  MBWMThemeClass *klass;

  MBWM_ASSERT (client);

  if (!theme || !client)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->decor_dimensions)
    klass->decor_dimensions (theme, client, north, south, west, east);
}

void
mb_wm_theme_paint_decor (MBWMTheme *theme, MBWMDecor *decor)
{
  MBWMThemeClass *klass;

  if (!theme)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->paint_decor)
    klass->paint_decor (theme, decor);
}

void
mb_wm_theme_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
  MBWMThemeClass *klass;

  if (!theme)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->paint_button)
    klass->paint_button (theme, button);
}

Bool
mb_wm_theme_supports (MBWMTheme *theme, MBWMThemeCaps capability)
{
  if (theme)
    return False;

  return ((capability & theme->caps != False));
}

typedef enum
{
  XML_CTX_UNKNOWN = 0,
  XML_CTX_THEME,
  XML_CTX_CLIENT,
  XML_CTX_DECOR,
  XML_CTX_BUTTON,
  XML_CTX_IMG,
} XmlCtx;

struct stack_data
{
  XmlCtx  ctx;
  void   *data;
};

struct expat_data
{
  XML_Parser   par;
  int          theme_type;
  int          version;
  MBWMList     *xml_clients;
  char         *img;
  MBWMList     *stack;
};

MBWMTheme *
mb_wm_theme_new (MBWindowManager * wm, const char * theme_path)
{
  MBWMTheme *theme = NULL;
  int        theme_type = 0;
  char      *path = NULL;
  char       buf[256];
  XML_Parser par = NULL;
  FILE      *file = NULL;
  MBWMList  *xml_clients = NULL;
  char      *img = NULL;

  /* Attempt to parse the xml theme, if any, retrieving the theme type
   *
   * NB: We cannot do this in the _init function, since we need to know the
   *     type *before* we can create the underlying object on which the
   *     init method operates.
   */
  if (theme_path)
    {
      struct stat  st;

      if (*theme_path == '/')
	{
	  if (!stat (theme_path, &st))
	    path = (char *) theme_path;
	}
      else
	{
	  const char  *home = getenv("HOME");
	  int          size;

	  if (home)
	    {
	      const char  *fmt = "%s/.themes/%s/matchbox2/theme.xml";

	      size = strlen (theme_path) + strlen (fmt) + strlen (home);
	      path = alloca (size);
	      snprintf (path, size, fmt, home, theme_path);

	      if (stat (path, &st))
		path = NULL;
	    }

	  if (!path)
	    {
	      const char * fmt = "%s/themes/%s/matchbox2/theme.xml";

	      size = strlen (theme_path) + strlen (fmt) + strlen (DATADIR);
	      path = alloca (size);
	      snprintf (path, size, fmt, DATADIR, theme_path);

	      if (stat (path, &st))
		path = NULL;
	    }
	}

      if (path)
	{
	  struct expat_data  udata;

	  if (!(file = fopen (path, "r")) ||
	      !(par = XML_ParserCreate(NULL)))
	    {
	      goto default_theme;
	    }

	  memset (&udata, 0, sizeof (struct expat_data));
	  udata.par = par;

	  XML_SetElementHandler (par,
				 xml_element_start_cb,
				 xml_element_end_cb);

	  XML_SetUserData(par, (void *)&udata);

	  printf ("========= parsing [%s]\n", path);

	  while (fgets (buf, sizeof (buf), file) &&
		 XML_Parse(par, buf, strlen(buf), 0));

	  XML_Parse(par, NULL, 0, 1);

	  if (udata.version == 2)
	    {
	      theme_type  = udata.theme_type;
	      xml_clients = udata.xml_clients;

	      if (udata.img)
		{
		  if (*udata.img == '/')
		    img = udata.img;
		  else
		    {
		      int len = strlen (path) + strlen (udata.img);
		      char * s;
		      char * p = malloc (len + 1);
		      strncpy (p, path, len);

		      s = strrchr (p, '/');

		      if (s)
			{
			  *(s+1) = 0;
			  strcat (p, udata.img);
			}
		      else
			{
			  strncpy (p, udata.img, len);
			}

		      img = p;
		      free (udata.img);
		    }
		}
	    }

	  xml_stack_free (udata.stack);
	}
  }

  if (theme_type)
    {
      theme =
	MB_WM_THEME (mb_wm_object_new (theme_type,
				   MBWMObjectPropWm,        wm,
				   MBWMObjectPropThemePath, path,
				   MBWMObjectPropThemeImg,  img,
				   MBWMObjectPropThemeXmlClients, xml_clients,
				   NULL));
    }

 default_theme:

  if (!theme)
    {
      theme = MB_WM_THEME (mb_wm_object_new (
#ifdef USE_CAIRO
				   MB_WM_TYPE_THEME_CAIRO,
#else
				   MB_WM_TYPE_THEME_SIMPLE,
#endif
			           MBWMObjectPropWm, wm,
				   MBWMObjectPropThemeXmlClients, xml_clients,
			           NULL));
    }

  if (par)
    XML_ParserFree (par);

  if (file)
    fclose (file);

  if (img)
    free (img);

  return theme;
}

MBWMDecor *
mb_wm_theme_create_decor (MBWMTheme             *theme,
			  MBWindowManagerClient *client,
			  MBWMDecorType          type)
{
  MBWMThemeClass *klass;

  MBWM_ASSERT (client);

  if (!theme || !client)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->create_decor)
    klass->create_decor (theme, client, type);
}

/*
 * Returns True if the theme prescribes at least one value for the geometry
 */
Bool
mb_wm_theme_get_client_geometry (MBWMTheme             * theme,
				 MBWindowManagerClient * client,
				 MBGeometry            * geom)
{
  MBWMXmlClient * c;
  MBWMClientType  c_type;

  if (!geom || !client || !theme)
    return False;

  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if (!theme || !theme->xml_clients ||
      !(c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) ||
      (c->x < 0 && c->y < 0 && c->width < 0 && c->height < 0))
    {
      return False;
    }

  geom->x      = c->x;
  geom->y      = c->y;
  geom->width  = c->width;
  geom->height = c->height;

  return True;
}


/*
 * Expat callback stuff
 */

static void
xml_stack_push (MBWMList ** stack, XmlCtx ctx)
{
  struct stack_data * s = malloc (sizeof (struct stack_data));

  s->ctx = ctx;
  s->data = NULL;

  *stack = mb_wm_util_list_prepend (*stack, s);
}

static XmlCtx
xml_stack_top_ctx (MBWMList *stack)
{
  struct stack_data * s = stack->data;

  return s->ctx;
}

static void *
xml_stack_top_data (MBWMList *stack)
{
  struct stack_data * s = stack->data;

  return s->data;
}

static void
xml_stack_top_set_data (MBWMList *stack, void * data)
{
  struct stack_data * s = stack->data;

  s->data = data;
}

static void
xml_stack_pop (MBWMList ** stack)
{
  MBWMList * top = *stack;
  struct stack_data * s = top->data;

  *stack = top->next;
  free (s);
  free (top);
}

static void
xml_stack_free (MBWMList *stack)
{
  MBWMList * l = stack;
  while (l)
    {
      MBWMList * n = l->next;
      free (l->data);
      free (l);

      l = n;
    }
}

static void
xml_element_start_cb (void *data, const char *tag, const char **expat_attr)
{
  struct expat_data * exd = data;

  MBWM_DBG ("tag <%s>\n", tag);

  if (!strcmp (tag, "theme"))
    {
      const char ** p = expat_attr;

      xml_stack_push (&exd->stack, XML_CTX_THEME);

      while (*p)
	{
	  if (!strcmp (*p, "engine_version"))
	    exd->version = atoi (*(p+1));
	  else if (!strcmp (*p, "engine_type"))
	    {
	      if (!strcmp (*(p+1), "default"))
#ifdef USE_CAIRO
		exd->theme_type = MB_WM_TYPE_THEME_CAIRO;
#else
		exd->theme_type = MB_WM_TYPE_THEME_SIMPLE;
#endif
#ifdef THEME_PNG
	      else if (!strcmp (*(p+1), "png"))
		exd->theme_type = MB_WM_TYPE_THEME_PNG;
#endif
	    }

	  p += 2;
	}
    }

  if (!strcmp (tag, "client"))
    {
      MBWMXmlClient * c = mb_wm_xml_client_new ();
      const char **p = expat_attr;

      XmlCtx ctx = xml_stack_top_ctx (exd->stack);

      xml_stack_push (&exd->stack, XML_CTX_CLIENT);

      if (ctx != XML_CTX_THEME)
	{
	  MBWM_DBG ("Expected context theme");
	  return;
	}

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
	  else if (!strcmp (*p, "width"))
	    {
	      c->width = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "height"))
	    {
	      c->height = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "x"))
	    {
	      c->x = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "y"))
	    {
	      c->y = atoi (*(p+1));
	    }

	  p += 2;
	}

      if (!c->type)
	mb_wm_xml_client_free (c);
      else
	{
	  exd->xml_clients = mb_wm_util_list_prepend (exd->xml_clients, c);
	  xml_stack_top_set_data (exd->stack, c);
	}


      return;
    }

  if (!strcmp (tag, "decor"))
    {
      MBWMXmlDecor * d = mb_wm_xml_decor_new ();
      const char **p = expat_attr;
      XmlCtx ctx = xml_stack_top_ctx (exd->stack);
      MBWMXmlClient * c = xml_stack_top_data (exd->stack);

      xml_stack_push (&exd->stack, XML_CTX_DECOR);

      if (ctx != XML_CTX_CLIENT || !c)
	{
	  MBWM_DBG ("Expected context client");
	  return;
	}

      while (*p)
	{
	  if (!strcmp (*p, "clr-fg"))
	    mb_wm_xml_clr_from_string (&d->clr_fg, *(p+1));
	  else if (!strcmp (*p, "clr-bg"))
	    mb_wm_xml_clr_from_string (&d->clr_bg, *(p+1));
	  else if (!strcmp (*p, "clr-bg2"))
	    mb_wm_xml_clr_from_string (&d->clr_bg2, *(p+1));
	  else if (!strcmp (*p, "clr-frame"))
	    mb_wm_xml_clr_from_string (&d->clr_frame, *(p+1));
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
	  else if (!strcmp (*p, "width"))
	    {
	      d->width = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "height"))
	    {
	      d->height = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "x"))
	    {
	      d->x = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "y"))
	    {
	      d->y = atoi (*(p+1));
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

      if (!d->type)
	mb_wm_xml_decor_free (d);
      else
	{
	  c->decors = mb_wm_util_list_prepend (c->decors, d);
	  xml_stack_top_set_data (exd->stack, d);
	}

      return;
    }

  if (!strcmp (tag, "button"))
    {
      MBWMXmlButton * b = mb_wm_xml_button_new ();
      const char **p = expat_attr;
      XmlCtx ctx = xml_stack_top_ctx (exd->stack);
      MBWMXmlDecor * d = xml_stack_top_data (exd->stack);

      xml_stack_push (&exd->stack, XML_CTX_BUTTON);

      if (ctx != XML_CTX_DECOR || !d)
	{
	  MBWM_DBG ("Expected context decor");
	  return;
	}

      while (*p)
	{
	  if (!strcmp (*p, "clr-fg"))
	    mb_wm_xml_clr_from_string (&b->clr_fg, *(p+1));
	  else if (!strcmp (*p, "clr-bg"))
	    mb_wm_xml_clr_from_string (&b->clr_bg, *(p+1));
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
	  else if (!strcmp (*p, "x"))
	    {
	      b->x = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "y"))
	    {
	      b->y = atoi (*(p+1));
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

      if (!b->type)
	{
	  mb_wm_xml_button_free (b);
	  return;
	}

      d->buttons = mb_wm_util_list_append (d->buttons, b);

      xml_stack_top_set_data (exd->stack, b);

      return;
    }

  if (!strcmp (tag, "img"))
    {
      const char **p = expat_attr;
      XmlCtx ctx = xml_stack_top_ctx (exd->stack);

      xml_stack_push (&exd->stack, XML_CTX_IMG);

      if (ctx != XML_CTX_THEME)
	{
	  MBWM_DBG ("Expected context theme");
	  return;
	}

      while (*p)
	{
	  if (!strcmp (*p, "src"))
	    {
	      exd->img = strdup (*(p+1));
	      return;
	    }

	  p += 2;
	}

      return;
    }

}

static void
xml_element_end_cb (void *data, const char *tag)
{
  struct expat_data * exd = data;

  XmlCtx ctx = xml_stack_top_ctx (exd->stack);

  MBWM_DBG ("tag </%s>\n", tag);

  if (!strcmp (tag, "theme"))
    {
      XML_StopParser (exd->par, 0);
    }
  else if (!strcmp (tag, "client"))
    {
      if (ctx == XML_CTX_CLIENT)
	{
	  xml_stack_pop (&exd->stack);
	}
      else
	MBWM_DBG ("Expected client on the top of the stack!");
    }
  else if (!strcmp (tag, "decor"))
    {
      if (ctx == XML_CTX_DECOR)
	{
	  xml_stack_pop (&exd->stack);
	}
      else
	MBWM_DBG ("Expected decor on the top of the stack!");
    }
  else if (!strcmp (tag, "button"))
    {
      if (ctx == XML_CTX_BUTTON)
	{
	  xml_stack_pop (&exd->stack);
	}
      else
	MBWM_DBG ("Expected button on the top of the stack!");
    }
  else if (!strcmp (tag, "img"))
    {
      if (ctx == XML_CTX_IMG)
	{
	  xml_stack_pop (&exd->stack);
	}
      else
	MBWM_DBG ("Expected img on the top of the stack!");
    }
}

