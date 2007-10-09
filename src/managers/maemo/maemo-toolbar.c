/*
 *  Matchbox Window Manager II - A lightweight window manager not for the
 *                               desktop.
 *
 *  Authored by Tomas Frydrych <tf@o-hand.com>
 *
 *  Copyright (c) 2007 OpenedHand Ltd - http://o-hand.com
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

#include "maemo-toolbar.h"

static void
maemo_toolbar_class_init (MBWMObjectClass *klass)
{
  MBWindowManagerClientClass *client;

  client = (MBWindowManagerClientClass *)klass;

  client->client_type = MBWMClientTypePanel;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MaemoToolbar";
#endif
}

static int
maemo_toolbar_init (MBWMObject *this, va_list vap)
{
  MBWindowManagerClient * client = MB_WM_CLIENT (this);
  MBWindowManager       * wm = client->wmref;
  MBGeometry              geom;
  MBWMClientWindow      * win = client->window;

  client->stacking_layer = MBWMStackLayerTop;
  client->want_focus = 0;

  mb_wm_client_set_layout_hints (client,
				 LayoutPrefReserveEdgeNorth |
				 LayoutPrefVisible          |
				 LayoutPrefFixedX           |
				 LayoutPrefOverlaps);


  if (!wm->theme)
    return 1;

  if (mb_wm_theme_get_client_geometry (wm->theme, client, &geom))
    {
      /* The theme prescribes geometry for the panel, resize the
       * window accordingly
       */
      int x, y, w, h;
      int g_x, g_y;
      unsigned int g_w, g_h, g_bw, g_d;

      x = geom.x;
      y = geom.y;
      w = geom.width;
      h = geom.height;

      /* If some of the theme values are unset, we have to get the current
       * values for the window in their place -- this is a round trip, so
       * we only do this when necessary
       */
      if (x < 0 || y < 0 || w < 0 || h < 0)
	{
	  Window root;
	  XGetGeometry (wm->xdpy, win->xwindow,
			&root, &g_x, &g_y, &g_w, &g_h, &g_bw, &g_d);
	}

      if (x < 0)
	x = g_x;

      if (y < 0)
	y = g_y;

      if (w < 0)
	w = g_w;

      if (h < 0)
	h = g_h;

      win->geometry.x = x;
      win->geometry.y = y;
      win->geometry.width  = w;
      win->geometry.height = h;

      XMoveResizeWindow (wm->xdpy, win->xwindow, x, y, w, h);
    }

  return 1;
}

static void
maemo_toolbar_destroy (MBWMObject *this)
{
}

int
maemo_toolbar_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MaemoToolbarClass),
	sizeof (MaemoToolbar),
	maemo_toolbar_init,
	maemo_toolbar_destroy,
	maemo_toolbar_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_CLIENT_PANEL, 0);
    }

  return type;
}

MBWindowManagerClient*
maemo_toolbar_new (MBWindowManager *wm, MBWMClientWindow *win)
{
  MBWindowManagerClient *client = NULL;

  client = MB_WM_CLIENT (mb_wm_object_new (MB_WM_TYPE_MAEMO_TOOLBAR,
					  MBWMObjectPropWm,           wm,
					  MBWMObjectPropClientWindow, win,
					  NULL));

  return client;
}


