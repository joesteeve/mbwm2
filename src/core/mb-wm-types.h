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

#ifndef _HAVE_MB_WM_TYPES_H
#define _HAVE_MB_WM_TYPES_H

typedef struct MBWMFuncInfo
{
  void *func;
  void *data;
  void *userdata;
  unsigned long signal;
  unsigned long id;
} MBWMFuncInfo;

typedef struct MBGeometry
{
  int          x,y;
  unsigned int width,height;

} MBGeometry;

typedef struct MBWMList MBWMList;

typedef void (*MBWMListForEachCB) (void *data, void *userdata);

struct MBWMList
{
  MBWMList *next, *prev;
  void *data;
};

typedef struct MBWMClientWindowAttributes /* Needs to be sorted */
{
  Visual *visual;
  Window root;
  int class;
  int bit_gravity;
  int win_gravity;
  int backing_store;
  unsigned long backing_planes;
  unsigned long backing_pixel;
  Bool save_under;
  Colormap colormap;
  Bool map_installed;
  int map_state;
  long all_event_masks;
  long your_event_mask;
  long do_not_propagate_mask;
  Bool override_redirect;

} MBWMClientWindowAttributes ;

typedef struct MBWMRgbaIcon
{
  int width;
  int height;
  unsigned long *pixels;
} MBWMRgbaIcon;

typedef struct MBWindowManager             MBWindowManager;
typedef struct MBWindowManagerClient       MBWindowManagerClient;
typedef struct MBWindowManagerClientClass  MBWindowManagerClientClass;
typedef struct MBWindowManagerClientPriv   MBWindowManagerClientPriv;
typedef struct MBWMClientWindow            MBWMClientWindow;
typedef struct MBWMClientWindowClass       MBWMClientWindowClass;
typedef struct MBWMTheme                   MBWMTheme;
typedef struct MBWMThemeClass              MBWMThemeClass;
typedef struct MBWMThemeCairo              MBWMThemeCairo;
typedef struct MBWMThemeCairoClass         MBWMThemeCairoClass;
typedef struct MBWMThemePng                MBWMThemePng;
typedef struct MBWMThemePngClass           MBWMThemePngClass;
typedef struct MBWMThemeSimple             MBWMThemeSimple;
typedef struct MBWMThemeSimpleClass        MBWMThemeSimpleClass;
typedef enum   MBWMThemeCaps               MBWMThemeCaps;
typedef struct MBWMDecor                   MBWMDecor;
typedef struct MBWMDecorClass              MBWMDecorClass;
typedef struct MBWMDecorButton             MBWMDecorButton;
typedef struct MBWMDecorButtonClass        MBWMDecorButtonClass;
typedef struct MBWMLayout                  MBWMLayout;
typedef struct MBWMLayoutClass             MBWMLayoutClass;
typedef struct MBWMMainContext             MBWMMainContext;
typedef struct MBWMMainContextClass        MBWMMainContextClass;


typedef enum MBWMClientType
{
  MBWMClientTypeApp     = (1 << 0),
  MBWMClientTypeDialog  = (1 << 1),
  MBWMClientTypePanel   = (1 << 2),
  MBWMClientTypeDesktop = (1 << 3),
  MBWMClientTypeInput   = (1 << 4),

  MBWMClientTypeLast    = MBWMClientTypeInput,
} MBWMClientType;

typedef unsigned long MBWMCookie;

