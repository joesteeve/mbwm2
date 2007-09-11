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

#ifndef _HAVE_MB_WM_WINDOW_MANAGER_H
#define _HAVE_MB_WM_WINDOW_MANAGER_H

typedef struct MBWindowManagerClass   MBWindowManagerClass;
typedef struct MBWindowManagerPriv    MBWindowManagerPriv;

#define MB_WINDOW_MANAGER(c)       ((MBWindowManager*)(c))
#define MB_WINDOW_MANAGER_CLASS(c) ((MBWindowManagerClass*)(c))
#define MB_TYPE_WINDOW_MANAGER     (mb_wm_class_type ())

typedef struct MBWindowManagerEventFuncs
{
  /* FIXME: figure our X wrap / unwrap mechanism */
  MBWMList *map_notify;
  MBWMList *unmap_notify;
  MBWMList *map_request;
  MBWMList *destroy_notify;
  MBWMList *configure_request;
  MBWMList *configure_notify;
  MBWMList *key_press;
  MBWMList *property_notify;
  MBWMList *button_press;
  MBWMList *button_release;

  MBWMList *timeout;
}
MBWindowManagerEventFuncs;

struct MBWindowManager
{
  MBWMObject                   parent;

  Display                     *xdpy;
  unsigned int                 xdpy_width, xdpy_height;
  int                          xscreen;
  MBWindowManagerEventFuncs   *event_funcs;

  MBWindowManagerClient       *stack_top, *stack_bottom;
  MBWMList                    *clients;
  MBWindowManagerClient       *desktop;
  MBWindowManagerClient       *stack_top_app;

  Atom                         atoms[MBWM_ATOM_COUNT];

  MBWMKeys                    *keys; /* Keybindings etc */

  XasContext                  *xas_context;

  /* ### Private ### */
  Bool                         need_display_sync;
  int                          client_type_cnt;
  int                          stack_n_clients;
  MBWMRootWindow              *root_win;

  int                          argc;
  const char                 **argv;
};

struct MBWindowManagerClass
{
  MBWMObjectClass parent;

  void (*process_cmdline) (MBWindowManager * wm, int argc, char **argv);

  MBWindowManagerClient* (*client_new) (MBWindowManager *wm,
					MBWMClientWindow *w);
};

MBWindowManager *
mb_wm_new (int argc, char **argv);

int
mb_wm_class_type ();

void
mb_wm_main_loop(MBWindowManager *wm);

MBWindowManagerClient*
mb_wm_managed_client_from_xwindow(MBWindowManager *wm, Window win);

int
mb_wm_register_client_type (void);

void
mb_wm_manage_client (MBWindowManager       *wm,
		     MBWindowManagerClient *client);

void
mb_wm_unmanage_client (MBWindowManager       *wm,
		       MBWindowManagerClient *client);

void
mb_wm_display_sync_queue (MBWindowManager* wm);

void
mb_wm_get_display_geometry (MBWindowManager  *wm,
			    MBGeometry       *geometry);

void
mb_wm_activate_client(MBWindowManager * wm, MBWindowManagerClient *c);

MBWindowManagerClient*
mb_wm_get_visible_main_client(MBWindowManager *wm);

#endif
