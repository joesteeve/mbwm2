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

struct MBWMDecor
{
  MBWMDecorType          type;
  Window                 xwin;
  MBWindowManagerClient *parent;

  MBGeometry             geom;
  Bool                   dirty;

  MBWMDecorResizedFunc   resize;
  MBWMDecorResizedFunc   repaint;
  void                  *userdata;

  int                    refcnt;
};


struct MBWMDecorButton
{
  Window  xwin;
  int     refcnt;
};

static Bool
mb_wm_decor_reparent (MBWMDecor *decor);

static Bool
mb_wm_decor_sync_window (MBWMDecor *decor)
{
  MBWindowManager     *wm;
  XSetWindowAttributes attr;

  if (decor->parent == NULL)
    return False;

  wm = decor->parent->wmref;  

  attr.override_redirect = True;
  attr.background_pixel  = BlackPixel(wm->xdpy, wm->xscreen);
  
  if (decor->xwin == None)
    {
      mb_wm_util_trap_x_errors();  

      decor->xwin 
	= XCreateWindow(wm->xdpy, 
			wm->xwin_root, 
			decor->geom.x,
			decor->geom.y,
			decor->geom.width,
			decor->geom.height,
			0,
			CopyFromParent, 
			CopyFromParent, 
			CopyFromParent,
			CWOverrideRedirect|CWBackPixel,
			&attr);

      MBWM_DBG("g is +%i+%i %ix%i",
	       decor->geom.x,
	       decor->geom.y,
	       decor->geom.width,
	       decor->geom.height);
      
      if (mb_wm_util_untrap_x_errors())
	return False;
      
      return mb_wm_decor_reparent (decor);
    }
  else
    {
      /* Resize */
      mb_wm_util_trap_x_errors();  

      XMoveResizeWindow(wm->xdpy, 
			decor->xwin,
			decor->geom.x,
			decor->geom.y,
			decor->geom.width,
			decor->geom.height);

      if (mb_wm_util_untrap_x_errors())
	return False;
    }

  return True;
}

static Bool
mb_wm_decor_reparent (MBWMDecor *decor)
{
  MBWindowManager *wm;

  if (decor->parent == NULL)
    return False;

  MBWM_MARK();

  wm = decor->parent->wmref;  

  /* FIXME: Check if we already have a parent here */

  mb_wm_util_trap_x_errors();  

  XReparentWindow (wm->xdpy, 
		   decor->xwin, 
		   decor->parent->xwin_frame, 
		   decor->geom.x, 
		   decor->geom.y);

  if (mb_wm_util_untrap_x_errors())
    return False;

  return True;
}

static void
mb_wm_decor_calc_geometry (MBWMDecor *decor)
{
  MBWindowManager       *wm;
  MBWindowManagerClient *client;

  if (decor->parent == NULL)
    return;

  client = decor->parent;
  wm = client->wmref;  

  switch (decor->type)
    {
    case MBWMDecorTypeNorth:
      decor->geom.x      = 0;
      decor->geom.y      = 0;
      decor->geom.height = mb_wm_client_frame_north_height(client);
      decor->geom.width  = client->frame_geometry.width;
      break;
    case MBWMDecorTypeSouth:
      decor->geom.x      = 0;
      decor->geom.y      = mb_wm_client_frame_south_y(client);
      decor->geom.height = mb_wm_client_frame_south_height(client);
      decor->geom.width  = client->frame_geometry.width;
      break;
    case MBWMDecorTypeWest:
      decor->geom.x      = 0;
      decor->geom.y      = mb_wm_client_frame_north_height(client);
      decor->geom.height = client->window->geometry.height;
      decor->geom.width  = mb_wm_client_frame_west_width(client);
      break;
    case MBWMDecorTypeEast:
      decor->geom.x      = mb_wm_client_frame_east_x(client);
      decor->geom.y      = mb_wm_client_frame_north_height(client);
      decor->geom.height = client->window->geometry.height;
      decor->geom.width  = mb_wm_client_frame_east_width(client);
      break;
    default:
      /* FIXME: some kind of callback for custom types here ? */
      break;
    }

  MBWM_DBG("geom is +%i+%i %ix%i, Type %i",
	   decor->geom.x,
	   decor->geom.y,
	   decor->geom.width,
	   decor->geom.height,
	   decor->type);


  return;
}

void
mb_wm_decor_handle_map (MBWMDecor *decor)
{
  /* Not needed as XMapSubWindows() is used */
}


void
mb_wm_decor_handle_repaint (MBWMDecor *decor)
{
  if (decor->parent == NULL)
    return;

  if (decor->dirty && decor->repaint)
    decor->repaint(decor->parent->wmref, decor, decor->userdata);

  decor->dirty = False;
}

void
mb_wm_decor_handle_resize (MBWMDecor *decor)
{
  if (decor->parent == NULL)
    return;

  mb_wm_decor_calc_geometry (decor);

  mb_wm_decor_sync_window (decor);

  /* Fire resize callback */
  if (decor->resize)
    decor->resize(decor->parent->wmref, decor, decor->userdata);

  /* Fire repaint callback */
  mb_wm_decor_mark_dirty (decor);
}

MBWMDecor*
mb_wm_decor_create (MBWindowManager     *wm,
		    MBWMDecorType        type,
		    MBWMDecorResizedFunc resize,
		    MBWMDecorResizedFunc repaint,
		    void                *userdata)
{
  MBWMDecor           *decor;

  decor = mb_wm_util_malloc0(sizeof(MBWMDecor));

  decor->type    = type;
  decor->dirty   = True; 	/* Needs painting */
  decor->resize  = resize;
  decor->repaint = repaint;
  decor->userdata = userdata;

  mb_wm_decor_ref (decor);

  return decor;
}

Window
mb_wm_decor_get_x_window (MBWMDecor *decor)
{
  return decor->xwin;
}

MBWMDecorType
mb_wm_decor_get_type (MBWMDecor *decor)
{
  return decor->type;
}

const MBGeometry*
mb_wm_decor_get_geometry (MBWMDecor *decor)
{
  return &decor->geom;
}

/* Mark a client in need of a repaint */
void
mb_wm_decor_mark_dirty (MBWMDecor *decor)
{
  decor->dirty = True;

  if (decor->parent)
    mb_wm_client_decor_mark_dirty (decor->parent);
}

void
mb_wm_decor_attach (MBWMDecor             *decor,
		    MBWindowManagerClient *client)
{
  decor->parent = client;
  client->decor = mb_wm_util_list_append(client->decor, decor);

  mb_wm_decor_mark_dirty (decor);

  return;
}

void
mb_wm_decor_detach (MBWMDecor *decor)
{

}

void
mb_wm_decor_unref (MBWMDecor *decor)
{
 if (--decor->refcnt <= 0)
   {
     mb_wm_decor_detach (decor);
     /* XDestroyWindow(wm->dpy, decor->xwin); */
     free(decor);
   }
}

void
mb_wm_decor_ref (MBWMDecor *decor)
{
  decor->refcnt++;
}

#if 0

MBWMDecorButton*
mb_wm_decor_button_create (MBWindowManager            *wm, 
			   int                         width,
			   int                         height,
			   MBWMDecorButtonFlags        flags,
			   MBWMDecorButtonActivateFunc func,
			   void                       *userdata)
{

}

void
mb_wm_decor_button_add (MBWMDecor       *decor,
			MBWMDecorButton *button,
			int              x,
			int              y)
{

}

#endif
