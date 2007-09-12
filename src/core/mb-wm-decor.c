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
mb_wm_decor_destroy (MBWMObject *obj);

static void
mb_wm_decor_button_sync_window (MBWMDecorButton *button);

static void
mb_wm_decor_init (MBWMObject *obj, va_list vap)
{
  MBWMDecor             *decor = MB_WM_DECOR (obj);
  MBWindowManager       *wm = NULL;
  MBWMDecorType          type;
  MBWMDecorResizedFunc   resize;
  MBWMDecorRepaintFunc   repaint;
  void                  *userdata;
  char                  *prop;

  prop = va_arg(vap, char *);
  while (prop)
    {
      if (!strcmp ("wm", prop))
	{
	  wm = va_arg(vap, MBWindowManager *);
	}
      else if (!strcmp ("type", prop))
	{
	  type = va_arg(vap, MBWMDecorType);
	}
      else if (!strcmp ("resize", prop))
	{
	  resize = va_arg(vap, MBWMDecorResizedFunc);
	}
      else if (!strcmp ("repaint", prop))
	{
	  repaint = va_arg(vap, MBWMDecorRepaintFunc);
	}
      else if (!strcmp ("user-data", prop))
	{
	  userdata = va_arg(vap, void*);
	}

      prop = va_arg(vap, char *);
    }

  decor->type     = type;
  decor->dirty    = True; 	/* Needs painting */
  decor->resize   = resize;
  decor->repaint  = repaint;
  decor->userdata = userdata;
}

int
mb_wm_decor_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMDecorClass),
	sizeof (MBWMDecor),
	mb_wm_decor_init,
	mb_wm_decor_destroy,
	NULL
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

static Bool
mb_wm_decor_reparent (MBWMDecor *decor);

