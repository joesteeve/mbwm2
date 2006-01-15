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
      if (c->type == type)
	highest_client = c;
    }

  return highest_client;
}

MBWindowManagerClient*
mb_wm_stack_get_lowest_by_type(MBWindowManager *w, int wanted_type)

{
  MBWindowManagerClient *c = NULL;

  mb_wm_stack_enumerate(w,c)
    if (c->type == wanted_type)
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

      if (client->stacked_below != NULL) client->stacked_below->stacked_above = client->stacked_above;
      if (client->stacked_above != NULL) client->stacked_above->stacked_below = client->stacked_below;
    }

  client->stacked_above = client->stacked_below = NULL;

  wm->stack_n_clients--;
}