typedef enum MBWMAtom
{
  /* ICCCM */

  MBWM_ATOM_WM_NAME = 0,
  MBWM_ATOM_WM_STATE,
  MBWM_ATOM_WM_HINTS,
  MBWM_ATOM_WM_CHANGE_STATE,
  MBWM_ATOM_WM_PROTOCOLS,
  MBWM_ATOM_WM_DELETE_WINDOW,
  MBWM_ATOM_WM_COLORMAP_WINDOWS,
  MBWM_ATOM_WM_CLIENT_MACHINE,
  MBWM_ATOM_WM_TRANSIENT_FOR,
  MBWM_ATOM_WM_TAKE_FOCUS,

  /* EWMH */

  MBWM_ATOM_NET_WM_WINDOW_TYPE,
  MBWM_ATOM_NET_WM_WINDOW_TYPE_NORMAL,
  MBWM_ATOM_NET_WM_WINDOW_TYPE_TOOLBAR,
  MBWM_ATOM_NET_WM_WINDOW_TYPE_DOCK,
  MBWM_ATOM_NET_WM_WINDOW_TYPE_MENU,
  MBWM_ATOM_NET_WM_WINDOW_TYPE_DIALOG,
  MBWM_ATOM_NET_WM_WINDOW_TYPE_SPLASH,
  MBWM_ATOM_NET_WM_WINDOW_TYPE_DESKTOP,
  MBWM_ATOM_NET_WM_WINDOW_TYPE_INPUT,

  MBWM_ATOM_NET_WM_STATE,
  MBWM_ATOM_NET_WM_STATE_FULLSCREEN,
  MBWM_ATOM_NET_WM_STATE_MODAL,
  MBWM_ATOM_NET_WM_STATE_ABOVE,
  MBWM_ATOM_NET_WM_STATE_STICKY,
  MBWM_ATOM_NET_WM_STATE_MAXIMIZED_VERT,
  MBWM_ATOM_NET_WM_STATE_MAXIMIZED_HORZ,
  MBWM_ATOM_NET_WM_STATE_SHADED,
  MBWM_ATOM_NET_WM_STATE_SKIP_TASKBAR,
  MBWM_ATOM_NET_WM_STATE_SKIP_PAGER,
  MBWM_ATOM_NET_WM_STATE_HIDDEN,
  MBWM_ATOM_NET_WM_STATE_BELOW,
  MBWM_ATOM_NET_WM_STATE_DEMANDS_ATTENTION,

  MBWM_ATOM_NET_SUPPORTED,
  MBWM_ATOM_NET_CLIENT_LIST,
  MBWM_ATOM_NET_NUMBER_OF_DESKTOPS,
  MBWM_ATOM_NET_ACTIVE_WINDOW,
  MBWM_ATOM_NET_SUPPORTING_WM_CHECK,
  MBWM_ATOM_NET_CLOSE_WINDOW,
  MBWM_ATOM_NET_WM_NAME,
  MBWM_ATOM_NET_WM_USER_TIME,

  MBWM_ATOM_NET_CLIENT_LIST_STACKING,
  MBWM_ATOM_NET_CURRENT_DESKTOP,
  MBWM_ATOM_NET_WM_DESKTOP,
  MBWM_ATOM_NET_WM_ICON,
  MBWM_ATOM_NET_DESKTOP_GEOMETRY,
  MBWM_ATOM_NET_WORKAREA,
  MBWM_ATOM_NET_SHOW_DESKTOP,
  MBWM_ATOM_NET_SHOWING_DESKTOP,
  MBWM_ATOM_NET_DESKTOP_VIEWPORT,
  MBWM_ATOM_NET_FRAME_EXTENTS,
  MBWM_ATOM_NET_WM_FULL_PLACEMENT,

  MBWM_ATOM_NET_WM_ALLOWED_ACTIONS,
  MBWM_ATOM_NET_WM_ACTION_MOVE,
  MBWM_ATOM_NET_WM_ACTION_RESIZE,
  MBWM_ATOM_NET_WM_ACTION_MINIMIZE,
  MBWM_ATOM_NET_WM_ACTION_SHADE,
  MBWM_ATOM_NET_WM_ACTION_STICK,
  MBWM_ATOM_NET_WM_ACTION_MAXIMIZE_HORZ,
  MBWM_ATOM_NET_WM_ACTION_MAXIMIZE_VERT,
  MBWM_ATOM_NET_WM_ACTION_FULLSCREEN,
  MBWM_ATOM_NET_WM_ACTION_CHANGE_DESKTOP,
  MBWM_ATOM_NET_WM_ACTION_CLOSE,

  MBWM_ATOM_NET_WM_PING,
  MBWM_ATOM_NET_WM_PID,

  /* Startup Notification */
  MBWM_ATOM_NET_STARTUP_ID,

  /* Misc */

  MBWM_ATOM_UTF8_STRING,
  MBWM_ATOM_MOTIF_WM_HINTS,
  MBWM_ATOM_WIN_SUPPORTING_WM_CHECK,

  MBWM_ATOM_NET_WM_CONTEXT_HELP,
  MBWM_ATOM_NET_WM_CONTEXT_ACCEPT,
  MBWM_ATOM_NET_WM_CONTEXT_CUSTOM,
  MBWM_ATOM_NET_WM_SYNC_REQUEST,

  MBWM_ATOM_CM_TRANSLUCENCY,

  MBWM_ATOM_MB_APP_WINDOW_LIST_STACKING,

  /* FIXME: Custom/Unused to sort out
   *
   * MB_WM_STATE_DOCK_TITLEBAR,
   *
   * _NET_WM_SYNC_REQUEST_COUNTER,
   * _NET_WM_SYNC_REQUEST,
   * _MB_CURRENT_APP_WINDOW,
   *
   * MB_COMMAND,
   * MB_CLIENT_EXEC_MAP,
   * MB_CLIENT_STARTUP_LIST,
   * MB_DOCK_TITLEBAR_SHOW_ON_DESKTOP,
   * MB_GRAB_TRANSFER,
   *
   * WINDOW_TYPE_MESSAGE,
   *
   * _MB_THEME,
   * _MB_THEME_NAME,
  */

  MBWM_ATOM_COUNT

} MBWMAtom;

