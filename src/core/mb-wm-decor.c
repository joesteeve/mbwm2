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
mb_wm_decor_class_init (MBWMObjectClass *klass)
{
#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMDecor";
#endif
}

static int
mb_wm_decor_init (MBWMObject *obj, va_list vap)
{
  MBWMDecor             *decor = MB_WM_DECOR (obj);
  MBWindowManager       *wm = NULL;
  MBWMDecorType          type = 0;
  MBWMObjectProp         prop;
  int                    i = 0;
  int                    abs_packing = 0;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropWm:
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	case MBWMObjectPropDecorType:
	  type = va_arg(vap, MBWMDecorType);
	  break;
	case MBWMObjectPropDecorAbsolutePacking:
	  abs_packing = va_arg(vap, int);
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  decor->type     = type;
  decor->dirty    = True; 	/* Needs painting */
  decor->absolute_packing = abs_packing;

  return 1;
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
	mb_wm_decor_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

static void
mb_wm_decor_repaint (MBWMDecor *decor)
{
  MBWMTheme   *theme = decor->parent_client->wmref->theme;

  mb_wm_theme_paint_decor (theme, decor);
}

static void
mb_wm_decor_resize (MBWMDecor *decor)
{
  const MBGeometry *geom;
  MBWMTheme        *theme = decor->parent_client->wmref->theme;
  MBWMList         *l;
  int               btn_x_start, btn_x_end;
  int               abs_packing = decor->absolute_packing;

  geom = mb_wm_decor_get_geometry (decor);

  btn_x_end = geom->width;
  btn_x_start = 0;

  l = decor->buttons;

  if (abs_packing)
    {
      while (l)
	{
	  int off_x, off_y;

	  MBWMDecorButton  *btn = (MBWMDecorButton  *)l->data;
	  mb_wm_theme_get_button_position (theme, decor, btn->type,
					   &off_x, &off_y);

	  mb_wm_decor_button_move_to (btn, off_x, off_y);

	  l = l->next;
	}
    }
  else
    {
      while (l)
	{
	  int off_x, off_y;

	  MBWMDecorButton  *btn = (MBWMDecorButton  *)l->data;
	  mb_wm_theme_get_button_position (theme, decor, btn->type,
					   &off_x, &off_y);

	  if (btn->pack == MBWMDecorButtonPackEnd)
	    {
	      btn_x_end -= (btn->geom.width + off_x);
	      mb_wm_decor_button_move_to (btn, btn_x_end, off_y);
	    }
	  else
	    {
	      mb_wm_decor_button_move_to (btn, btn_x_start + off_x, off_y);
	      btn_x_start += btn->geom.width;
	    }

	  l = l->next;
	}
    }

  decor->pack_start_x = btn_x_start;
  decor->pack_end_x   = btn_x_end;
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

      mb_wm_decor_resize(decor);

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
  MBWMList *l;
  if (decor->parent_client == NULL)
    return;

  if (decor->dirty)
    {
      mb_wm_decor_repaint(decor);

      l = decor->buttons;
      while (l)
	{
	  MBWMDecorButton * button = l->data;
	  mb_wm_decor_button_handle_repaint (button);

	  l = l->next;
	}

      decor->dirty = False;
    }
}

void
mb_wm_decor_handle_resize (MBWMDecor *decor)
{
  if (decor->parent_client == NULL)
    return;

  mb_wm_decor_calc_geometry (decor);

  mb_wm_decor_sync_window (decor);

  /* Fire resize callback */
  mb_wm_decor_resize(decor);

  /* Fire repaint callback */
  mb_wm_decor_mark_dirty (decor);
}

MBWMDecor*
mb_wm_decor_new (MBWindowManager      *wm,
		 MBWMDecorType         type)
{
  MBWMObject *decor;

  decor = mb_wm_object_new (MB_WM_TYPE_DECOR,
			    MBWMObjectPropWm,               wm,
			    MBWMObjectPropDecorType,        type,
			    NULL);

  return MB_WM_DECOR(decor);
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

int
mb_wm_decor_get_pack_start_x (MBWMDecor *decor)
{
  return decor->pack_start_x;
}


int
mb_wm_decor_get_pack_end_x (MBWMDecor *decor)
{
  return decor->pack_end_x;
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
  MBWMList * l = decor->buttons;

  mb_wm_decor_detach (decor);

  while (l)
    {
      MBWMList * old = l;
      mb_wm_object_unref (MB_WM_OBJECT (l->data));
      l = l->next;
      free (old);
    }

  /* XDestroyWindow(wm->dpy, decor->xwin); */
}

/* Buttons */
static void
mb_wm_decor_button_destroy (MBWMObject* obj);

static void
mb_wm_decor_button_stock_button_pressed (MBWMDecorButton *button)
{
  MBWindowManagerClient *client = button->decor->parent_client;
  MBWindowManager       *wm = client->wmref;

#if 0
  /* FIXME -- deal with modality -- these should perhaps be ignored
   * if system modal dialog is present ??
   */
  switch (type)
    {
    case MBWMDecorButtonClose:
    case MBWMDecorButtonMinimize:
    case MBWMDecorButtonFullscreen:
      return;
    default:
      ;
    }
#endif

  switch (button->type)
    {
    case MBWMDecorButtonClose:
      mb_wm_client_deliver_delete (client);
      break;
    case MBWMDecorButtonMinimize:
      mb_wm_client_set_state (client,
			      MBWM_ATOM_NET_WM_STATE_HIDDEN,
			      MBWMClientWindowStateChangeAdd);
      break;
    case MBWMDecorButtonFullscreen:
      mb_wm_client_set_state (client,
			      MBWM_ATOM_NET_WM_STATE_FULLSCREEN,
			      MBWMClientWindowStateChangeAdd);
      break;
    case MBWMDecorButtonAccept:
      mb_wm_client_deliver_wm_protocol (client,
				wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_ACCEPT]);
      break;
    case MBWMDecorButtonHelp:
      mb_wm_client_deliver_wm_protocol (client,
				wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_HELP]);
      break;
    default:
    case MBWMDecorButtonMenu:
      mb_wm_client_deliver_wm_protocol (client,
				wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_CUSTOM]);
      break;
    }

  return;
}

