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

#define MB_WM_CLIENT(c) ((MBWindowManagerClient*)(c))
#define MB_WM_CLIENT_CLASS(c) ((MBWindowManagerClientClass*)(c))
#define MB_WM_TYPE_CLIENT (mb_wm_client_class_type ())
#define MB_WM_CLIENT_XWIN(w) (w)->window->xwindow

typedef void (*MBWindowManagerClientInitMethod) (MBWindowManagerClient *client);

typedef unsigned int MBWMClientType;

/* Clients hint to what stacking layer they exist in. By default all
 * transients to that client will also be stacked there.
 */
typedef enum MBWMStackLayerType
{
  MBWMStackLayerUnknown     = 0, /* Transients */
  MBWMStackLayerBottom       , 	 /* Desktop window */
  MBWMStackLayerBottomMid    ,	 /* Panels */
  MBWMStackLayerMid          ,	 /* Apps */
  MBWMStackLayerTopMid       ,	 /* Trans for root dialogs */
  MBWMStackLayerTop          ,	 /* Something else ? */
  N_MBWMStackLayerTypes
}
MBWMStackLayerType;

/* Clients can also hint to as how they would like to be managed by the
 * layout manager.
 */
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
				      MBWMClientWindow      *win);

typedef  void (*MBWMClientInitMethod) (MBWindowManager       *wm,
				       MBWindowManagerClient *client,
				       MBWMClientWindow      *win);

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


struct MBWindowManagerClientClass
{
  MBWMObjectClass              parent;

  MBWMClientRealizeMethod      realize;	 /* create dpy resources / reparent */
  MBWMClientGeometryMethod     geometry; /* requests a gemetry change */
  MBWMClientStackMethod        stack;    /* positions win in stack */
  MBWMClientShowMethod         show;
  MBWMClientHideMethod         hide;
  MBWMClientSyncMethod         sync;     /* sync internal changes to display */
};

struct MBWindowManagerClient
{
  MBWMObject                   parent;
  /* ### public ### */

  MBWindowManager             *wmref;
  char                        *name;
  MBWMClientWindow            *window;
  Window                       xwin_frame;
  MBWMStackLayerType           stacking_layer;
  unsigned long                stacking_hints;

  MBWMClientLayoutHints        layout_hints;

  MBWindowManagerClient       *stacked_above, *stacked_below;
  MBWindowManagerClient       *next_focused_client;

  MBGeometry frame_geometry;  /* FIXME: in ->priv ? */
  MBWMList                    *decor;
  MBWMList                    *transients;
  MBWindowManagerClient       *transient_for;

  int                          skip_unmaps;

  int                          pings_pending;
  int                          pings_sent;

  Bool                         not_responding;
  Bool                         kill_attempted;

  /* To add focus, coverage */

  /* ### Private ### */

  MBWindowManagerClientPriv   *priv;
  unsigned long                sig_prop_change_id;
};

#define mb_wm_client_frame_west_width(c) \
         ((c)->window->geometry.x - (c)->frame_geometry.x)
#define mb_wm_client_frame_east_width(c) \
         (((c)->frame_geometry.x + (c)->frame_geometry.width) \
          - ((c)->window->geometry.x + (c)->window->geometry.width))
#define mb_wm_client_frame_east_x(c) \
          ((c)->window->geometry.x + (c)->window->geometry.width)
#define mb_wm_client_frame_north_height(c) \
         ((c)->window->geometry.y - (c)->frame_geometry.y)
#define mb_wm_client_frame_south_y(c) \
         ((c)->window->geometry.y + (c)->window->geometry.height)
#define mb_wm_client_frame_south_height(c) \
         ( ((c)->frame_geometry.y + (c)->frame_geometry.height) \
          - ((c)->window->geometry.y + (c)->window->geometry.height) )

MBWMClientWindow*
mb_wm_client_window_new (MBWindowManager *wm, Window window);

MBWindowManagerClient*
mb_wm_client_new (MBWindowManager *wm, MBWMClientWindow *win);

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

Bool
mb_wm_client_needs_geometry_sync (MBWindowManagerClient *client);

Bool
mb_wm_client_needs_fullscreen_sync (MBWindowManagerClient *client);

Bool
mb_wm_client_needs_decor_sync (MBWindowManagerClient *client);

Bool
mb_wm_client_needs_synthetic_config_event (MBWindowManagerClient *client);

void 				/* FIXME: rename */
mb_wm_client_synthetic_config_event_queue (MBWindowManagerClient *client);

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
mb_wm_client_fullscreen_mark_dirty (MBWindowManagerClient *client);

void
mb_wm_client_geometry_mark_dirty (MBWindowManagerClient *client);

void
mb_wm_client_visibility_mark_dirty (MBWindowManagerClient *client);

void
mb_wm_client_add_transient (MBWindowManagerClient *client,
			    MBWindowManagerClient *transient);

void
mb_wm_client_remove_transient (MBWindowManagerClient *client,
			       MBWindowManagerClient *transient);

const MBWMList*
mb_wm_client_get_transients (MBWindowManagerClient *client);

MBWindowManagerClient*
mb_wm_client_get_transient_for (MBWindowManagerClient *client);

const char*
mb_wm_client_get_name (MBWindowManagerClient *client);

void
mb_wm_client_deliver_delete (MBWindowManagerClient *client);

void
mb_wm_client_deliver_wm_protocol (MBWindowManagerClient *client,
				  Atom protocol);

void
mb_wm_client_shutdown (MBWindowManagerClient *client);

void
mb_wm_client_set_state (MBWindowManagerClient *client,
			Atom state,
			MBWMClientWindowStateChange state_op);

#endif
