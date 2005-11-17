/* 
 *  Matchbox Window Manager II - A lightweight window manager not for the
 *                               desktop.
 *
 *  Authored By Matthew Allum <mallum@o-hand.com>
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

#include "mb-wm.h"

enum
  {
    StackingNeedsSync   = (1<<1),
    GeometryNeedsSync   = (1<<2),
    VisibilityNeedsSync = (1<<3),
  };

struct MBWindowManagerClientPriv
{
  Bool realized;
  Bool mapped;
  int  sync_state;
};

void
mb_wm_client_stacking_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref);
  client->priv->sync_state |= StackingNeedsSync;
}

void
mb_wm_client_geometry_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref);

  client->priv->sync_state |= GeometryNeedsSync;
}

void
mb_wm_client_visibility_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref);

  client->priv->sync_state |= VisibilityNeedsSync;

  MBWM_DBG(" sync state: %i", client->priv->sync_state);
}


MBWindowManagerClient* 	/* FIXME: rename to mb_wm_client_base/class_new ? */
mb_wm_client_new (MBWindowManager *wm, MBWMWindow *win)
{
  MBWindowManagerClient *client = NULL;

  client = mb_wm_util_malloc0(sizeof(MBWindowManagerClient));

  if (!client)
    return NULL; 		/* FIXME: Handle out of memory */

  return client;
}

void
mb_wm_client_init (MBWindowManager             *wm, 
		   MBWindowManagerClient       *client,
		   MBWMWindow                  *win)
{
  client->priv   = mb_wm_util_malloc0(sizeof(MBWindowManagerClientPriv));

  if (client->init)
    client->init(wm, client, win);
  else
    mb_wm_client_base_init (wm, client, win);      
}

void
mb_wm_client_realize (MBWindowManagerClient *client)
{
  MBWM_ASSERT (client->realize != NULL);

  client->realize(client);

  client->priv->realized = True;
}

Bool
mb_wm_client_is_realized (MBWindowManagerClient *client)
{
  return client->priv->realized;
}


/* ## Stacking ## */


void
mb_wm_client_stack (MBWindowManagerClient *client,
		    int                    flags)
{
  MBWM_ASSERT (client->stack != NULL);

  client->stack(client, flags);

  mb_wm_client_stacking_mark_dirty (client);
}

Bool
mb_wm_client_needs_stack_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & StackingNeedsSync);
}



void
mb_wm_client_show (MBWindowManagerClient *client)
{
  MBWM_ASSERT (client->show != NULL);

  client->show(client);

  client->priv->mapped = True;

  mb_wm_client_visibility_mark_dirty (client);
}


void
mb_wm_client_hide (MBWindowManagerClient *client)
{
  MBWM_ASSERT (client->hide != NULL);

  client->hide(client);

  client->priv->mapped = False;

  mb_wm_client_visibility_mark_dirty (client);
}

Bool
mb_wm_client_needs_visibility_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & VisibilityNeedsSync);
}

Bool
mb_wm_client_needs_geometry_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & GeometryNeedsSync);
}

Bool
mb_wm_client_needs_sync (MBWindowManagerClient *client)
{
  MBWM_DBG("sync_state: %i", client->priv->sync_state);

  return client->priv->sync_state;
}

Bool
mb_wm_client_is_mapped (MBWindowManagerClient *client)
{
  return client->priv->mapped;
}


void
mb_wm_client_display_sync (MBWindowManagerClient *client)
{
  MBWM_ASSERT (client->sync != NULL);

  client->sync(client);

  client->priv->sync_state = 0;
}

void
mb_wm_client_destroy (MBWindowManagerClient *client)
{
  /* free everything up */
  MBWM_ASSERT (client->destroy != NULL);

  client->destroy(client);

  mb_wm_display_sync_queue (client->wmref);
}


Bool
mb_wm_client_request_geometry (MBWindowManagerClient *client,
			       MBGeometry            *new_geometry,
			       MBWMClientReqGeomType  flags)
{
  MBWM_ASSERT (client->geometry != NULL);

  return client->geometry(client, new_geometry, flags); 
}

MBWMClientLayoutHints
mb_wm_client_get_layout_hints (MBWindowManagerClient *client)
{
  return client->layout_hints;
}

void
mb_wm_client_set_layout_hints (MBWindowManagerClient *client,
			       MBWMClientLayoutHints  hints)
{
  client->layout_hints = hints;
}

void
mb_wm_client_set_layout_hint (MBWindowManagerClient *client,
			      MBWMClientLayoutHints  hint,
			      Bool                   state)
{
  if (state)
    client->layout_hints |= hint;
  else
    client->layout_hints &= ~hint;
}

void  /* needs to be boolean, client may not have any coverage */
mb_wm_client_get_coverage (MBWindowManagerClient *client,
			   MBGeometry            *coverage)
{
  MBGeometry *geometry;

  if (client->xwin_frame)
    geometry = &client->frame_geometry; 
  else
    geometry = &client->window->geometry;

  coverage->x      = geometry->x;
  coverage->y      = geometry->y;
  coverage->width  = geometry->width;
  coverage->height = geometry->height;
}