/* Keys */

typedef struct MBWMKeyBinding MBWMKeyBinding;
typedef struct MBWMKeys       MBWMKeys;

typedef void (*MBWMKeyPressedFunc) (MBWindowManager   *wm,
				    MBWMKeyBinding    *binding,
				    void              *userdata);

typedef void (*MBWMKeyDestroyFunc) (MBWindowManager   *wm,
				    MBWMKeyBinding    *binding,
				    void              *userdata);

struct MBWMKeyBinding
{
  KeySym                   keysym;
  int                      modifier_mask;
  MBWMKeyPressedFunc       pressed;
  MBWMKeyDestroyFunc       destroy;
  void                    *userdata;
  /* FIXME: free func */
};

/* Event Callbacks */

typedef Bool (*MBWMXEventFunc)
     (void              *xev,
      void              *userdata);

typedef Bool (*MBWindowManagerMapNotifyFunc)
     (XMapEvent         *xev,
      void              *userdata);

typedef Bool (*MBWindowManagerMapRequestFunc)
     (XMapRequestEvent  *xev,
      void              *userdata);

typedef Bool (*MBWindowManagerUnmapNotifyFunc)
     (XUnmapEvent       *xev,
      void              *userdata);

typedef Bool (*MBWindowManagerDestroyNotifyFunc)
     (XDestroyWindowEvent  *xev,
      void                 *userdata);

typedef Bool (*MBWindowManagerConfigureNotifyFunc)
     (XConfigureEvent      *xev,
      void                 *userdata);

typedef Bool (*MBWindowManagerConfigureRequestFunc)
     (XConfigureRequestEvent  *xev,
      void                    *userdata);

typedef Bool (*MBWindowManagerKeyPressFunc)
     (XKeyEvent               *xev,
      void                    *userdata);

typedef Bool (*MBWindowManagerPropertyNotifyFunc)
     (XPropertyEvent          *xev,
      void                    *userdata);

typedef Bool (*MBWindowManagerButtonPressFunc)
     (XButtonEvent            *xev,
      void                    *userdata);

typedef Bool (*MBWindowManagerButtonReleaseFunc)
     (XButtonEvent            *xev,
      void                    *userdata);

typedef Bool (*MBWindowManagerTimeOutFunc)
     (void                    *userdata);

typedef Bool (*MBWindowManagerFdWatchFunc)
     (int                      fd,
      int                      events,
      void                    *userdata);

typedef struct MBWMXEventFuncInfo
{
  MBWMXEventFunc func;
  Window         xwindow;
  void          *userdata;
  unsigned long  id;
}
MBWMXEventFuncInfo;

typedef struct MBWMTimeOutEventInfo MBWMTimeOutEventInfo;
typedef struct MBWMFdWatchInfo      MBWMFdWatchInfo;

typedef enum MBWMDecorButtonFlags
{
  MB_WM_DECOR_BUTTON_INVISIBLE = (1<<1)

} MBWMDecorButtonFlags;

typedef enum MBWMDecorType
{
  MBWMDecorTypeNorth,
  MBWMDecorTypeSouth,
  MBWMDecorTypeEast,
  MBWMDecorTypeWest,

} MBWMDecorType;

typedef enum MBWMSyncType
{
  MBWMSyncStacking          = (1<<1),
  MBWMSyncGeometry          = (1<<2),
  MBWMSyncVisibility        = (1<<3),
  MBWMSyncDecor             = (1<<4),
  MBWMSyncSyntheticConfigEv = (1<<5),
  MBWMSyncFullscreen        = (1<<6),
} MBWMSyncType;


#endif
