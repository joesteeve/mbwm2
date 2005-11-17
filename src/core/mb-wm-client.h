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

#ifndef _HAVE_MB_CLIENT_H
#define _HAVE_MB_CLIENT_H

#define MBWM_CLIENT(c) ((MBWindowManagerClient*)(c)) 
#define MBWM_CLIENT_XWIN(w) (w)->window->xwindow

typedef void (*MBWindowManagerClientInitMethod) (MBWindowManagerClient *client);

typedef unsigned int MBWMClientType;


typedef enum MBWMClientStackFlags
  {
    MBWMClientStackFlagAbove,
    MBWMClientStackFlagBelow,
  }
MBWMClientStackFlags;

typedef enum MBWMClientStackHints
  {

    StackPrefAlwaysOnBottom = (1<<1),
    StackPrefAlwaysOnTop,
    StackPrefAlwaysAboveType,
    StackPrefAlwaysAboveTransient, 
    StackPrefAlwaysBelowType, 
    StackPrefAlwaysBelowClient,

    StackPrefPagable  		/* FIXME: Possible ? */
  } 
MBWMClientStackHints;

typedef struct MBWMClientStackHintValues
{
  MBWMClientType *below_types;
  int             n_below_types;

  MBWMClientType *above_types;
  int             n_above_types;


}
MBWMClientStackHintValues;

typedef enum MBWMClientLayoutHints
  {
    LayoutPrefReserveEdgeNorth = (1<<1), /* panels */
    LayoutPrefReserveEdgeSouth = (1<<2),  
    LayoutPrefReserveEdgeEast = (1<<3),
    LayoutPrefReserveEdgeWest = (1<<4),
    LayoutPrefReserveNorth = (1<<5),     /* Input wins */
    LayoutPrefReserveSouth = (1<<6), 
    LayoutPrefReserveEast = (1<<7),
    LayoutPrefReserveWest = (1<<8),
    LayoutPrefGrowToFreeSpace  = (1<<9), /* Free space left by above   */
    LayoutPrefFullscreen  = (1<<10),      /* Fullscreen and desktop wins */
    LayoutPrefPositionFree = (1<<11),    /* Dialog, panel in titlebar */
    LayoutPrefVisible = (1<<12),         /* Flag is toggled by stacking */
  } 
MBWMClientLayoutHints;

typedef enum MBWMClientReqGeomType
  {
    MBWMClientReqGeomDontCommit         = (1 << 1),
    MBWMClientReqGeomIsViaConfigureReq  = (1 << 2),
    MBWMClientReqGeomIsViaUserAction    = (1 << 3),
    MBWMClientReqGeomIsViaLayoutManager = (1 << 4),
    MBWMClientReqGeomForced             = (1 << 5)
  }
MBWMClientReqGeomType;

/* Methods */

typedef  void (*MBWMClientNewMethod) (MBWindowManager       *wm, 
				      MBWMWindow            *win);

typedef  void (*MBWMClientInitMethod) (MBWindowManager       *wm, 
				       MBWindowManagerClient *client,
				       MBWMWindow            *win);

typedef  void (*MBWMClientRealizeMethod) (MBWindowManagerClient *client);

typedef  void (*MBWMClientDestroyMethod) (MBWindowManagerClient *client);

typedef  Bool (*MBWMClientGeometryMethod) (MBWindowManagerClient *client,
					   MBGeometry            *new_geometry,
					   MBWMClientReqGeomType  flags);

typedef  void (*MBWMClientStackMethod) (MBWindowManagerClient *client,
					int                    flags);

typedef  void (*MBWMClientShowMethod) (MBWindowManagerClient *client);

typedef  void (*MBWMClientHideMethod) (MBWindowManagerClient *client);

typedef  void (*MBWMClientSyncMethod) (MBWindowManagerClient *client);


struct MBWindowManagerClient
{
  int                          type;
  /* ### public ### */

