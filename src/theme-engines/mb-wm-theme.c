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

#include <sys/stat.h>
#include <expat.h>

static void
mb_wm_theme_destroy (MBWMObject *obj)
{
  MBWMTheme *theme = MB_WM_THEME (obj);

  if (theme->path)
    free (theme->path);
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
mb_wm_theme_paint_decor (MBWMTheme *theme, MBWMDecor *decor)
{
  MBWMThemeClass *klass;

  if (!theme)
    return;

  klass = MB_WM_THEME_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(theme)));

  if (klass->paint_decor)
    klass->paint_decor (theme, decor);
}

void
mb_wm_theme_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
  MBWMThemeClass *klass;

  if (!theme)
    return;

  klass = MB_WM_THEME_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(theme)));

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
};

static void
xml_element_start_cb(void *data, const char *tag, const char **expat_attr)
{
  static Bool seen_theme_el = False;

  struct expat_data * d = data;

  if (!strcmp (tag, "theme"))
    {
      const char ** p = expat_attr;
      Bool seen_version = False;
      Bool seen_type    = False;

      while (*p)
	{
	  if (!strcmp (*p, "engine_version"))
	    {
	      d->version = atoi (*(p+1));
	      seen_version = True;
	    }
	  else if (!strcmp (*p, "engine_type"))
	    {
	      seen_type = True;

#ifdef USE_CAIRO
	      if (!strcmp (*(p+1), "cairo"))
		d->theme_type = MB_WM_TYPE_THEME_CAIRO;
#else
	      if (!strcmp (*(p+1), "simple"))
		d->theme_type = MB_WM_TYPE_THEME_SIMPLE;
#endif
#ifdef THEME_PNG
	      else if (!strcmp (*(p+1), "png"))
		d->theme_type = MB_WM_TYPE_THEME_PNG;
#endif
	    }

	  if (seen_version && seen_type)
	    {
	      XML_StopParser (d->par, 0);
	      break;
	    }

	  p += 2;
	}

      seen_theme_el = True;

    }

  if (strcmp (tag, "theme") && seen_theme_el)
    XML_StopParser (d->par, 0);
}

MBWMTheme *
mb_wm_theme_new (MBWindowManager * wm, const char * theme_path)
{
  MBWMTheme *theme;
  int        theme_type = 0;
  char      *path = NULL;

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
	  char               buf[256];
	  struct expat_data  udata;
	  FILE              *file = NULL;
	  XML_Parser         par = NULL;

	  if (!(file = fopen (path, "r")) ||
	      !(par = XML_ParserCreate(NULL)))
	    {
	      path = NULL;
	      goto done;
	    }

	  udata.theme_type = 0;
	  udata.version    = 0;
	  udata.par        = par;

	  XML_SetElementHandler (par, xml_element_start_cb, NULL);
	  XML_SetUserData(par, (void *)&udata);

	  while (fgets (buf, sizeof (buf), file) &&
		 XML_Parse(par, buf, strlen(buf), 0));

	  if (udata.version == 2)
	    theme_type = udata.theme_type;

	done:
	  if (file)
	    fclose (file);

	  if (par)
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
	return theme;
    }

  /* Fall back on the default cairo theme */
  return MB_WM_THEME (mb_wm_object_new (MB_WM_TYPE_THEME_CAIRO,
					MBWMObjectPropWm, wm,
					NULL));
}

