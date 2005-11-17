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
#define MBWM_WINDOW_PROP_WM_HINTS   (1<<6)
#define MBWM_WINDOW_PROP_RGBA_ICON  (1<<7)
#define MBWM_WINDOW_PROP_PID        (1<<10)
#define MBWM_WINDOW_PROP_PROTOS     (1<<11)
#define MBWM_WINDOW_PROP_TRANSIENCY (1<<12)
#define MBWM_WINDOW_PROP_STATE      (1<<13)
#define MBWM_WINDOW_PROP_NET_STATE  (1<<14)
#define MBWM_WINDOW_PROP_STARTUP_ID (1<<15)

#define MBWM_WINDOW_PROP_ALL        (0xffffffff)

typedef enum MBWMWindowEWMHState
  {
    MBWMWindowEWMHStateModal            = (1<<1),
    MBWMWindowEWMHStateSticky           = (1<<2),
    MBWMWindowEWMHStateMaximisedVert    = (1<<3),
    MBWMWindowEWMHStateMaximisedHorz    = (1<<4),
    MBWMWindowEWMHStateShaded           = (1<<5),
    MBWMWindowEWMHStateSkipTaskbar      = (1<<6),
    MBWMWindowEWMHStateSkipPager        = (1<<7),
    MBWMWindowEWMHStateHidden           = (1<<8),
    MBWMWindowEWMHStateFullscreen       = (1<<9),
    MBWMWindowEWMHStateAbove            = (1<<10),
    MBWMWindowEWMHStateBelow            = (1<<11),
    MBWMWindowEWMHStateDemandsAttention = (1<<12)
  }
MBWMWindowEWMHState;

struct MBWMWindow
{
  MBGeometry          geometry;
  unsigned int        depth;
  char               *name;
  Window              xwindow;

  Atom                net_type;
  Bool                want_key_input;
  Window              xwin_group;
  Pixmap              icon_pixmap, icon_pixmap_mask;

  /* WithdrawnState 0, NormalState 1, IconicState 3 */
  int                 initial_state ;

  MBWMWindowEWMHState ewmh_state;
  Window              xwin_transient_for;

};


MBWMWindow*
mb_wm_client_window_new (MBWindowManager *wm, Window xwin);

Bool
mb_wm_window_sync_properties (MBWindowManager *wm,
			      MBWMWindow      *win,
			      unsigned long    props_req);

#endif
