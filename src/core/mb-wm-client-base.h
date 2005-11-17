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

#ifndef _HAVE_MB_WM_CLIENT_BASE_H
#define _HAVE_MB_WM_CLIENT_BASE_H

void
mb_wm_client_base_init (MBWindowManager             *wm, 
			MBWindowManagerClient       *client,
			MBWMWindow                  *win);
void
mb_wm_client_base_realize (MBWindowManagerClient *client);

void
mb_wm_client_base_stack (MBWindowManagerClient *client,
			 int                    flags);
void
mb_wm_client_base_show (MBWindowManagerClient *client);

void
mb_wm_client_base_hide (MBWindowManagerClient *client);

void
mb_wm_client_base_destroy (MBWindowManagerClient *client);

void
mb_wm_client_base_display_sync (MBWindowManagerClient *client);

Bool
mb_wm_client_base_request_geometry (MBWindowManagerClient *client,
				    MBGeometry            *new_geometry,
				    MBWMClientReqGeomType  flags);

#endif
