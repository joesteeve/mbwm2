/*
 *  Matchbox Window Manager II - A lightweight window manager not for the
 *                               desktop.
 *
 *  Authored by Tomas Frydrych <tf@o-hand.com>
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

#include "maemo-window-manager.h"
#include "maemo-toolbar.h"
#include "maemo-input.h"
#include "mb-wm-client-app.h"
#include "mb-wm-client-panel.h"
#include "mb-wm-client-dialog.h"
#include "mb-wm-client-desktop.h"
#include "mb-wm-client-input.h"
#include "mb-window-manager.h"

#include <stdarg.h>

static void
maemo_window_manager_process_cmdline (MBWindowManager *, int , char **);

static MBWindowManagerClient*
maemo_window_manager_client_new_func (MBWindowManager *wm,
				      MBWMClientWindow *win)
{
  if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DOCK])
    {
      printf("### is panel ###\n");
      return maemo_toolbar_new (wm, win);
    }
  else if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DIALOG])
    {
      printf("### is dialog ###\n");
      return mb_wm_client_dialog_new (wm, win);
    }
  else if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_NOTIFICATION])
    {
      printf("### is notifcation ###\n");
      return mb_wm_client_dialog_new (wm, win);
    }
  else if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DESKTOP])
    {
      printf("### is desktop ###\n");
      /* Only one desktop allowed */
      if (wm->desktop)
	return NULL;

      return mb_wm_client_desktop_new (wm, win);
    }
  else if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_TOOLBAR])
    {
      printf("### is input ###\n");
      return maemo_input_new (wm, win);
    }
  else if (win->net_type ==wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_MENU] ||
	   win->net_type ==wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_POPUP_MENU])
    {
      printf("### is menu ###\n");
      return mb_wm_client_menu_new(wm, win);
    }
  else if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_NORMAL])
    {
      printf("### is application ###\n");
      return mb_wm_client_app_new(wm, win);
    }
  else
    {
#if 1
      char * name = XGetAtomName (wm->xdpy, win->net_type);
      printf("### unhandled window type %s ###\n", name);
      XFree (name);
#endif
    }

  return NULL;
}

static void
maemo_window_manager_class_init (MBWMObjectClass *klass)
{
  MBWindowManagerClass *wm_class;

  MBWM_MARK();

  wm_class = (MBWindowManagerClass *)klass;

  wm_class->process_cmdline = maemo_window_manager_process_cmdline;
  wm_class->client_new      = maemo_window_manager_client_new_func;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MaemoWindowManager";
#endif
}

static void
maemo_window_manager_destroy (MBWMObject *this)
{
}

static int
maemo_window_manager_init (MBWMObject *this, va_list vap)
{
  MBWindowManager *wm = MB_WINDOW_MANAGER (this);
  wm->modality_type = MBWMModalitySystem;

  return 1;
}

int
maemo_window_manager_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWindowManagerClass),
	sizeof (MBWindowManager),
	maemo_window_manager_init,
	maemo_window_manager_destroy,
	maemo_window_manager_class_init
      };

      type = mb_wm_object_register_class (&info, MB_TYPE_WINDOW_MANAGER, 0);
    }

  return type;
}

MBWindowManager*
maemo_window_manager_new (int argc, char **argv)
{
  MBWindowManager      *wm = NULL;

  wm = MB_WINDOW_MANAGER (mb_wm_object_new (MB_TYPE_MAEMO_WINDOW_MANAGER,
					    MBWMObjectPropArgc, argc,
					    MBWMObjectPropArgv, argv,
					    NULL));

  if (!wm)
    return wm;

  return wm;
}

static void
maemo_window_manager_process_cmdline (MBWindowManager *wm,
				      int argc, char **argv)
{
  MBWindowManagerClass * wm_class =
    MB_WINDOW_MANAGER (MB_WM_OBJECT_GET_PARENT_CLASS (MB_WM_OBJECT (wm)));

  if (wm_class->process_cmdline)
    wm_class->process_cmdline (wm, argc, argv);
}

