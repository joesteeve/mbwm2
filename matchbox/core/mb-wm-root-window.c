/*
 *  Matchbox Window Manager II - A lightweight window manager not for the
 *                               desktop.
 *
 *  Authored By Tomas Frydrych <tf@o-hand.com>
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

#include "mb-wm.h"

#include "../theme-engines/mb-wm-theme.h"
#include "../client-types/mb-wm-client-dialog.h"
#include "../client-types/mb-wm-client-app.h"

#include <X11/Xmd.h>

static void
mb_wm_root_window_class_init (MBWMObjectClass *klass)
{
  MBWMRootWindowClass *rw_class;

  MBWM_MARK();

  rw_class = (MBWMRootWindowClass *)klass;

#if MBWM_WANT_DEBUG
  klass->klass_name = "MBWMRootWindow";
#endif
}

static void
mb_wm_root_window_destroy (MBWMObject *this)
{
}

static Bool
mb_wm_root_window_init_attributes (MBWMRootWindow * win);

static void
mb_wm_root_window_init_properties (MBWMRootWindow * win);

static int
mb_wm_root_window_init (MBWMObject *this, va_list vap)
{
  MBWMRootWindow       *root_window = MB_WM_ROOT_WINDOW (this);
  MBWindowManager      *wm;
  MBWMObjectProp        prop;
  XSetWindowAttributes  attr;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      if (prop == MBWMObjectPropWm)
	{
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	}
      else
	MBWMO_PROP_EAT (vap, prop);

      prop = va_arg(vap, MBWMObjectProp);
    }

  root_window->wm = wm;
  root_window->xwindow = RootWindow(wm->xdpy, wm->xscreen);

  if (!mb_wm_root_window_init_attributes (root_window))
    {
      MBWM_DBG ("Failed to initialize root window attributes.");
      abort ();
    }

  attr.override_redirect = True;
  root_window->hidden_window = XCreateWindow(wm->xdpy,
					     root_window->xwindow,
					     -200, -200, 5, 5, 0,
					     CopyFromParent,
					     CopyFromParent,
					     CopyFromParent,
					     CWOverrideRedirect, &attr);

  mb_wm_root_window_init_properties (root_window);

  return 1;
}

int
mb_wm_root_window_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMRootWindowClass),
	sizeof (MBWMRootWindow),
	mb_wm_root_window_init,
	mb_wm_root_window_destroy,
	mb_wm_root_window_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

MBWMRootWindow*
mb_wm_root_window_get (MBWindowManager *wm)
{
  static MBWMRootWindow * root_window = NULL;

  if (!root_window)
    {
      root_window
	= MB_WM_ROOT_WINDOW (mb_wm_object_new (MB_WM_TYPE_ROOT_WINDOW,
					       MBWMObjectPropWm, wm,
					       NULL));
    }
  else
    mb_wm_object_ref (MB_WM_OBJECT (root_window));

  return root_window;
}

static Bool
mb_wm_root_window_init_attributes (MBWMRootWindow * win)
{
  XSetWindowAttributes  sattr;
  MBWindowManager      *wm = win->wm;

  sattr.event_mask =  SubstructureRedirectMask
                      |SubstructureNotifyMask
                      |StructureNotifyMask
                      |PropertyChangeMask;

  mb_wm_util_trap_x_errors();

  XChangeWindowAttributes(wm->xdpy, win->xwindow, CWEventMask, &sattr);

  XSync(wm->xdpy, False);

  if (mb_wm_util_untrap_x_errors())
    {
      /* FIXME: Error codes */
      mb_wm_util_fatal_error("Unable to manage display - "
			     "another window manager already active?");
      return False;
    }

  XSelectInput(wm->xdpy, win->xwindow, sattr.event_mask);

  return True;
}

