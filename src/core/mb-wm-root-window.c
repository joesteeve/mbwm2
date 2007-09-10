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

static void
mb_wm_root_window_class_init (MBWMObjectClass *klass)
{
  MBWMRootWindowClass *rw_class;

  MBWM_MARK();

  rw_class = (MBWMRootWindowClass *)klass;
}

static void
mb_wm_root_window_destroy (MBWMObject *this)
{
  MBWMRootWindow *win = MB_WM_ROOT_WINDOW (this);

  mb_wm_object_unref (MB_WM_OBJECT (win->wm));
}

static void
mb_wm_root_window_init (MBWMObject *this, va_list vap)
{
  MBWM_MARK();
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

static void
mb_wm_root_window_init_properties (MBWMRootWindow * win);

static Bool
mb_wm_root_window_init_attributes (MBWMRootWindow * win);

MBWMRootWindow*
mb_wm_root_window_get (MBWindowManager *wm)
{
  static MBWMRootWindow * root_window = NULL;

  if (!root_window)
    {
      XSetWindowAttributes attr;

      root_window
	= MB_WM_ROOT_WINDOW (mb_wm_object_new (MB_WM_TYPE_ROOT_WINDOW,
					       NULL));

      if (!root_window)
	return root_window;

      root_window->wm = mb_wm_object_ref (MB_WM_OBJECT (wm));
      root_window->xwindow = RootWindow(wm->xdpy, wm->xscreen);

      if (!mb_wm_root_window_init_attributes (root_window))
	{
	  mb_wm_object_unref (MB_WM_OBJECT (root_window));
	  root_window = NULL;
	  return root_window;
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

static void
mb_wm_root_window_init_properties (MBWMRootWindow * win)
{
  MBWindowManager  *wm = win->wm;
  Window            rwin = win->xwindow;
  Window            hwin = win->hidden_window;

  int               num_supported = 0;
  int               num_desktops = 1;
  unsigned long     val[1];
  char             *app_name = "matchbox";

  val[0] = hwin;

  /*
   * The compliance info
   */

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


  XChangeProperty(wm->xdpy, hwin,
		  wm->atoms[MBWM_ATOM_NET_WM_NAME],
		  wm->atoms[MBWM_ATOM_UTF8_STRING],
		  8, PropModeReplace,
		  (unsigned char *)app_name, strlen(app_name)+1);

  XStoreName(wm->xdpy, hwin, app_name);

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
    wm->atoms[MBWM_ATOM_NET_SHOW_DESKTOP],
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
    0, 0
   };

  num_supported = sizeof(supported)/sizeof(Atom) - 2;

  /* Check to see if the theme supports help / accept buttons */
  if (( mb_wm_theme_supports (MBWMThemeCapsFrameMainButtonActionAccept)
	/*|| w->config->use_title == False */)
      && mb_wm_theme_supports (MBWMThemeCapsFrameMainButtonActionAccept))
    supported[num_supported++] = wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_ACCEPT];

  if (( mb_wm_theme_supports (MBWMThemeCapsFrameMainButtonActionHelp)
	/*|| w->config->use_title == False*/ )
      && mb_wm_theme_supports (MBWMThemeCapsFrameMainButtonActionHelp))
    supported[num_supported++] = wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_HELP];

  XChangeProperty(wm->xdpy, rwin, wm->atoms[MBWM_ATOM_NET_SUPPORTED],
		  XA_ATOM, 32, PropModeReplace, (unsigned char *)supported,
		  num_supported);

  /*
   * Desktop info
   */
  XChangeProperty(wm->xdpy, rwin, wm->atoms[MBWM_ATOM_NET_NUMBER_OF_DESKTOPS],
		  XA_CARDINAL, 32, PropModeReplace,
		  (unsigned char *)&num_desktops, 1);

  XChangeProperty(wm->xdpy, rwin, wm->atoms[MBWM_ATOM_NET_CURRENT_DESKTOP],
		  XA_CARDINAL, 32, PropModeReplace,
		  (unsigned char *)&num_desktops, 0);

  XSync(wm->xdpy, False);
}

static int
mb_wm_root_window_handle_message(MBWMRootWindow *win, XClientMessageEvent *e)
{
  MBWindowManager       *wm = win->wm;
  MBWindowManagerClient *c = NULL;

  if (e->message_type == wm->atoms[MBWM_ATOM_NET_ACTIVE_WINDOW])
    {
      if ((c = mb_wm_managed_client_from_xwindow(wm,
						 e->window)) != NULL)
	{
#if 0
	  /* Likely activated by a TN so start pinging if aggresive setup */
	  if (w->config->ping_aggressive
	      && c->type == MBCLIENT_TYPE_APP
	      && c != mb_wm_get_visible_main_client(wm, w))
	    ewmh_ping_client_start (c);
#endif
	}

      mb_wm_activate_client(wm, c);

      return 1;
    }
  else if (e->message_type == wm->atoms[MBWM_ATOM_NET_CLOSE_WINDOW])
    {
#if 0
      if ((c = mb_wm_managed_client_from_xwindow(wm, e->window)) != NULL)
	mb_wm_client_deliver_delete(wm, c);
#endif
      return 1;
    }
  else if (e->message_type == wm->atoms[MBWM_ATOM_WM_PROTOCOLS]
	   && e->data.l[0] == wm->atoms[MBWM_ATOM_NET_WM_PING])
    {
      if ((c = mb_wm_managed_client_from_xwindow(wm, e->data.l[1])) != NULL)
	{
#if 0
	  /* We got a response to a ping. stop pinging it now
	   * until close button is pressed again.
	   */
	  if (c->ping_handler_called)
	    {
	      int len;
	      char *buf;
	      /* aha! this was thought be be dead but has come
	       * alive again..
	       */
	      len = strlen(w->config->ping_handler) + 32;
	      buf = malloc(len);

	      if (buf)
		{
		  snprintf(buf, len-1, "%s %i %li 1",
			   w->config->ping_handler,
			   c->pid,
			   c->window);

		  fork_exec(buf);

		  free(buf);
		}
	    }

	  if (w->config->ping_aggressive)
	    {
	      if (c->pings_pending >= 0)
		c->pings_pending--;
	    }
	  else
	    {
	      /* Regular pinging, assume 1 reply and the
	       * app is alive.
	       */
	      if (c->pings_pending > 0)
		{
		  ewmh_ping_client_stop(c);
		}
	    }
#endif
	}
    }
  else if (e->message_type == wm->atoms[MBWM_ATOM_NET_WM_STATE])
    {
      if (e->data.l[1] == wm->atoms[MBWM_ATOM_NET_WM_STATE_FULLSCREEN]
	  && ((c = mb_wm_managed_client_from_xwindow(wm, e->window)) != NULL)
	  && MB_WM_IS_CLIENT_APP (c))
	{
#if 0
	  switch (e->data.l[0])
	    {
	    case _NET_WM_STATE_REMOVE:
	      if (c->flags & CLIENT_FULLSCREEN_FLAG)
		main_client_toggle_fullscreen(c);
	      break;
	    case _NET_WM_STATE_ADD:
	      if (!(c->flags & CLIENT_FULLSCREEN_FLAG))
		main_client_toggle_fullscreen(c);
	      break;
	    case _NET_WM_STATE_TOGGLE:
	      main_client_toggle_fullscreen(c);
	      break;
	    }
#endif
	}
      else if (e->data.l[1] == wm->atoms[MBWM_ATOM_NET_WM_STATE_ABOVE]
	       && ((c = mb_wm_managed_client_from_xwindow(wm, e->window)) !=
		   NULL)
	       && MB_WM_IS_CLIENT_DIALOG (c))
	{
#if 0
	  dbg("got EWMH above state change\n");
	  switch (e->data.l[0])
	    {
	    case _NET_WM_STATE_REMOVE:
	      c->flags &= ~CLIENT_HAS_ABOVE_STATE;
	      break;
	    case _NET_WM_STATE_ADD:
	      c->flags |= CLIENT_HAS_ABOVE_STATE;
	      break;
	    case _NET_WM_STATE_TOGGLE:
	      c->flags ^= CLIENT_HAS_ABOVE_STATE;
	      break;
	    }
	  wm_activate_client(c);
#endif
	}
      return 1;
    }
#if 0
  else if (e->message_type == wm->atoms[MBWM_ATOM_NET_SHOW_DESKTOP]
	   && wm_get_desktop(w) )
    {
      if (e->data.l[0] == 1)
	{ 			/* Show the desktop, if not shown */
	  if (!(w->flags & DESKTOP_RAISED_FLAG))
	    wm_toggle_desktop(w);
	}
      else
	{                 /* Hide the desktop, if shown */
	  if (w->flags & DESKTOP_RAISED_FLAG)
	    wm_toggle_desktop(w);
	}
    }
#endif
  return 0;
}