  MBWindowManager             *wmref;
  char                        *name;
  MBWMWindow                  *window;
  Window                       xwin_frame;
  unsigned long                stacking_hints;

  MBWMClientLayoutHints        layout_hints;

  MBWindowManagerClient       *stacked_above, *stacked_below;
  MBWindowManagerClient       *next_focused_client;

  MBGeometry frame_geometry;  /* FIXME: in ->priv ? */

  /* ### Methods ### */

  MBWMClientNewMethod          new;
  MBWMClientInitMethod         init;
  MBWMClientRealizeMethod      realize;	 /* create dpy resources / reparent */
  MBWMClientDestroyMethod      destroy;
  MBWMClientGeometryMethod     geometry; /* requests a gemetry change */
  MBWMClientStackMethod        stack;    /* positions win in stack */
  MBWMClientShowMethod         show;
  MBWMClientHideMethod         hide;
  MBWMClientSyncMethod         sync;     /* sync internal changes to display */

  /* focus, coverage */

#if 0
       new() 
     - init()                   
     - request_geometry()  --move resize or request_position, request_size */
     - realize()          /* would reparent */
     - get_coverage ()
     - unrealize
     - map ()
     - unamp ()
     - update_stack_position ()  /* just raise() or stack() ? */
     - focus() ?
     - paint ()
     - destroy ()

#endif

  /* ### Private ? ### */

  MBWindowManagerClientPriv   *priv;
};

#define mb_wm_client_frame_east_width(c) \
         ((c)->window->geometry.x) - (c)->frame_geometry.x)
#define mb_wm_client_frame_west_width(c) \
         (((c)->frame_geometry.x + (c)->frame_geometry.width) \
          - ((c)->frame_geometry.x + (c)->frame_geometry.width))

#define mb_wm_client_frame_size_north(c) \
         ((c)->window->geometry.y) - (c)->frame_geometry.y)



MBWMWindow*
mb_wm_client_window_new (MBWindowManager *wm, Window window);

MBWindowManagerClient*
mb_wm_client_new (MBWindowManager *wm, MBWMWindow *win);

void
mb_wm_client_init (MBWindowManager             *wm, 
		   MBWindowManagerClient       *client,
                   MBWMWindow *win);

void
mb_wm_client_realize (MBWindowManagerClient *client);

void
mb_wm_client_stack (MBWindowManagerClient *client,
  int                    flags);
void
mb_wm_client_show (MBWindowManagerClient *client);

void
mb_wm_client_hide (MBWindowManagerClient *client);

void
mb_wm_client_display_sync (MBWindowManagerClient *client);

Bool
mb_wm_client_is_realized (MBWindowManagerClient *client);

Bool
mb_wm_client_request_geometry (MBWindowManagerClient *client,
                               MBGeometry            *new_geometry,
                               MBWMClientReqGeomType  flags);
void
mb_wm_client_destroy (MBWindowManagerClient *client);

Bool
mb_wm_client_needs_geometry_sync (MBWindowManagerClient *client);

Bool
mb_wm_client_needs_sync (MBWindowManagerClient *client);

Bool
mb_wm_client_is_mapped (MBWindowManagerClient *client);

void
mb_wm_client_get_coverage (MBWindowManagerClient *client,
                           MBGeometry            *coverage);

MBWMClientLayoutHints
mb_wm_client_get_layout_hints (MBWindowManagerClient *client);

void
mb_wm_client_set_layout_hints (MBWindowManagerClient *client,
                               MBWMClientLayoutHints  hints);

void
mb_wm_client_set_layout_hint (MBWindowManagerClient *client,
			      MBWMClientLayoutHints  hint,
                              Bool                   state);

void
mb_wm_client_stacking_mark_dirty (MBWindowManagerClient *client);

void
mb_wm_client_geometry_mark_dirty (MBWindowManagerClient *client);

void
mb_wm_client_visibility_mark_dirty (MBWindowManagerClient *client);

#endif
