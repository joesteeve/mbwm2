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

#include <matchbox/mb-wm-config.h>
#include <matchbox/core/mb-wm.h>

#define MB_WM_THEME(c) ((MBWMTheme*)(c))
#define MB_WM_THEME_CLASS(c) ((MBWMThemeClass*)(c))
#define MB_WM_TYPE_THEME (mb_wm_theme_class_type ())

#define MB_WM_THEME_PNG(c) ((MBWMThemePng*)(c))
#define MB_WM_THEME_PNG_CLASS(c) ((MBWMThemePngClass*)(c))
#define MB_WM_TYPE_THEME_PNG (mb_wm_theme_png_class_type ())

enum MBWMThemeCaps
{
  MBWMThemeCapsFrameMainButtonActionAccept = (1<<0),
  MBWMThemeCapsFrameDlgButtonActionAccept  = (1<<1),
  MBWMThemeCapsFrameMainButtonActionHelp   = (1<<2),
  MBWMThemeCapsFrameDlgButtonActionHelp    = (1<<3),
  MBWMThemeCapsFrameMainButtonActionCustom = (1<<4),
  MBWMThemeCapsFrameDlgButtonActionCustom  = (1<<5),
};


struct MBWMThemeClass
{
  MBWMObjectClass               parent;

  void (*paint_decor)           (MBWMTheme              *theme,
				 MBWMDecor              *decor);

  void (*paint_button)          (MBWMTheme              *theme,
				 MBWMDecorButton        *button);

  void (*decor_dimensions)      (MBWMTheme              *theme,
				 MBWindowManagerClient  *client,
				 int                    *north,
				 int                    *south,
				 int                    *west,
				 int                    *east);

  void (*button_size)           (MBWMTheme              *theme,
				 MBWMDecor              *decor,
				 MBWMDecorButtonType     type,
				 int                    *width,
				 int                    *height);

  void (*button_position)       (MBWMTheme              *theme,
				 MBWMDecor              *decor,
				 MBWMDecorButtonType     type,
				 int                    *x,
				 int                    *y);

  MBWMDecor* (*create_decor)    (MBWMTheme             *theme,
			         MBWindowManagerClient *client,
			         MBWMDecorType          type);

  void  (*resize_decor)         (MBWMTheme             *theme,
			         MBWMDecor             *decor);
};

struct MBWMTheme
{
  MBWMObject             parent;

  MBWindowManager       *wm;
  MBWMThemeCaps          caps;
  char                  *path;
  MBWMList              *xml_clients;

  Bool                   compositing;
  Bool                   shaped;
  MBWMColor              color_lowlight;
  MBWMColor              color_shadow;
  MBWMCompMgrShadowType  shadow_type;
};

int
mb_wm_theme_class_type ();

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
			     MBWMDecor             *decor,
			     MBWMDecorButtonType    type,
			     int                   *width,
			     int                   *height);

void
mb_wm_theme_get_button_position (MBWMTheme             *theme,
				 MBWMDecor             *decor,
				 MBWMDecorButtonType    type,
				 int                   *x,
				 int                   *y);

Bool
mb_wm_theme_is_button_press_activated (MBWMTheme              *theme,
				       MBWMDecor              *decor,
				       MBWMDecorButtonType    type);

MBWMDecor *
mb_wm_theme_create_decor (MBWMTheme             *theme,
			  MBWindowManagerClient *client,
			  MBWMDecorType          type);

void
mb_wm_theme_resize_decor (MBWMTheme             *theme,
			  MBWMDecor             *decor);

Bool
mb_wm_theme_get_client_geometry (MBWMTheme             * theme,
				 MBWindowManagerClient * client,
				 MBGeometry            * geom);

MBWMClientLayoutHints
mb_wm_theme_get_client_layout_hints (MBWMTheme             * theme,
				     MBWindowManagerClient * client);

Bool
mb_wm_theme_is_client_shaped (MBWMTheme             * theme,
			      MBWindowManagerClient * client);

void
mb_wm_theme_get_lowlight_color (MBWMTheme             * theme,
				unsigned int          * red,
				unsigned int          * green,
				unsigned int          * blue,
				unsigned int          * alpha);

void
mb_wm_theme_get_shadow_color (MBWMTheme             * theme,
			      unsigned int          * red,
			      unsigned int          * green,
			      unsigned int          * blue,
			      unsigned int          * alpha);


MBWMCompMgrShadowType
mb_wm_theme_get_shadow_type (MBWMTheme * theme);

Bool
mb_wm_theme_use_compositing_mgr (MBWMTheme * theme);

typedef unsigned int (*MBWMThemeCustomClientTypeFunc) (const char *type_name,
						       void       *user_data);

void
mb_wm_theme_set_custom_client_type_func (MBWMThemeCustomClientTypeFunc  func,
					 void                      *user_data);

typedef unsigned int (*MBWMThemeCustomButtonTypeFunc) (const char *type_name,
						       void       *user_data);

void
mb_wm_theme_set_custom_button_type_func (MBWMThemeCustomButtonTypeFunc  func,
					 void                      *user_data);

typedef unsigned int (*MBWMThemeCustomThemeTypeFunc) (const char *type_name,
						      void       *user_data);

void
mb_wm_theme_set_custom_theme_type_func (MBWMThemeCustomThemeTypeFunc  func,
					 void                      *user_data);


typedef MBWMTheme * (*MBWMThemeCustomThemeAllocFunc) (int theme_type, ...);

void
mb_wm_theme_set_custom_theme_alloc_func (MBWMThemeCustomThemeAllocFunc  func);

#endif
