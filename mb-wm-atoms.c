#include "mb-wm.h"

void
mb_wm_atoms_init(MBWindowManager *wm)
{
  /*  
   *   The list below *MUST* be kept in the same order as the corresponding
   *   emun in structs.h or *everything* will break.
   *   Doing it like this avoids a mass of round trips on startup.
   */

  char *atom_names[] = {

    "WM_STATE",
    "WM_CHANGE_STATE",
    "WM_PROTOCOLS",
    "WM_DELETE_WINDOW",
    "WM_COLORMAP_WINDOWS",
    "WM_CLIENT_MACHINE",
    "WM_TRANSIENT_FOR",

    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_WINDOW_TYPE_NORMAL",
    "_NET_WM_WINDOW_TYPE_TOOLBAR",
    "_NET_WM_WINDOW_TYPE_DOCK",
    "_NET_WM_WINDOW_TYPE_MENU",
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_NET_WM_WINDOW_TYPE_SPLASH",
    "_NET_WM_WINDOW_TYPE_DESKTOP",
    "_NET_WM_WINDOW_TYPE_INPUT",
    
    "_NET_WM_WINDOW_STATE",
    "_NET_WM_WINDOW_STATE_FULLSCREEN",
    "_NET_WM_WINDOW_STATE_MODAL",
    "_NET_WM_WINDOW_STATE_ABOVE",
    
    "_NET_SUPPORTED",
    "_NET_CLIENT_LIST",
    "_NET_NUMBER_OF_DESKTOPS",
    "_NET_ACTIVE_WINDOW",
    "_NET_SUPPORTING_WM_CHECK",
    "_NET_CLOSE_WINDOW",
    "_NET_WM_NAME",
    
    "_NET_CLIENT_LIST_STACKING",
    "_NET_CURRENT_DESKTOP",
    "_NET_WM_DESKTOP",
    "_NET_WM_ICON",
    "_NET_DESKTOP_GEOMETRY",
    "_NET_WORKAREA",
    "_NET_SHOW_DESKTOP",
    
    "_NET_WM_ALLOWED_ACTIONS",
    "_NET_WM_ACTION_MOVE",
    "_NET_WM_ACTION_RESIZE",
    "_NET_WM_ACTION_MINIMIZE",
    "_NET_WM_ACTION_SHADE",
    "_NET_WM_ACTION_STICK",
    "_NET_WM_ACTION_MAXIMIZE_HORZ",
    "_NET_WM_ACTION_MAXIMIZE_VERT",
    "_NET_WM_ACTION_FULLSCREEN",
    "_NET_WM_ACTION_CHANGE_DESKTOP",
    "_NET_WM_ACTION_CLOSE",
    
    "_NET_WM_PING",
    "_NET_WM_PID",
    
    "_NET_STARTUP_ID",
    
    "UTF8_STRING",
    "MOTIF_WM_HINTS",
    "WIN_SUPPORTING_WM_CHECK",
  };

  /* FIXME: Error Traps */

  XInternAtoms (wm->xdpy, 
		atom_names, 
		MBWM_ATOM_COUNT,
                False, 
		wm->atoms);
}