void
mb_wm_root_window_update_supported_props (MBWMRootWindow *win)
{
  MBWindowManager  *wm = win->wm;
  Window            rwin = win->xwindow;
  long            num_supported = 0;

  /*
   * Supported info
   */
  Atom supported[] = {
    wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_TOOLBAR],
    wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DOCK],
    wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DIALOG],
    wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DESKTOP],
    wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_SPLASH],
    wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_MENU],
    wm->atoms[MBWM_ATOM_NET_WM_STATE],
    wm->atoms[MBWM_ATOM_NET_WM_STATE_FULLSCREEN],
    wm->atoms[MBWM_ATOM_NET_WM_STATE_MODAL],
    wm->atoms[MBWM_ATOM_NET_SUPPORTED],
    wm->atoms[MBWM_ATOM_NET_CLIENT_LIST],
    wm->atoms[MBWM_ATOM_NET_NUMBER_OF_DESKTOPS],
    wm->atoms[MBWM_ATOM_NET_ACTIVE_WINDOW],
    wm->atoms[MBWM_ATOM_NET_SUPPORTING_WM_CHECK],
    wm->atoms[MBWM_ATOM_NET_CLOSE_WINDOW],
    wm->atoms[MBWM_ATOM_NET_CURRENT_DESKTOP],
    wm->atoms[MBWM_ATOM_NET_CLIENT_LIST_STACKING],
    wm->atoms[MBWM_ATOM_NET_SHOWING_DESKTOP],
    wm->atoms[MBWM_ATOM_NET_WM_NAME],
    wm->atoms[MBWM_ATOM_NET_WM_ICON],
    wm->atoms[MBWM_ATOM_NET_WM_ALLOWED_ACTIONS],
    wm->atoms[MBWM_ATOM_NET_WM_ACTION_MOVE],
    wm->atoms[MBWM_ATOM_NET_WM_ACTION_FULLSCREEN],
    wm->atoms[MBWM_ATOM_NET_WM_ACTION_CLOSE],
    wm->atoms[MBWM_ATOM_NET_STARTUP_ID],
    wm->atoms[MBWM_ATOM_NET_WM_PING],
    wm->atoms[MBWM_ATOM_NET_WORKAREA],
    wm->atoms[MBWM_ATOM_NET_DESKTOP_GEOMETRY],
    wm->atoms[MBWM_ATOM_NET_WM_PING],
    wm->atoms[MBWM_ATOM_NET_WM_PID],
    wm->atoms[MBWM_ATOM_CM_TRANSLUCENCY],
    wm->atoms[MBWM_ATOM_NET_WM_FULL_PLACEMENT],
    wm->atoms[MBWM_ATOM_NET_FRAME_EXTENTS],
    0, 0, 0
   };

  num_supported = sizeof(supported)/sizeof(Atom) - 3;

  /* Check to see if the theme supports help / accept buttons */
  if (wm->theme)
    {
      if (mb_wm_theme_supports (wm->theme,
				MBWMThemeCapsFrameMainButtonActionAccept))
	supported[num_supported++]=wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_ACCEPT];

      if (mb_wm_theme_supports (wm->theme,
				MBWMThemeCapsFrameMainButtonActionHelp))
	supported[num_supported++] = wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_HELP];

      if (mb_wm_theme_supports (wm->theme,
				MBWMThemeCapsFrameMainButtonActionCustom))
	supported[num_supported++]=wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_CUSTOM];
    }

  XChangeProperty(wm->xdpy, rwin, wm->atoms[MBWM_ATOM_NET_SUPPORTED],
		  XA_ATOM, 32, PropModeReplace, (unsigned char *)supported,
		  num_supported);

  if (wm->theme)
    {
      if (wm->theme->path)
	XChangeProperty(wm->xdpy, rwin, wm->atoms[MBWM_ATOM_MB_THEME],
			XA_STRING, 8, PropModeReplace,
			(unsigned char *)wm->theme->path,
			strlen (wm->theme->path) + 1);
      else
	XDeleteProperty (wm->xdpy, rwin, wm->atoms[MBWM_ATOM_MB_THEME]);
    }
}

static void
mb_wm_root_window_init_properties (MBWMRootWindow * win)
{
  MBWindowManager  *wm = win->wm;
  Window            rwin = win->xwindow;
  Window            hwin = win->hidden_window;

  long            card32;
  unsigned long     val[2];
  char             *app_name = "matchbox";

  val[0] = hwin;

  /* Window name */
  XChangeProperty(wm->xdpy, hwin,
		  wm->atoms[MBWM_ATOM_NET_WM_NAME],
		  wm->atoms[MBWM_ATOM_UTF8_STRING],
		  8, PropModeReplace,
		  (unsigned char *)app_name, strlen(app_name)+1);

  XStoreName(wm->xdpy, hwin, app_name);

  /* Crack Needed to stop gnome session hanging ? */
  XChangeProperty(wm->xdpy, rwin,
		  wm->atoms[MBWM_ATOM_WIN_SUPPORTING_WM_CHECK],
		  XA_WINDOW, 32, PropModeReplace, (unsigned char *)val,
		  1);

  XChangeProperty(wm->xdpy, hwin,
		  wm->atoms[MBWM_ATOM_WIN_SUPPORTING_WM_CHECK],
		  XA_WINDOW, 32, PropModeReplace,
		  (unsigned char *)val, 1);

  /* Correct way of doing it */
  XChangeProperty(wm->xdpy, rwin,
		  wm->atoms[MBWM_ATOM_NET_SUPPORTING_WM_CHECK],
		  XA_WINDOW, 32, PropModeReplace, (unsigned char *)val,
		  1);

  XChangeProperty(wm->xdpy, hwin,
		  wm->atoms[MBWM_ATOM_NET_SUPPORTING_WM_CHECK],
		  XA_WINDOW, 32, PropModeReplace,
		  (unsigned char *)val, 1);

  mb_wm_root_window_update_supported_props (win);

  /*
   * Desktop info
   */
  card32 = 1;
  XChangeProperty(wm->xdpy, rwin, wm->atoms[MBWM_ATOM_NET_NUMBER_OF_DESKTOPS],
		  XA_CARDINAL, 32, PropModeReplace,
		  (unsigned char *)&card32, 1);

  --card32;
  XChangeProperty(wm->xdpy, rwin, wm->atoms[MBWM_ATOM_NET_CURRENT_DESKTOP],
		  XA_CARDINAL, 32, PropModeReplace,
		  (unsigned char *)&card32, 1);

  XChangeProperty(wm->xdpy, rwin, wm->atoms[MBWM_ATOM_NET_SHOWING_DESKTOP],
		  XA_CARDINAL, 32, PropModeReplace,
		  (unsigned char *)&card32, 1);

  val[0] = 0;
  val[1] = 0;

  XChangeProperty(wm->xdpy, rwin, wm->atoms[MBWM_ATOM_NET_DESKTOP_VIEWPORT],
		  XA_CARDINAL, 32, PropModeReplace,
		  (unsigned char *)&val[0], 2);

  XSync(wm->xdpy, False);
}

