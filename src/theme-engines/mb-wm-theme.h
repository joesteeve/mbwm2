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

#ifndef _HAVE_MB_WM_THEME_H
#define _HAVE_MB_WM_THEME_H

#include "mb-wm.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

typedef enum MBWMThemeCaps 
{
  MBWMThemeCapsHasSomethingOrOther = (1<<1)

} MBWMThemeCaps;

void
mb_wm_theme_paint_decor (MBWindowManager   *wm,
			 MBWMDecor         *decor);

void
mb_wm_theme_paint_button (MBWindowManager   *wm,
			  MBWMDecorButton   *button);

Bool
mb_wm_theme_switch (MBWindowManager   *wm,
		    const char        *detail);

Bool
mb_wm_theme_init (MBWindowManager   *wm);


#endif