static Bool
mb_wm_decor_sync_window (MBWMDecor *decor)
{
  MBWindowManager     *wm;
  XSetWindowAttributes attr;

  if (decor->parent_client == NULL)
    return False;

  wm = decor->parent_client->wmref;

  attr.override_redirect = True;
  attr.background_pixel  = BlackPixel(wm->xdpy, wm->xscreen);

  if (decor->xwin == None)
    {
      mb_wm_util_trap_x_errors();

      decor->xwin
	= XCreateWindow(wm->xdpy,
			wm->root_win->xwindow,
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

      /* Fire resize now so calling code knows initial size */
      if (decor->resize)
	decor->resize(decor->parent_client->wmref, decor, decor->userdata);

      mb_wm_util_list_foreach(decor->buttons,
		             (MBWMListForEachCB)mb_wm_decor_button_sync_window,
			      NULL);

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

      /* Next up sort buttons */

      mb_wm_util_list_foreach(decor->buttons,
			      (MBWMListForEachCB)mb_wm_decor_button_sync_window,
			      NULL);

      if (mb_wm_util_untrap_x_errors())
	return False;
    }

  return True;
}

static Bool
mb_wm_decor_reparent (MBWMDecor *decor)
{
  MBWindowManager *wm;

  if (decor->parent_client == NULL)
    return False;

  MBWM_MARK();

  wm = decor->parent_client->wmref;

  /* FIXME: Check if we already have a parent here */

  mb_wm_util_trap_x_errors();

  XReparentWindow (wm->xdpy,
		   decor->xwin,
		   decor->parent_client->xwin_frame,
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

  if (decor->parent_client == NULL)
    return;

  client = decor->parent_client;
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
  if (decor->parent_client == NULL)
    return;

  if (decor->dirty && decor->repaint)
    decor->repaint(decor->parent_client->wmref, decor, decor->userdata);

  decor->dirty = False;
}

void
mb_wm_decor_handle_resize (MBWMDecor *decor)
{
  if (decor->parent_client == NULL)
    return;

  mb_wm_decor_calc_geometry (decor);

  mb_wm_decor_sync_window (decor);

  /* Fire resize callback */
  if (decor->resize)
    decor->resize(decor->parent_client->wmref, decor, decor->userdata);

  /* Fire repaint callback */
  mb_wm_decor_mark_dirty (decor);
}

MBWMDecor*
mb_wm_decor_new (MBWindowManager      *wm,
		 MBWMDecorType         type,
		 MBWMDecorResizedFunc  resize,
		 MBWMDecorRepaintFunc  repaint,
		 void                 *userdata)
{
  MBWMDecor *decor;

  decor = MB_WM_DECOR(mb_wm_object_new (MB_WM_TYPE_DECOR,
					"wm",        wm,
					"type",      type,
					"resize",    resize,
					"repaint",   repaint,
					"user-data", userdata,
					NULL));

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

MBWindowManagerClient*
mb_wm_decor_get_parent (MBWMDecor *decor)
{
  return decor->parent_client;
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

  if (decor->parent_client)
    mb_wm_client_decor_mark_dirty (decor->parent_client);
}

void
mb_wm_decor_attach (MBWMDecor             *decor,
		    MBWindowManagerClient *client)
{
  decor->parent_client = client;
  client->decor = mb_wm_util_list_append(client->decor, decor);

  mb_wm_decor_mark_dirty (decor);

  return;
}

void
mb_wm_decor_detach (MBWMDecor *decor)
{
}

static void
mb_wm_decor_destroy (MBWMObject* obj)
{
  MBWMDecor *decor = MB_WM_DECOR(obj);

  mb_wm_decor_detach (decor);

  /* XDestroyWindow(wm->dpy, decor->xwin); */
}

/* Buttons */
static void
mb_wm_decor_button_destroy (MBWMObject* obj);

static Bool
mb_wm_decor_button_press_handler (MBWindowManager *wm,
				  XButtonEvent    *xev,
				  void            *userdata)
{
  MBWMDecorButton *button;

  button = (MBWMDecorButton *)userdata;

  if (xev->window == button->xwin)
    {
      button->state = MBWMDecorButtonStatePressed;

      MBWM_DBG("button is %ix%i\n", button->geom.width, button->geom.height);
      if (button->press)
	button->press(wm, button, button->userdata);

      return False;
    }

  return True;
}

static Bool
mb_wm_decor_button_release_handler (MBWindowManager *wm,
				    XButtonEvent    *xev,
				    void            *userdata)
{
  MBWMDecorButton *button;

  button = (MBWMDecorButton *)userdata;

  if (xev->window == button->xwin)
    {
      button->state = MBWMDecorButtonStateInactive;

      MBWM_DBG("got release");
      if (button->release)
	button->release(wm, button, button->userdata);

      return False;
    }

  return True;
}

static void
mb_wm_decor_button_init (MBWMObject *obj, va_list vap)
{
  MBWMDecorButton             *button = MB_WM_DECOR_BUTTON (obj);
  MBWindowManager             *wm = NULL;
  MBWMDecor                   *decor = NULL;
  int                          width;
  int                          height;
  MBWMDecorButtonPressedFunc   press;
  MBWMDecorButtonReleasedFunc  release;
  MBWMDecorButtonRepaintFunc   paint;
  MBWMDecorButtonFlags         flags;
  void                        *userdata = NULL;
  char                        *prop;

  prop = va_arg(vap, char *);
  while (prop)
    {
      if (!strcmp ("wm", prop))
	{
	  wm = va_arg(vap, MBWindowManager *);
	}
      else if (!strcmp ("decor", prop))
	{
	  decor = va_arg(vap, MBWMDecor*);
	}
      else if (!strcmp ("width", prop))
	{
	  width = va_arg(vap, int);
	}
      else if (!strcmp ("height", prop))
	{
	  height = va_arg(vap, int);
	}
      else if (!strcmp ("press", prop))
	{
	  press = va_arg(vap, MBWMDecorButtonPressedFunc);
	}
      else if (!strcmp ("release", prop))
	{
	  release = va_arg(vap, MBWMDecorButtonReleasedFunc);
	}
      else if (!strcmp ("paint", prop))
	{
	  paint = va_arg(vap, MBWMDecorButtonRepaintFunc);
	}
      else if (!strcmp ("flags", prop))
	{
	  flags = va_arg(vap, MBWMDecorButtonFlags);
	}
      else if (!strcmp ("user-data", prop))
	{
	  userdata = va_arg(vap, void*);
	}

      prop = va_arg(vap, char *);
    }

  if (!wm)
    return;

  button->press    = press;
  button->release  = release;
  button->repaint  = paint;
  button->userdata = userdata;
  button->decor    = decor;

  button->geom.width  = width;
  button->geom.height = height;
  button->decor  = decor;
  decor->buttons = mb_wm_util_list_append (decor->buttons, button);

  mb_wm_x_event_handler_add (wm, ButtonPress,
			     (MBWMXEventFunc)mb_wm_decor_button_press_handler,
			     button);

  mb_wm_x_event_handler_add (wm, ButtonRelease,
		            (MBWMXEventFunc)mb_wm_decor_button_release_handler,
			     button);
}

int
mb_wm_decor_button_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMDecorButtonClass),
	sizeof (MBWMDecorButton),
	mb_wm_decor_button_init,
	mb_wm_decor_button_destroy,
	NULL
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

static void
mb_wm_decor_button_destroy (MBWMObject* obj)
{
  /*
  MBWMDecor *decor = MB_WM_DECOR(obj);
  */
}

static Bool
mb_wm_decor_button_realize (MBWMDecorButton *button)
{
  MBWindowManager     *wm;
  XSetWindowAttributes attr;

  wm = button->decor->parent_client->wmref;

  attr.override_redirect = True;
  attr.background_pixel  = BlackPixel(wm->xdpy, wm->xscreen);
  attr.event_mask = ButtonPressMask|ButtonReleaseMask;

  if (button->xwin == None)
    {
      mb_wm_util_trap_x_errors();

      /* NOTE: may want input only window here if button paints
       *       directly onto decor.
      */

      /* FIXME: Event Mask */
      button->xwin
	= XCreateWindow(wm->xdpy,
			button->decor->xwin,
			button->geom.x,
			button->geom.y,
			button->geom.width,
			button->geom.height,
			0,
			CopyFromParent,
			CopyFromParent,
			CopyFromParent,
			CWOverrideRedirect|CWBackPixel|CWEventMask,
			&attr);

      MBWM_DBG("@@@ Button Reparented @@@");

      /* FIXME: call paint() */

      if (mb_wm_util_untrap_x_errors())
	return False;
      }
}

static void
mb_wm_decor_button_sync_window (MBWMDecorButton *button)
{
  MBWindowManager     *wm;

  MBWM_MARK();

  wm = button->decor->parent_client->wmref;

  if (button->xwin == None)
    mb_wm_decor_button_realize (button);

  MBWM_DBG("####### X moving to %i, %i\n",
	   button->geom.x,
	   button->geom.y);


  /* FIXME: conditional */
  XMoveWindow(wm->xdpy,
	      button->xwin,
	      button->geom.x,
	      button->geom.y);

  if (button->visible)
    XMapWindow(wm->xdpy, button->xwin);
  else
    XUnmapWindow(wm->xdpy, button->xwin);
}

void
mb_wm_decor_button_show (MBWMDecorButton *button)
{
  button->visible = True;
}

void
mb_wm_decor_button_hide (MBWMDecorButton *button)
{
  button->visible = False;
}

void
mb_wm_decor_button_move_to (MBWMDecorButton *button, int x, int y)
{
  /* FIXME: set a sync flag so it know X movewindow is needed */

  button->geom.x = x;
  button->geom.y = y;

  MBWM_DBG("#######  moving to %i, %i\n",
	   button->geom.x,
	   button->geom.y);

}

MBWMDecorButton*
mb_wm_decor_button_new (MBWindowManager            *wm,
			MBWMDecor                  *decor,
			int                         width,
			int                         height,
			MBWMDecorButtonPressedFunc  press,
			MBWMDecorButtonReleasedFunc release,
			MBWMDecorButtonRepaintFunc  paint,
			MBWMDecorButtonFlags        flags,
			void                       *userdata)
{
  MBWMDecorButton  *button;

  button = MB_WM_DECOR_BUTTON(mb_wm_object_new (MB_WM_TYPE_DECOR_BUTTON,
						"wm",        wm,
						"decor",     decor,
						"width",     width,
						"height",    height,
						"press",     press,
						"release",   release,
						"paint",     paint,
						"flags",     flags,
						"user-data", userdata,
						NULL));

  return button;
}

