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
#include "mb-wm-theme-private.h"

#include <sys/stat.h>
#include <expat.h>

static void
xml_element_start_cb (void *data, const char *tag, const char **expat_attr);

static void
mb_wm_theme_destroy (MBWMObject *obj)
{
  MBWMTheme *theme = MB_WM_THEME (obj);

  if (theme->path)
    free (theme->path);

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
mb_wm_theme_init (MBWMObject *obj, va_list vap)
{
  MBWMTheme        *theme = MB_WM_THEME (obj);
  MBWindowManager  *wm = NULL;
  MBWMObjectProp    prop;
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
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  theme->wm = wm;

  if (path)
    theme->path = strdup (path);
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

  if (klass->button_size)
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

struct expat_data
{
  XML_Parser   par;
  int          theme_type;
  int          version;
  MBWMList     *xml_clients;
};

MBWMTheme *
mb_wm_theme_new (MBWindowManager * wm, const char * theme_path)
{
  MBWMTheme *theme;
  int        theme_type = 0;
  char      *path = NULL;
  char       buf[256];
  XML_Parser par = NULL;
  FILE      *file = NULL;
  MBWMList  *xml_clients = NULL;

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
	      path = NULL;

	      if (file)
		{
		  fclose (file);
		  file = NULL;
		}
	      goto default_theme;
	    }

	  udata.theme_type  = 0;
	  udata.version     = 0;
	  udata.par         = par;
	  udata.xml_clients = NULL;

	  XML_SetElementHandler (par, xml_element_start_cb, NULL);
	  XML_SetUserData(par, (void *)&udata);

	  printf ("========= parsing [%s]\n", path);

	  while (fgets (buf, sizeof (buf), file) &&
		 XML_Parse(par, buf, strlen(buf), 0));

	  XML_Parse(par, NULL, 0, 1);

	  if (udata.version == 2)
	    {
	      theme_type = udata.theme_type;
	      xml_clients = udata.xml_clients;
	    }

	  fclose (file);
	  XML_ParserFree (par);
	}
  }

  if (theme_type)
    {
      theme =
	MB_WM_THEME (mb_wm_object_new (theme_type,
				       MBWMObjectPropWm,        wm,
				       MBWMObjectPropThemePath, path,
				       NULL));

      if (theme)
	{
	  theme->xml_clients = xml_clients;
	  return theme;
	}
    }

 default_theme:

  if (par)
    XML_ParserFree (par);

  /* Fall back on the default cairo theme */
  return MB_WM_THEME (mb_wm_object_new (MB_WM_TYPE_THEME_CAIRO,
					MBWMObjectPropWm, wm,
					NULL));
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

/*****************************************************************
 * XML Parser stuff
 */
struct Button *
button_new ()
{
  struct Button * b = mb_wm_util_malloc0 (sizeof (struct Button));

  b->x = -1;
  b->y = -1;

  return b;
}

void
button_free (struct Button * b)
{
  free (b);
}

struct Decor *
decor_new ()
{
  struct Decor * d = mb_wm_util_malloc0 (sizeof (struct Decor));
  return d;
}

void
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

struct Client *
client_new ()
{
  struct Client * c = mb_wm_util_malloc0 (sizeof (struct Client));
  return c;
}

void
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

void
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

static void
xml_element_start_cb_2 (void *data, const char *tag, const char **expat_attr)
{
  MBWMTheme *theme = data;

  if (!expat_attr)
    return;

}

struct Client *
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

struct Decor *
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

struct Button *
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
void
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

static void
xml_element_start_cb (void *data, const char *tag, const char **expat_attr)
{
  struct expat_data * exd = data;

  if (!strcmp (tag, "theme"))
    {
      const char ** p = expat_attr;

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
	exd->xml_clients = mb_wm_util_list_prepend (exd->xml_clients, c);

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

      if (!d->type || !exd->xml_clients)
	  decor_free (d);
      else
	{
	  struct Client * c = exd->xml_clients->data;
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
	  !exd->xml_clients ||
	  !exd->xml_clients->data)
	{
	  button_free (b);
	  return;
	}

      struct Client * c = exd->xml_clients->data;

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
