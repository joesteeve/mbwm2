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

/* FIXME: below limits to 32 props */

/* When a property changes
 *    - window updates its internal values
 *    - somehow signals client object to process with what changed.
 */

#define MBWM_WINDOW_PROP_WIN_TYPE       (1<<0)
#define MBWM_WINDOW_PROP_GEOMETRY       (1<<1)
#define MBWM_WINDOW_PROP_ATTR           (1<<2)
#define MBWM_WINDOW_PROP_NAME           (1<<3)
#define MBWM_WINDOW_PROP_SIZE_HINTS     (1<<4)
#define MBWM_WINDOW_PROP_WM_HINTS       (1<<5)
#define MBWM_WINDOW_PROP_NET_ICON       (1<<6)
#define MBWM_WINDOW_PROP_NET_PID        (1<<7)
#define MBWM_WINDOW_PROP_PROTOS         (1<<8)
#define MBWM_WINDOW_PROP_TRANSIENCY     (1<<9)
#define MBWM_WINDOW_PROP_STATE          (1<<10)
#define MBWM_WINDOW_PROP_NET_STATE      (1<<11)
#define MBWM_WINDOW_PROP_STARTUP_ID     (1<<12)
#define MBWM_WINDOW_PROP_CLIENT_MACHINE (1<<13)

#define MBWM_WINDOW_PROP_ALL        (0xffffffff)

typedef enum MBWMWindowEWMHState
  {
    MBWMWindowEWMHStateModal            = (1<<0),
    MBWMWindowEWMHStateSticky           = (1<<1),
    MBWMWindowEWMHStateMaximisedVert    = (1<<2),
    MBWMWindowEWMHStateMaximisedHorz    = (1<<3),
    MBWMWindowEWMHStateShaded           = (1<<4),
    MBWMWindowEWMHStateSkipTaskbar      = (1<<5),
    MBWMWindowEWMHStateSkipPager        = (1<<6),
    MBWMWindowEWMHStateHidden           = (1<<7),
    MBWMWindowEWMHStateFullscreen       = (1<<8),
    MBWMWindowEWMHStateAbove            = (1<<9),
    MBWMWindowEWMHStateBelow            = (1<<10),
    MBWMWindowEWMHStateDemandsAttention = (1<<11)
  }
MBWMWindowEWMHState;

typedef enum MBWMWindowProtos
  {
    MBWMWindowProtosFocus         = (1<<0),
    MBWMWindowProtosDelete        = (1<<1),
    MBWMWindowProtosContextHelp   = (1<<2),
    MBWMWindowProtosContextAccept = (1<<3),
    MBWMWindowProtosContextCustom = (1<<4),
    MBWMWindowProtosPing          = (1<<5),
    MBWMWindowProtosSyncRequest   = (1<<6),
  }
MBWMWindowProtos;

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

  MBWMWindowProtos    protos;
  pid_t               pid;
  char               *machine;

  MBWMList           *icons;
};


MBWMWindow*
mb_wm_client_window_new (MBWindowManager *wm, Window xwin);

void
mb_wm_client_window_free (MBWMWindow *win);

Bool
mb_wm_window_sync_properties (MBWindowManager *wm,
			      MBWMWindow      *win,
			      unsigned long    props_req);

#endif
