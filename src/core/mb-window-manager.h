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

struct MBWindowManager
{
  MBWMObject                   parent;

  Display                     *xdpy;
  unsigned int                 xdpy_width, xdpy_height;
  int                          xscreen;

  MBWindowManagerClient       *stack_top, *stack_bottom;
  MBWMList                    *clients;
  MBWMList                    *unmapped_clients;
  MBWMList                    *desktop;
  MBWindowManagerClient       *stack_top_app;
  MBWindowManagerClient       *focused_client;

  Atom                         atoms[MBWM_ATOM_COUNT];

  MBWMKeys                    *keys; /* Keybindings etc */

  XasContext                  *xas_context;

  /* ### Private ### */
  Bool                         need_display_sync;
  int                          client_type_cnt;
  int                          stack_n_clients;
  MBWMRootWindow              *root_win;

  const char                  *sm_client_id;

  MBWMTheme                   *theme;
  MBWMLayout                  *layout;
  MBWMMainContext             *main_ctx;
};

struct MBWindowManagerClass
{
  MBWMObjectClass parent;

  void (*process_cmdline) (MBWindowManager * wm, int argc, char **argv);

  MBWindowManagerClient* (*client_new) (MBWindowManager *wm,
					MBWMClientWindow *w);

  /* These return True if now further action to be taken */
  Bool (*client_activate)   (MBWindowManager *wm, MBWindowManagerClient *c);
  Bool (*client_responding) (MBWindowManager *wm, MBWindowManagerClient *c);
  Bool (*client_hang)       (MBWindowManager *wm, MBWindowManagerClient *c);

  MBWMTheme * (*theme_init) (MBWindowManager *wm);
};

MBWindowManager *
mb_wm_new (int argc, char **argv);

void
mb_wm_set_layout (MBWindowManager *wm, MBWMLayout *layout);

int
mb_wm_class_type ();

void
mb_wm_main_loop(MBWindowManager *wm);

MBWindowManagerClient*
mb_wm_managed_client_from_xwindow(MBWindowManager *wm, Window win);

MBWindowManagerClient*
mb_wm_unmapped_client_from_xwindow(MBWindowManager *wm, Window win);

int
mb_wm_register_client_type (void);

void
mb_wm_manage_client (MBWindowManager       *wm,
		     MBWindowManagerClient *client,
		     Bool                   activate);

void
mb_wm_unmanage_client (MBWindowManager       *wm,
		       MBWindowManagerClient *client,
		       Bool                   destroy);

void
mb_wm_display_sync_queue (MBWindowManager* wm);

void
mb_wm_get_display_geometry (MBWindowManager  *wm,
			    MBGeometry       *geometry);

void
mb_wm_activate_client(MBWindowManager * wm, MBWindowManagerClient *c);

void
mb_wm_handle_ping_reply (MBWindowManager * wm, MBWindowManagerClient *c);

void
mb_wm_handle_hang_client (MBWindowManager * wm, MBWindowManagerClient *c);

void
mb_wm_handle_show_desktop (MBWindowManager * wm, Bool show);

MBWindowManagerClient*
mb_wm_get_visible_main_client(MBWindowManager *wm);

void
mb_wm_unfocus_client (MBWindowManager *wm, MBWindowManagerClient *client);

#endif
