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

#define MB_WM_THEME(c) ((MBWMTheme*)(c))
#define MB_WM_THEME_CLASS(c) ((MBWMThemeClass*)(c))
#define MB_WM_TYPE_THEME (mb_wm_theme_class_type ())

#define MB_WM_THEME_SIMPLE(c) ((MBWMThemeSimple*)(c))
#define MB_WM_THEME_SIMPLE_CLASS(c) ((MBWMThemeSimpleClass*)(c))
#define MB_WM_TYPE_THEME_SIMPLE (mb_wm_theme_simple_class_type ())

#define MB_WM_THEME_CAIRO(c) ((MBWMThemeCairo*)(c))
#define MB_WM_THEME_CAIRO_CLASS(c) ((MBWMThemeCairoClass*)(c))
#define MB_WM_TYPE_THEME_CAIRO (mb_wm_theme_cairo_class_type ())

enum MBWMThemeCaps
{
  MBWMThemeCapsFrameMainButtonActionAccept = (1<<0),
  MBWMThemeCapsFrameDlgButtonActionAccept  = (1<<1),
  MBWMThemeCapsFrameMainButtonActionHelp   = (1<<2),
  MBWMThemeCapsFrameDlgButtonActionHelp    = (1<<3),

};


struct MBWMThemeClass
{
  MBWMObjectClass           parent;

  void (*paint_decor)      (MBWMTheme              *theme,
			    MBWMDecor              *decor);

  void (*paint_button)     (MBWMTheme              *theme,
			    MBWMDecorButton        *button);

  void (*decor_dimensions) (MBWMTheme              *theme,
			    MBWindowManagerClient  *client,
			    int                    *north,
			    int                    *south,
			    int                    *west,
			    int                    *east);

  void (*button_size)      (MBWMTheme              *theme,
			    MBWindowManagerClient  *client,
			    int                    *width,
			    int                    *height);
};

struct MBWMTheme
{
  MBWMObject        parent;

  MBWindowManager  *wm;
  MBWMThemeCaps     caps;
  char             *path;
};

MBWMTheme *
mb_wm_theme_new (MBWindowManager * wm,  const char * theme_path);

void
mb_wm_theme_paint_decor (MBWMTheme *theme,
			 MBWMDecor *decor);

void
mb_wm_theme_paint_button (MBWMTheme       *theme,
			  MBWMDecorButton *button);

Bool
mb_wm_theme_supports (MBWMTheme     *theme,
		      MBWMThemeCaps  capability);

void
mb_wm_theme_get_decor_dimensions (MBWMTheme             *theme,
				  MBWindowManagerClient *client,
				  int                   *north,
				  int                   *south,
				  int                   *west,
				  int                   *east);

void
mb_wm_theme_get_button_size (MBWMTheme             *theme,
			     MBWindowManagerClient *client,
			     int                   *width,
			     int                   *height);

#endif
