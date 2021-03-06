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

static Bool
maemo_toolbar_request_geometry (MBWindowManagerClient *client,
				MBGeometry            *new_geometry,
				MBWMClientReqGeomType  flags);

static void
maemo_toolbar_stack (MBWindowManagerClient *client);

static MBWMStackLayerType
maemo_toolbar_stacking_layer (MBWindowManagerClient *client);

static void
maemo_toolbar_class_init (MBWMObjectClass *klass)
{
  MBWindowManagerClientClass *client;

  client = (MBWindowManagerClientClass *)klass;

  client->client_type = MBWMClientTypePanel;

  client->geometry = maemo_toolbar_request_geometry;
  client->stack = maemo_toolbar_stack;
  client->stacking_layer = maemo_toolbar_stacking_layer;

#if MBWM_WANT_DEBUG
  klass->klass_name = "MaemoToolbar";
#endif
}

static int
maemo_toolbar_init (MBWMObject *this, va_list vap)
{
  MBWindowManagerClient * client = MB_WM_CLIENT (this);
  MaemoToolbar          * toolbar = MAEMO_TOOLBAR (this);
  MBWindowManager       * wm = client->wmref;
  MBGeometry              geom;
  MBGeometry            * w_geom = &client->window->geometry;
  MBWMClientWindow      * win = client->window;
  MBWMClientLayoutHints   hints;
  int                     x, y, w, h;

  hints = mb_wm_client_get_layout_hints (client);

  /* FIXME -- is there a better way to differentiate between the
   * toolbar and the TN ?
   *
   * (Matchbox panel attachment is normally worked out on the basis of the
   * original window position, but both the TN and the toolbar seem to
   * have orgin at 0,0, which in MB means NorthEdge panel.)
   */
  if (w_geom->width > 80)
    hints |= LayoutPrefReserveEdgeNorth;
  else
    hints |= LayoutPrefReserveEdgeWest;

  if (hints & LayoutPrefReserveEdgeNorth)
    {
      hints |= (LayoutPrefFixedX | LayoutPrefOverlaps);
      client->stacking_layer = MBWMStackLayerTopMid;
    }
  else
    {
      toolbar->task_navigator = True;
      client->stacking_layer = MBWMStackLayerBottomMid;
    }

  mb_wm_client_set_layout_hints (client, hints);

  if (hints & LayoutPrefReserveEdgeNorth)
    {
      x = 425;
      y = 0;
      w = 280;
      h = 50;
    }
  else
    {
      w = 80;
    }

  if (!wm->theme)
    {
      if (hints & LayoutPrefReserveEdgeNorth)
	{
	  win->geometry.x = x;
	  win->geometry.y = y;
	  win->geometry.width  = w;
	  win->geometry.height = h;
  
	  mb_wm_client_geometry_mark_dirty (client);
	}
      else
	{
	  win->geometry.width  = w;
	  mb_wm_client_geometry_mark_dirty (client);
	}

      return 1;
    }

  if (mb_wm_theme_get_client_geometry (wm->theme, client, &geom))
    {
      /* The theme prescribes geometry for the panel, resize the
       * window accordingly
       */
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

      mb_wm_client_geometry_mark_dirty (client);
    }
  else if (hints & LayoutPrefReserveEdgeNorth)
    {
      /* Fallback values for the status bar */
      win->geometry.x = x;
      win->geometry.y = y;
      win->geometry.width  = w;
      win->geometry.height = h;

      mb_wm_client_geometry_mark_dirty (client);
    }
  else
    {
      win->geometry.width  = w;
      mb_wm_client_geometry_mark_dirty (client);
    }

  return 1;
}

static Bool
maemo_toolbar_request_geometry (MBWindowManagerClient *client,
				MBGeometry            *new_geometry,
				MBWMClientReqGeomType  flags)
{
  /*
   * Refuse to resize the panels
   */
#if 0
  if (client->window->geometry.x != new_geometry->x
      || client->window->geometry.y != new_geometry->y
      || client->window->geometry.width  != new_geometry->width
      || client->window->geometry.height != new_geometry->height)
    {
      printf ("======== reguested geometry %d, %d; %d x %d\n",
	      new_geometry->x,
	      new_geometry->y,
	      new_geometry->width,
	      new_geometry->height);

      client->window->geometry.x = new_geometry->x;
      client->window->geometry.y = new_geometry->y;
      client->window->geometry.width  = new_geometry->width;
      client->window->geometry.height = new_geometry->height;

      mb_wm_client_geometry_mark_dirty (client);
    }
#endif

  return True;
}

static MBWMStackLayerType
maemo_toolbar_stacking_layer (MBWindowManagerClient *client)
{
  /*
   * If we are showing desktop, ensure that we stack above it.
   */
  if (client->wmref->flags & MBWindowManagerFlagDesktop)
    return MBWMStackLayerTopMid;

  return client->stacking_layer;
}

static void
maemo_toolbar_stack (MBWindowManagerClient *client)
{
  MBGeometry              geom;

  /*
   * If this is 'normal' panel, i.e., the TN, we stack with the default
   * value set by maemo_toolbar_init().
   *
   * If this is the status bar, stack immediately above the top-level
   * application.
   */

  if (!(client->layout_hints & LayoutPrefOverlaps))
    {
      mb_wm_stack_move_top (client);
      return;
    }

  mb_wm_stack_move_client_above_type (client,
				     MBWMClientTypeApp|MBWMClientTypeDesktop);
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


