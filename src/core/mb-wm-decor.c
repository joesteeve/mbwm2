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
  Window                 xwin;
  MBWindowManagerClient *parent;
  int                    x,y,width,height;
  Bool                   dirty;
  int                    refcnt;
};


struct MBWMDecorButton
{
  Window  xwin;
  int     refcnt;
};

MBWMDecor*
mb_wm_decor_create (MBWindowManager *wm, 
		    int              width,
		    int              height)
{
  MBWMDecor           *decor;
  XSetWindowAttributes attr;

  decor = mb_wm_util_malloc0(sizeof(MBWMDecor));

  attr.override_redirect = True;
  attr.background_pixel  = BlackPixel(wm->xdpy, wm->xscreen);
  
  mb_wm_util_trap_x_errors();  

  decor->xwin 
    = XCreateWindow(wm->xdpy, 
		    wm->xwin_root, 
		    0,0,width,height,
		    0,
		    CopyFromParent, 
		    CopyFromParent, 
		    CopyFromParent,
		    CWOverrideRedirect|CWBackPixel,
		    &attr);

  decor->width  = width;
  decor->height = height;
  decor->dirty  = True; 	/* Needs painting */

  mb_wm_decor_ref (decor);

  if (mb_wm_util_untrap_x_errors())
    {
      free(decor); 
      decor = NULL;
    }

  return decor;
}

Window
mb_wm_decor_get_x_window (MBWMDecor *decor)
{
  return decor->xwin;
}

/* Mark a client in need of a repaint */
void
mb_wm_decor_mark_dirty (MBWMDecor *decor)
{
  decor->dirty = True;

  if (decor->parent)
    mb_wm_client_decor_mark_dirty (decor->parent);
}

Bool
mb_wm_decor_attach (MBWMDecor             *decor,
		    MBWindowManagerClient *client,
		    int                    x,
		    int                    y)
{
  MBWindowManager *wm;

  wm = client->wmref;  

  /* FIXME: Check if we already have a parent here */

  mb_wm_util_trap_x_errors();  

  XReparentWindow (wm->xdpy, 
		   client->xwin_frame, 
		   decor->xwin, x, y);

  if (mb_wm_util_untrap_x_errors())
    return False;

  decor->parent = client;

  decor->x = x;
  decor->y = y;

  client->decor = mb_wm_util_list_append(client->decor, decor);

  return TRUE;
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
