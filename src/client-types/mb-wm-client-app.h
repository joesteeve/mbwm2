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

#ifndef _HAVE_MB_CLIENT_APP_H
#define _HAVE_MB_CLIENT_APP_H

#include "mb-wm.h"

#define MB_WM_CLIENT_TYPE_APP mb_wm_client_app_get_type()

typedef struct MBWindowManagerClientApp MBWindowManagerClientApp;

int
mb_wm_client_app_get_type ();

MBWindowManagerClient*
mb_wm_client_app_new (MBWindowManager *wm, MBWMWindow *win);

#endif
