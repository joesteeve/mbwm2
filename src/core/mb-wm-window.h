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

#ifndef _HAVE_MB_WM_WINDOW_H
#define _HAVE_MB_WM_WINDOW_H

#define MBWM_WINDOW_PROP_WIN_TYPE   (1<<1)
#define MBWM_WINDOW_PROP_GEOMETRY   (1<<2)
#define MBWM_WINDOW_PROP_ATTR       (1<<3)
#define MBWM_WINDOW_PROP_NAME       (1<<4)
#define MBWM_WINDOW_PROP_SIZE_HINTS (1<<5)
#define MBWM_WINDOW_PROP_ICON       (1<<6)
#define MBWM_WINDOW_PROP_URGENCY    (1<<7)
#define MBWM_WINDOW_PROP_GROUP      (1<<8)
#define MBWM_WINDOW_PROP_PID        (1<<9)
#define MBWM_WINDOW_PROP_PROTOS     (1<<10)
#define MBWM_WINDOW_PROP_TRANSIENCY (1<<11)
#define MBWM_WINDOW_PROP_STATE      (1<<12)
#define MBWM_WINDOW_PROP_NET_STATE  (1<<13)
#define MBWM_WINDOW_PROP_STARTUP_ID (1<<14)

#define MBWM_WINDOW_PROP_ALL        (0xffffffff)

struct MBWMWindow
{
  MBGeometry   geometry;
  unsigned int depth;
  char        *name;
  Window       xwindow;

  Atom         net_type;
};


MBWMWindow*
mb_wm_client_window_new (MBWindowManager *wm, Window xwin);

Bool
mb_wm_window_sync_properties (MBWindowManager *wm,
			      MBWMWindow      *win,
			      unsigned long    props_req);

#endif
