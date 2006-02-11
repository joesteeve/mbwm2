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
  MB_WM_DECOR_BUTTON_INVISIBLE = (1<<1)

} MBWMDecorButtonFlags;

typedef enum MBWMDecorType
{
  MBWMDecorTypeNorth,
  MBWMDecorTypeSouth,
  MBWMDecorTypeEast,
  MBWMDecorTypeWest,

} MBWMDecorType;

typedef void (*MBWMDecorResizedFunc) (MBWindowManager   *wm,
				      MBWMDecor         *decor,
				      void              *userdata);

typedef void (*MBWMDecorRepaintFunc) (MBWindowManager   *wm,
				      MBWMDecor         *decor,
				      void              *userdata);

typedef void (*MBWMDecorButtonRepaintFunc) (MBWindowManager   *wm,
					    MBWMDecorButton   *button,
					    void              *userdata);

typedef void (*MBWMDecorButtonPressedFunc) (MBWindowManager   *wm,
					    MBWMDecorButton   *button,
					    void              *userdata);

typedef void (*MBWMDecorButtonReleasedFunc) (MBWindowManager   *wm,
					     MBWMDecorButton   *button,
					     void              *userdata);

MBWMDecor*
mb_wm_decor_create (MBWindowManager     *wm,
		    MBWMDecorType        type,
		    MBWMDecorResizedFunc resize,
		    MBWMDecorResizedFunc repaint,
		    void                *userdata);

static Bool
mb_wm_decor_reparent (MBWMDecor *decor);

static void
mb_wm_decor_calc_geometry (MBWMDecor *decor);

void
mb_wm_decor_handle_repaint (MBWMDecor *decor);

void
mb_wm_decor_handle_resize (MBWMDecor *decor);

Window
mb_wm_decor_get_x_window (MBWMDecor *decor);

MBWMDecorType
mb_wm_decor_get_type (MBWMDecor *decor);

const MBGeometry*
mb_wm_decor_get_geometry (MBWMDecor *decor);

MBWindowManagerClient*
mb_wm_decor_get_parent (MBWMDecor *decor);

void
mb_wm_decor_mark_dirty (MBWMDecor *decor);

void
mb_wm_decor_attach (MBWMDecor             *decor,
		    MBWindowManagerClient *client);

void
mb_wm_decor_detach (MBWMDecor *decor);

void
mb_wm_decor_unref (MBWMDecor *decor);

void
mb_wm_decor_ref (MBWMDecor *decor);


#endif