static Bool
mb_wm_decor_button_press_handler (XButtonEvent    *xev,
				  void            *userdata)
{
  MBWMDecorButton *button = (MBWMDecorButton *)userdata;
  MBWindowManager *wm = button->decor->parent_client->wmref;

  if (xev->window == button->xwin)
    {
      button->state = MBWMDecorButtonStatePressed;

      MBWM_DBG("button is %ix%i\n", button->geom.width, button->geom.height);
      if (button->press)
	button->press(wm, button, button->userdata);
      else
	mb_wm_decor_button_stock_button_pressed (button);

      return False;
    }

  return True;
}

static Bool
mb_wm_decor_button_release_handler (XButtonEvent    *xev,
				    void            *userdata)
{
  MBWMDecorButton *button = (MBWMDecorButton *)userdata;
  MBWindowManager *wm = button->decor->parent_client->wmref;

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
mb_wm_decor_button_class_init (MBWMObjectClass *klass)
{
#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMDecorButton";
#endif
}

static int
mb_wm_decor_button_init (MBWMObject *obj, va_list vap)
{
  MBWMDecorButton             *button = MB_WM_DECOR_BUTTON (obj);
  MBWindowManager             *wm = NULL;
  MBWMDecor                   *decor = NULL;
  MBWMDecorButtonPressedFunc   press = NULL;
  MBWMDecorButtonReleasedFunc  release = NULL;
  MBWMDecorButtonFlags         flags = 0;
  MBWMDecorButtonType          type = 0;
  MBWMDecorButtonPack          pack = MBWMDecorButtonPackEnd;
  void                        *userdata = NULL;
  MBWMObjectProp               prop;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropWm:
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	case MBWMObjectPropDecor:
	  decor = va_arg(vap, MBWMDecor*);
	  break;
	case MBWMObjectPropDecorButtonPressedFunc:
	  press = va_arg(vap, MBWMDecorButtonPressedFunc);
	  break;
	case MBWMObjectPropDecorButtonReleasedFunc:
	  release = va_arg(vap, MBWMDecorButtonReleasedFunc);
	  break;
	case MBWMObjectPropDecorButtonFlags:
	  flags = va_arg(vap, MBWMDecorButtonFlags);
	  break;
	case MBWMObjectPropDecorButtonUserData:
	  userdata = va_arg(vap, void*);
	  break;
	case MBWMObjectPropDecorButtonType:
	  type = va_arg(vap, MBWMDecorButtonType);
	  break;
	case MBWMObjectPropDecorButtonPack:
	  pack = va_arg(vap, MBWMDecorButtonPack);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  if (!wm || !decor)
    return 0;

  /*
   * Decors must be attached before we can start adding buttons to them,
   * otherwise we cannot work out the button geometry.
   */
  MBWM_ASSERT (decor->parent_client);

  button->geom.width  = 0;
  button->geom.height = 0;

  mb_wm_theme_get_button_size (wm->theme,
			       decor,
			       type,
			       &button->geom.width,
			       &button->geom.height);

  button->press    = press;
  button->release  = release;
  button->userdata = userdata;
  button->decor    = decor;
  button->type = type;
  button->pack = pack;

  decor->buttons = mb_wm_util_list_append (decor->buttons, button);

  /* the decor assumes a reference, so add one for the caller */
  mb_wm_object_ref (obj);

  return 1;
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
	mb_wm_decor_button_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

static void
mb_wm_decor_button_destroy (MBWMObject* obj)
{
  MBWMDecorButton * button = MB_WM_DECOR_BUTTON (obj);
  MBWMMainContext * ctx = button->decor->parent_client->wmref->main_ctx;

  mb_wm_main_context_x_event_handler_remove (ctx, ButtonPress,
					     button->press_cb_id);

  mb_wm_main_context_x_event_handler_remove (ctx, ButtonRelease,
					     button->release_cb_id);
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

      button->press_cb_id =
	mb_wm_main_context_x_event_handler_add (wm->main_ctx,
			    button->xwin,
			    ButtonPress,
			    (MBWMXEventFunc)mb_wm_decor_button_press_handler,
			    button);

      button->release_cb_id =
	mb_wm_main_context_x_event_handler_add (wm->main_ctx,
			    button->xwin,
			    ButtonRelease,
			   (MBWMXEventFunc)mb_wm_decor_button_release_handler,
			   button);

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
			MBWMDecorButtonPack         pack,
			MBWMDecor                  *decor,
			MBWMDecorButtonPressedFunc  press,
			MBWMDecorButtonReleasedFunc release,
			MBWMDecorButtonFlags        flags,
			void                       *userdata)
{
  MBWMObject  *button;

  button = mb_wm_object_new (MB_WM_TYPE_DECOR_BUTTON,
			     MBWMObjectPropWm,                      wm,
			     MBWMObjectPropDecorButtonPack,         pack,
			     MBWMObjectPropDecor,                   decor,
			     MBWMObjectPropDecorButtonPressedFunc,  press,
			     MBWMObjectPropDecorButtonReleasedFunc, release,
			     MBWMObjectPropDecorButtonFlags,        flags,
			     MBWMObjectPropDecorButtonUserData,     userdata,
			     NULL);

  return MB_WM_DECOR_BUTTON(button);
}

MBWMDecorButton*
mb_wm_decor_button_stock_new (MBWindowManager            *wm,
			      MBWMDecorButtonType         type,
			      MBWMDecorButtonPack         pack,
			      MBWMDecor                  *decor,
			      MBWMDecorButtonFlags        flags)
{
  MBWMObject  *button;

  button = mb_wm_object_new (MB_WM_TYPE_DECOR_BUTTON,
			     MBWMObjectPropWm,                      wm,
			     MBWMObjectPropDecorButtonType,         type,
			     MBWMObjectPropDecorButtonPack,         pack,
			     MBWMObjectPropDecor,                   decor,
			     MBWMObjectPropDecorButtonFlags,        flags,
			     NULL);

  return MB_WM_DECOR_BUTTON(button);
}

void
mb_wm_decor_button_handle_repaint (MBWMDecorButton *button)
{
  MBWMDecor * decor = button->decor;
  MBWMTheme * theme = decor->parent_client->wmref->theme;

  if (decor->parent_client == NULL)
    return;

  mb_wm_theme_paint_button (theme, button);
}
