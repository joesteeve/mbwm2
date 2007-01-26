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


static void
mb_wm_stack_ensure_trans_foreach (MBWindowManagerClient *client, void *data)
{
  mb_wm_stack_move_top (client);

  mb_wm_util_list_foreach 
    (mb_wm_client_get_transients (client),
     (MBWMListForEachCB) mb_wm_stack_ensure_trans_foreach,
     NULL);
}

void
mb_wm_stack_dump (MBWindowManager *wm)
{
#if (MBWM_WANT_DEBUG)
  MBWindowManagerClient *client;

  fprintf(stderr, "\n==== window stack =====\n");

  mb_wm_stack_enumerate_reverse (wm, client)
    {
      fprintf(stderr, "XID: %lx NAME: %s\n",
	      MB_WM_CLIENT_XWIN(client),
	      client->window->name ? client->window->name : "unknown" );
    }

  fprintf(stderr, "======================\n\n");
#endif
}

void
mb_wm_stack_ensure (MBWindowManager *wm)
{
  MBWindowManagerClient *client, *seen, *next;
  int                    i;

  if (wm->stack_bottom == NULL)
    return;

  /* Ensure the window stack is corrent;
   *  - with respect to client layer types
   *  - transients are stacked within these layers also
   *
   * We need to be careful here as we modify stacking list
   * in place while enumerating it.
   *
   * FIXME: This isn't optimal
  */

  /* bottom -> top on layer types */
  for (i=1; i<N_MBWMStackLayerTypes; i++)
    {
      /* Push each layer type to top, handling transients */
      client = wm->stack_bottom;
      seen   = NULL;

      while (client != seen && client != NULL)
	{
	  /* get the next valid client ( ignore transients ) before 
	   * modifying the list  
	  */
	  next = client->stacked_above;

	  while (next && mb_wm_client_get_transient_for (next))
	    next = next->stacked_above;

	  if (client->stacking_layer == i 
	      && mb_wm_client_get_transient_for (client) == NULL)
	    {
	      /* Keep track of the first client modified so we 
               * know when to stop iterating.
	      */
	      if (seen == NULL)
		seen = client;

	      mb_wm_stack_move_top (client);
	      
	      /* push transients to the top also */
	      mb_wm_util_list_foreach 
		(mb_wm_client_get_transients (client),
		 (MBWMListForEachCB) mb_wm_stack_ensure_trans_foreach,
		 NULL);
	    }
	  client = next;
	}
    }

  /* TODO: add a call here into clients stack method so they can overide
   *       these rules and do specific tweaks - i.e a dialog transient to
   *       a panel may want to be on a different layer.
   */
  mb_wm_stack_dump (wm);
}

void
mb_wm_stack_insert_above_client (MBWindowManagerClient *client, 
				 MBWindowManagerClient *client_below)
{
  MBWindowManager *wm = client->wmref;

  MBWM_ASSERT (client != NULL);

  if (client_below == NULL)
    {
      /* NULL so nothing below add at bottom */
      if (wm->stack_bottom) 
	{
	  client->stacked_above = wm->stack_bottom;
	  wm->stack_bottom->stacked_below = client;
	}

      wm->stack_bottom = client;
    }
  else
    {
      client->stacked_below = client_below;
      client->stacked_above = client_below->stacked_above;
      if (client->stacked_below) client->stacked_below->stacked_above = client;
      if (client->stacked_above) client->stacked_above->stacked_below = client;
    }

  if (client_below == wm->stack_top)
    wm->stack_top = client;

  wm->stack_n_clients++;
}


void 
mb_wm_stack_append_top (MBWindowManagerClient *client)
{
  MBWindowManager *wm = client->wmref;

  mb_wm_stack_insert_above_client(client, wm->stack_top);
}

void 
mb_wm_stack_prepend_bottom (MBWindowManagerClient *client)
{
  mb_wm_stack_insert_above_client(client, NULL);
}

void
mb_wm_stack_move_client_above_type (MBWindowManagerClient *client, 
				    int                    type_below)
{
  MBWindowManager       *wm = client->wmref;
  MBWindowManagerClient *highest_client = NULL;

  highest_client = mb_wm_stack_get_highest_by_type (wm, type_below);

  if (highest_client)
    mb_wm_stack_move_above_client(client, highest_client);
} 


void
mb_wm_stack_move_above_client (MBWindowManagerClient *client, 
			       MBWindowManagerClient *client_below)
{
  if (client == client_below) return;

  MBWM_ASSERT (client != NULL);
  MBWM_ASSERT (client_below != NULL);

  mb_wm_stack_remove(client);
  mb_wm_stack_insert_above_client(client, client_below);
}

MBWindowManagerClient*
mb_wm_stack_get_highest_by_type (MBWindowManager       *wm, 
				 int                    type)
{
  MBWindowManagerClient *highest_client = NULL, *c = NULL;

  mb_wm_stack_enumerate(wm,c)
    {
      if (MB_WM_OBJECT_TYPE(c) == type)
	highest_client = c;
    }

  return highest_client;
}

MBWindowManagerClient*
mb_wm_stack_get_lowest_by_type(MBWindowManager *w, int wanted_type)

{
  MBWindowManagerClient *c = NULL;

  mb_wm_stack_enumerate(w,c)
    if (MB_WM_OBJECT_TYPE(c) == wanted_type)
      return c;

  return NULL;
}

void
mb_wm_stack_cycle_by_type(MBWindowManager *wm, int type)
{
  MBWindowManagerClient *lowest, *highest;

  lowest  = mb_wm_stack_get_lowest_by_type (wm, type);
  highest = mb_wm_stack_get_highest_by_type (wm, type);

  if (lowest && highest && lowest != highest)
    {
      mb_wm_stack_move_above_client (lowest, highest);
    }
}

void
mb_wm_stack_remove (MBWindowManagerClient *client)
{
  MBWindowManager *wm = client->wmref;

  if (wm->stack_top == wm->stack_bottom)
    {
      wm->stack_top = wm->stack_bottom = NULL;
    }
  else
    {
      if (client == wm->stack_top)
	wm->stack_top = client->stacked_below;

      if (client == wm->stack_bottom)
	wm->stack_bottom = client->stacked_above;

      if (client->stacked_below != NULL) 
	client->stacked_below->stacked_above = client->stacked_above;
      if (client->stacked_above != NULL) 
	client->stacked_above->stacked_below = client->stacked_below;
    }

  client->stacked_above = client->stacked_below = NULL;

  wm->stack_n_clients--;
}

