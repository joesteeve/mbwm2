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

    "WM_NAME",
    "WM_STATE",
    "WM_HINTS",
    "WM_CHANGE_STATE",
    "WM_PROTOCOLS",
    "WM_DELETE_WINDOW",
    "WM_COLORMAP_WINDOWS",
    "WM_CLIENT_MACHINE",
    "WM_TRANSIENT_FOR",
    "WM_TAKE_FOCUS",

    "_NET_WM_WINDOW_TYPE",
    "_NET_WM_WINDOW_TYPE_NORMAL",
    "_NET_WM_WINDOW_TYPE_TOOLBAR",
    "_NET_WM_WINDOW_TYPE_INPUT",
    "_NET_WM_WINDOW_TYPE_DOCK",
    "_NET_WM_WINDOW_TYPE_MENU",
    "_NET_WM_WINDOW_TYPE_POPUP_MENU",
    "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
    "_NET_WM_WINDOW_TYPE_DIALOG",
    "_NET_WM_WINDOW_TYPE_SPLASH",
    "_NET_WM_WINDOW_TYPE_DESKTOP",
    "_NET_WM_WINDOW_TYPE_NOTIFICATION",

    "_NET_WM_STATE",
    "_NET_WM_STATE_FULLSCREEN",
    "_NET_WM_STATE_MODAL",
    "_NET_WM_STATE_ABOVE",
    "_NET_WM_STATE_STICKY",
    "_NET_WM_STATE_MAXIMIZED_VERT",
    "_NET_WM_STATE_MAXIMIZED_HORZ",
    "_NET_WM_STATE_SHADED",
    "_NET_WM_STATE_SKIP_TASKBAR",
    "_NET_WM_STATE_SKIP_PAGER",
    "_NET_WM_STATE_HIDDEN",
    "_NET_WM_STATE_BELOW",
    "_NET_WM_STATE_DEMANDS_ATTENTION",

    "_NET_SUPPORTED",
    "_NET_CLIENT_LIST",
    "_NET_NUMBER_OF_DESKTOPS",
    "_NET_ACTIVE_WINDOW",
    "_NET_SUPPORTING_WM_CHECK",
    "_NET_CLOSE_WINDOW",
    "_NET_WM_NAME",
    "_NET_WM_USER_TIME",

    "_NET_CLIENT_LIST_STACKING",
    "_NET_CURRENT_DESKTOP",
    "_NET_WM_DESKTOP",
    "_NET_WM_ICON",
    "_NET_DESKTOP_GEOMETRY",
    "_NET_WORKAREA",
    "_NET_SHOWING_DESKTOP",
    "_NET_DESKTOP_VIEWPORT",
    "_NET_FRAME_EXTENTS",
    "_NET_WM_FULL_PLACEMENT",

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
    "_MOTIF_WM_HINTS",
    "WIN_SUPPORTING_WM_CHECK",

    "_NET_WM_CONTEXT_HELP",
    "_NET_WM_CONTEXT_ACCEPT",
    "_NET_WM_CONTEXT_CUSTOM",
    "_NET_WM_SYNC_REQUEST",
    "CM_TRANSLUCENCY",
    "_MB_APP_WINDOW_LIST_STACKING",
    "_MB_THEME",
    "_MB_THEME_NAME",
    "_MB_COMMAND",
    "_MB_GRAB_TRANSFER",
    "_MB_CURRENT_APP_WINDOW",
  };

  /* FIXME: Error Traps */

  printf("%i vs %i\n", MBWM_ATOM_COUNT, sizeof (atom_names) / sizeof (char*));

  XInternAtoms (wm->xdpy,
		atom_names,
		MBWM_ATOM_COUNT,
                False,
		wm->atoms);
}