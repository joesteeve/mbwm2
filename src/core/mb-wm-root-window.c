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

enum {
  COOKIE_WIN_TYPE = 0,

  N_COOKIES
};

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
mb_wm_root_window_init (MBWMObject *this)
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

MBWMRootWindow*
mb_wm_root_window_new (MBWindowManager *wm, Window xwin)
{
  MBWMRootWindow           *root_window;

  root_window
    = MB_WM_ROOT_WINDOW (mb_wm_object_new (MB_WM_TYPE_ROOT_WINDOW));

  if (!root_window)
    return root_window;

  root_window->xwindow = xwin;
  root_window->wm = (MBWindowManager*)mb_wm_object_ref (MB_WM_OBJECT (wm));

  return root_window;
}

Bool
mb_wm_root_window_sync_properties (MBWindowManager  *wm,
				   MBWMRootWindow   *win,
				   unsigned long     props_req)
{
  MBWMCookie       cookies[N_COOKIES];
  Atom             actual_type_return, *result_atom = NULL;
  int              actual_format_return;
  unsigned long    nitems_return;
  unsigned long    bytes_after_return;
  unsigned int     foo;
  int              x_error_code;
  Window           xwin;
  int              changes = 0;

  xwin = win->xwindow;

  /* bundle all pending requests to server and wait for replys */
  XSync(wm->xdpy, False);


  if (changes)
    mb_wm_object_signal_emit (MB_WM_OBJECT (win), changes);

 abort:

  return True;
}

