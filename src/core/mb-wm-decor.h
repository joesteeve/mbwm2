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

#ifndef _HAVE_MB_WM_DECOR_H
#define _HAVE_MB_WM_DECOR_H

typedef struct MBWMDecorButton MBWMDecorButton;
typedef struct MBWMDecor       MBWMDecor;

typedef enum MBWMDecorButtonFlags
{
  MBDecorFoo

} MBWMDecorButtonFlags;


MBWMDecor*
mb_wm_decor_create (MBWindowManager *wm, 
		    int              width,
		    int              height);

Window
mb_wm_decor_get_x_window (MBWMDecor *decor);

Bool
mb_wm_decor_attach (MBWMDecor             *decor,
		    MBWindowManagerClient *client,
		    int                    x,
		    int                    y);

void
mb_wm_decor_unref (MBWMDecor *decor);

void
mb_wm_decor_ref (MBWMDecor *decor);


#endif
