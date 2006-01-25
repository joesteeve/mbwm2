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
mb_wm_client_base_realize (MBWindowManagerClient *client);

static void
mb_wm_client_base_stack (MBWindowManagerClient *client,
			 int                    flags);
static void
mb_wm_client_base_show (MBWindowManagerClient *client);

static void
mb_wm_client_base_hide (MBWindowManagerClient *client);

static void
mb_wm_client_base_destroy (MBWindowManagerClient *client);

static void
mb_wm_client_base_display_sync (MBWindowManagerClient *client);

static Bool
mb_wm_client_base_request_geometry (MBWindowManagerClient *client,
				    MBGeometry            *new_geometry,
				    MBWMClientReqGeomType  flags);

void
mb_wm_client_base_init (MBWindowManager             *wm, 
			MBWindowManagerClient       *client,
			MBWMWindow *win)
{
  client->wmref  = wm;   
  client->window = win; 

  client->new      = NULL;	             /* NOT NEEDED ? */
  client->init     = mb_wm_client_base_init;  /* NOT NEEDED ? */
  client->realize  = mb_wm_client_base_realize;
  client->destroy  = mb_wm_client_base_destroy;
  client->geometry = mb_wm_client_base_request_geometry;
  client->stack    = mb_wm_client_base_stack;
  client->show     = mb_wm_client_base_show;
  client->hide     = mb_wm_client_base_hide;
  client->sync     = mb_wm_client_base_display_sync;

  /* Need to set these values for initial reparenting */

  client->frame_geometry.x      = client->window->geometry.x;
  client->frame_geometry.y      = client->window->geometry.y;
  client->frame_geometry.width  = client->window->geometry.width;
  client->frame_geometry.height = client->window->geometry.height;
}

static void
mb_wm_client_base_realize (MBWindowManagerClient *client)
{
  MBWindowManager *wm = client->wmref;

  XSetWindowAttributes attr;

  MBWM_ASSERT(client->window != NULL);

  /* create the frame window */

  attr.override_redirect = True;
  attr.background_pixel  = BlackPixel(wm->xdpy, wm->xscreen);
  attr.event_mask = MBWMChildMask|MBWMButtonMask|ExposureMask;
  
  client->xwin_frame 
    = XCreateWindow(wm->xdpy, wm->xwin_root, 
		    client->frame_geometry.x,
		    client->frame_geometry.y,
		    client->frame_geometry.width,
		    client->frame_geometry.height,
		    0,
		    CopyFromParent, 
		    CopyFromParent, 
		    CopyFromParent,
		    CWOverrideRedirect|CWEventMask|CWBackPixel,
		    &attr);

  XClearWindow(wm->xdpy, client->xwin_frame);

  /* FIXME: below definetly belongs elsewhere ...  */
  XMoveResizeWindow(wm->xdpy, 
		    MBWM_CLIENT_XWIN(client),
		    client->window->geometry.x,
		    client->window->geometry.y,
		    client->window->geometry.width,
		    client->window->geometry.height);

  XReparentWindow(wm->xdpy, 
		  MBWM_CLIENT_XWIN(client), 
		  client->xwin_frame, 
		  client->window->geometry.x - client->frame_geometry.x,
		  client->window->geometry.y  -client->frame_geometry.y);

  XSelectInput(wm->xdpy, 
	       MBWM_CLIENT_XWIN(client),
	       PropertyChangeMask);
}

static void
mb_wm_client_base_stack (MBWindowManagerClient *client,
			 int                    flags)
{
  /* Stack to highest/lowest possible possition in stack */

  mb_wm_stack_append_top (client);
}

static void
mb_wm_client_base_show (MBWindowManagerClient *client)
{
  /* mark dirty somehow */
}

static void
mb_wm_client_base_hide (MBWindowManagerClient *client)
{
  
  /* mark dirty somehow */

}

static void
mb_wm_client_base_display_sync (MBWindowManagerClient *client)
{
  MBWindowManager *wm = client->wmref;

  MBWM_MARK();

  mb_wm_util_trap_x_errors();  

  if (mb_wm_client_needs_geometry_sync (client))
    {
      if (client->xwin_frame)
	XMoveResizeWindow(wm->xdpy, 
			  client->xwin_frame,
			  client->frame_geometry.x,
			  client->frame_geometry.y,
			  client->frame_geometry.width,
			  client->frame_geometry.height);
      
      XMoveResizeWindow(wm->xdpy, 
			MBWM_CLIENT_XWIN(client),
			client->window->geometry.x - client->frame_geometry.x,
			client->window->geometry.y - client->frame_geometry.y,
			client->window->geometry.width,
			client->window->geometry.height);
      
      /* FIXME: need flags to handle other stuff like confugre events etc */

      /* FIXME: Handle decor resizes - decor needs rethink */
    }

  if (mb_wm_client_needs_decor_sync (client))
    {
      /* Paint any decoration */

      /* Grep the list, call theming backend somehow */
    }

  if (mb_wm_client_needs_visibility_sync (client))
    {

      if (mb_wm_client_is_mapped (client))
	{
	  if (client->xwin_frame)
	    {
	      XMapWindow(wm->xdpy, client->xwin_frame); 
	      XMapSubwindows(wm->xdpy, client->xwin_frame);
	    }
	  else
	    XMapWindow(wm->xdpy, MBWM_CLIENT_XWIN(client)); 
	}
      else
	{
	  if (client->xwin_frame)
	    XUnmapWindow(wm->xdpy, client->xwin_frame); 
	  else
	    XMapWindow(wm->xdpy, MBWM_CLIENT_XWIN(client)); 
	}
    }

  XSync(wm->xdpy, False);
  mb_wm_util_untrap_x_errors();  
}

static void
mb_wm_client_base_destroy (MBWindowManagerClient *client)
{
  MBWindowManager *wm = client->wmref;

  MBWM_MARK();

  mb_wm_util_trap_x_errors();  

  XDestroyWindow(wm->xdpy, client->xwin_frame);

  XSync(wm->xdpy, False);
  mb_wm_util_untrap_x_errors();  
}

static Bool
mb_wm_client_base_request_geometry (MBWindowManagerClient *client,
				    MBGeometry            *new_geometry,
				    MBWMClientReqGeomType  flags)
{
  /* 
   *  flags are
   *
   *  MBReqGeomDontCommit
   *  MBReqGeomIsViaConfigureReq
   *  MBReqGeomIsViaUserAction
   *  MBReqGeomIsViaLayoutManager
   *  MBReqGeomForced
   *
  */

  /* Dont actually change anything - mb_wm_core_sync() should do that 
   * but mark dirty and 'queue any extra info like configure req'.   
  */

  if (flags & MBWMClientReqGeomIsViaLayoutManager)
    {
      MBWM_DBG("called with 'MBWMClientReqGeomIsViaLayoutManager' %ix%i+%i+%i",
	       new_geometry->width,
	       new_geometry->height,
	       new_geometry->x,
	       new_geometry->y);

      client->frame_geometry.x = new_geometry->x;
      client->frame_geometry.y = new_geometry->y;
      client->frame_geometry.width = new_geometry->width;
      client->frame_geometry.height = new_geometry->height;

      client->window->geometry.x = client->frame_geometry.x + 4;
      client->window->geometry.y = client->frame_geometry.y + 4;
      client->window->geometry.width  = client->frame_geometry.width - 8;
      client->window->geometry.height = client->frame_geometry.height - 8;

      mb_wm_client_geometry_mark_dirty (client);

      return True; /* Geometry accepted */
    }

  return TRUE;
}
