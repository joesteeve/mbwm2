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
  MBWMThemeClass *klass =
    MB_WM_THEME_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(theme)));

  if (klass->paint_decor)
    klass->paint_decor (theme, decor);
}

void
mb_wm_theme_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
  MBWMThemeClass *klass =
    MB_WM_THEME_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(theme)));

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
	  if (!stat (path, &st))
	    path = (char *) theme_path;
	}
      else
	{
	  const char  *fmt = "%s/.themes/%s/matchbox2/theme.xml";
	  const char  *home = getenv("HOME");
	  int          size;

	  if (home)
	    {
	      size = strlen (theme_path) + strlen (fmt) + strlen (home);
	      path = alloca (size);
	      snprintf (path, size, fmt, home, theme_path);

	      if (stat (path, &st))
		path = NULL;
	    }

	  if (!path)
	    {
	      size = strlen (theme_path) + strlen (fmt) + strlen (DATADIR);
	      path = alloca (size);
	      snprintf (path, size, fmt, DATADIR, theme_path);

	      if (stat (path, &st))
		path = NULL;
	    }
	}

      if (path)
	{
	  /*
	   * FIXME -- retrieve engine type from theme so we can create the
	   * appropriate object
	   */
	}
      else
	{
	  MBWM_DBG ("Could not locate theme [%s]", theme_path);
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

