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

#ifndef _HAVE_MB_WM_CORE_H
#define _HAVE_MB_WM_CORE_H

MBWindowManager*
mb_wm_new(void);

Status
mb_wm_init(MBWindowManager *wm, int *argc, char ***argv);

void
mb_wm_run(MBWindowManager *wm);

MBWindowManagerClient*
mb_wm_core_managed_client_from_xwindow(MBWindowManager *wm, Window win);

int
mb_wm_register_client_type (MBWindowManager* wm);

void
mb_wm_core_manage_client (MBWindowManager       *wm,
			  MBWindowManagerClient *client);

void
mb_wm_core_unmanage_client (MBWindowManager       *wm,
			    MBWindowManagerClient *client);

void
mb_wm_display_sync_queue (MBWindowManager* wm);

void
mb_wm_get_display_geometry (MBWindowManager  *wm, 
			    MBGeometry       *geometry);


#endif