int
mb_wm_root_window_handle_message (MBWMRootWindow *win, XClientMessageEvent *e)
{
  MBWindowManager       *wm = win->wm;
  MBWindowManagerClient *c = NULL;

  if (e->message_type == wm->atoms[MBWM_ATOM_NET_ACTIVE_WINDOW])
    {
      Window xwin = e->window;

      if ((c = mb_wm_managed_client_from_xwindow (wm, xwin)) != NULL)
	mb_wm_activate_client (wm, c);

      return 1;
    }
  else if (e->message_type == wm->atoms[MBWM_ATOM_NET_CLOSE_WINDOW])
    {
      if ((c = mb_wm_managed_client_from_xwindow(wm, e->window)) != NULL)
	mb_wm_client_deliver_delete(c);

      return 1;
    }
  else if (e->message_type == wm->atoms[MBWM_ATOM_WM_PROTOCOLS]
	   && e->data.l[0] == wm->atoms[MBWM_ATOM_NET_WM_PING])
    {
      if ((c = mb_wm_managed_client_from_xwindow(wm, e->data.l[2])) != NULL)
	mb_wm_handle_ping_reply (wm, c);
      return 1;
    }
  else if (e->message_type == wm->atoms[MBWM_ATOM_NET_WM_STATE])
    {
      MBWMClientWindowStateChange state_op = 0;

      if (e->data.l[0] == 0)
	state_op = MBWMClientWindowStateChangeRemove;
      else if (e->data.l[0] == 1)
	state_op = MBWMClientWindowStateChangeAdd;
      else if (e->data.l[0] == 2)
	state_op = MBWMClientWindowStateChangeToggle;

      if (e->data.l[1] == wm->atoms[MBWM_ATOM_NET_WM_STATE_FULLSCREEN]
	  && ((c = mb_wm_managed_client_from_xwindow(wm, e->window)) != NULL)
	  && MB_WM_IS_CLIENT_APP (c))
	{
	  mb_wm_client_set_state (c,
				  MBWM_ATOM_NET_WM_STATE_FULLSCREEN,
				  state_op);
	}
      else if (e->data.l[1] == wm->atoms[MBWM_ATOM_NET_WM_STATE_ABOVE]
	       && ((c = mb_wm_managed_client_from_xwindow(wm, e->window)) !=
		   NULL)
	       && MB_WM_IS_CLIENT_DIALOG (c))
	{
	  mb_wm_client_set_state (c,
				  MBWM_ATOM_NET_WM_STATE_ABOVE,
				  state_op);
	}
      return 1;
    }

  else if (e->message_type == wm->atoms[MBWM_ATOM_WM_CHANGE_STATE])
    {
      switch (e->data.l[0])
	{
	case IconicState:
	  if ((c = mb_wm_managed_client_from_xwindow (wm, e->window)))
	    mb_wm_client_iconize (c);

	default:
	  MBWM_DBG ("Unhandled value %d for WM_CHANGE_STATE ClientMessage",
		    e->data.l[0]);
	}
    }
  else if (e->message_type == wm->atoms[MBWM_ATOM_NET_SHOWING_DESKTOP])
    {
      mb_wm_handle_show_desktop (wm, e->data.l[0]);
      return 1;
    }
  else if (e->message_type == wm->atoms[MBWM_ATOM_NET_CURRENT_DESKTOP])
    {
      mb_wm_select_desktop (wm, e->data.l[0]);
      return 1;
    }
  else if (e->message_type == wm->atoms[MBWM_ATOM_MB_COMMAND])
    {
       switch (e->data.l[0])
	 {
	 case MB_CMD_EXIT:
	   exit(0);
	 case MB_CMD_NEXT:
	   mb_wm_cycle_apps (wm, False);
	   break;
	 case MB_CMD_PREV:
	   mb_wm_cycle_apps (wm, True);
	   break;
	 case MB_CMD_DESKTOP:
	   mb_wm_toggle_desktop (wm);
	   break;
#if ENABLE_COMPOSITE
	 case MB_CMD_COMPOSITE:
	   if (mb_wm_compositing_enabled (wm))
	     mb_wm_compositing_off (wm);
	   else
	     mb_wm_compositing_on (wm);
	   break;
#endif
	 default:
	   /*FIXME -- not implemented yet */
	 case MB_CMB_KEYS_RELOAD:
	   ;
	 }
    }

  return 0;
}
