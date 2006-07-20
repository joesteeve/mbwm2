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
mb_wm_client_base_display_sync (MBWindowManagerClient *client);

static Bool
mb_wm_client_base_request_geometry (MBWindowManagerClient *client,
				    MBGeometry            *new_geometry,
				    MBWMClientReqGeomType  flags);

void
mb_wm_client_base_class_init (MBWMObjectClass *klass) 
{
  MBWindowManagerClientClass *client;

  MBWM_MARK();

  client = (MBWindowManagerClientClass *)klass;

  client->realize  = mb_wm_client_base_realize;
  client->geometry = mb_wm_client_base_request_geometry;
  client->stack    = mb_wm_client_base_stack;
  client->show     = mb_wm_client_base_show;
  client->hide     = mb_wm_client_base_hide;
  client->sync     = mb_wm_client_base_display_sync;

  MBWM_DBG("client->stack is %p", client->stack);
}

void
mb_wm_client_base_init (MBWMObject *this)
{
  mb_wm_client_init (this);
}

int
mb_wm_client_base_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMClientBaseClass),      
	sizeof (MBWMClientBase), 
	mb_wm_client_base_init,
	mb_wm_client_base_destroy,
	mb_wm_client_base_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_CLIENT);
    }

  return type;
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
  
  /* This should probably be called via rather than on new clien sync() ...? */

  if (client->xwin_frame == None)
    {
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
      
      /* Assume geomerty sync will fix this up correctly togeather with
       * any decoration creation. Layout manager will call this
       */
      XReparentWindow(wm->xdpy, 
		      MB_WM_CLIENT_XWIN(client), 
		      client->xwin_frame, 
		      0, 0);
    }

  XSetWindowBorderWidth(wm->xdpy, MB_WM_CLIENT_XWIN(client), 0);

  XAddToSaveSet(wm->xdpy, MB_WM_CLIENT_XWIN(client)); 

  XSelectInput(wm->xdpy, 
	       MB_WM_CLIENT_XWIN(client),
	       PropertyChangeMask);
}

static void
mb_wm_client_base_stack (MBWindowManagerClient *client,
			 int                    flags)
{
  /* Stack to highest/lowest possible possition in stack */

  mb_wm_stack_move_top(client);

  mb_wm_util_list_foreach (mb_wm_client_get_transients (client),
			   (MBWMListForEachCB)mb_wm_client_stack,
			   (void*)flags);
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

  /* Sync up any geometry */

  if (mb_wm_client_needs_geometry_sync (client))
    {
      mb_wm_util_trap_x_errors();  

      if (client->xwin_frame)
	XMoveResizeWindow(wm->xdpy, 
			  client->xwin_frame,
			  client->frame_geometry.x,
			  client->frame_geometry.y,
			  client->frame_geometry.width,
			  client->frame_geometry.height);

      /* FIXME: Call XConfigureWindow(w->dpy, e->window, value_mask, &xwc);
       *        here instead as can set border width = 0.
      */
      XMoveResizeWindow(wm->xdpy, 
			MB_WM_CLIENT_XWIN(client),
			client->window->geometry.x - client->frame_geometry.x,
			client->window->geometry.y - client->frame_geometry.y,
			client->window->geometry.width,
			client->window->geometry.height);
      
      /* FIXME: need flags to handle other stuff like configure events etc */

      if (mb_wm_client_needs_synthetic_config_event (client))
	; /* TODO: send fake config event */

      /* Resize any decor */
      mb_wm_util_list_foreach(client->decor, 
			      (MBWMListForEachCB)mb_wm_decor_handle_resize,
			      NULL);

      mb_wm_util_untrap_x_errors();  
    }

  /* Paint any decor */

  mb_wm_util_trap_x_errors();  

  if (mb_wm_client_needs_decor_sync (client))
    {
      mb_wm_util_list_foreach(client->decor, 
			      (MBWMListForEachCB)mb_wm_decor_handle_repaint,
			      NULL);
    }

  mb_wm_util_untrap_x_errors();  

  /* Handle any mapping - should be visible state ? */

  if (mb_wm_client_needs_visibility_sync (client))
    {
      MBWM_DBG("needs visibility sync");

      mb_wm_util_trap_x_errors();  

      if (mb_wm_client_is_mapped (client))
	{
	  if (client->xwin_frame)
	    {
	      XMapWindow(wm->xdpy, client->xwin_frame); 
	      XMapSubwindows(wm->xdpy, client->xwin_frame);
	    }
	  else
	    {
	      MBWM_DBG("mapping...");
	      XMapWindow(wm->xdpy, MB_WM_CLIENT_XWIN(client)); 
	    }

	  mb_wm_window_change_property (wm,
					client->window,
					wm->atoms[MBWM_ATOM_WM_STATE], 
					wm->atoms[MBWM_ATOM_WM_STATE],
					32, 
					(void *)NormalState, 
					1);
	}
      else
	{
	  if (client->xwin_frame)
	    XUnmapWindow(wm->xdpy, client->xwin_frame); 
	  else
	    XMapWindow(wm->xdpy, MB_WM_CLIENT_XWIN(client)); 

	  /* FIXME: iconized state? */
	  mb_wm_window_change_property (wm,
					client->window,
					wm->atoms[MBWM_ATOM_WM_STATE], 
					wm->atoms[MBWM_ATOM_WM_STATE],
					32, 
					(void *)WithdrawnState, 
					1);

	}

      mb_wm_util_untrap_x_errors();  
    }

  mb_wm_util_trap_x_errors();  
  XSync(wm->xdpy, False);
  mb_wm_util_untrap_x_errors();  
}

void
mb_wm_client_base_destroy (MBWMObject *this)
{
  MBWindowManagerClient *parent;
  MBWindowManagerClient *client;
  MBWindowManager *wm;

  MBWM_MARK();

  client = MB_WM_CLIENT(this);

  wm = client->wmref;

  mb_wm_util_trap_x_errors();  

  XDestroyWindow(wm->xdpy, client->xwin_frame);

  XSync(wm->xdpy, False);
  mb_wm_util_untrap_x_errors();  
  
  parent = mb_wm_client_get_transient_for (MB_WM_CLIENT(this));

  if (parent)
    mb_wm_client_remove_transient (parent, MB_WM_CLIENT(this));

  mb_wm_client_destroy (this);
}

/* Note request geometry always called by layout manager */
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

      client->frame_geometry.x      = new_geometry->x;
      client->frame_geometry.y      = new_geometry->y;
      client->frame_geometry.width  = new_geometry->width;
      client->frame_geometry.height = new_geometry->height;

      client->window->geometry.x = client->frame_geometry.x + 4;
      client->window->geometry.y = client->frame_geometry.y + 4;
      client->window->geometry.width  = client->frame_geometry.width - 8;
      client->window->geometry.height = client->frame_geometry.height - 8;

      mb_wm_client_geometry_mark_dirty (client);

      return True; /* Geometry accepted */
    }

  return True;
}

void base_foo(void)
{
  ; /* nasty hack to workaround linking issues WTF... */
}
