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

#ifndef _HAVE_MB_WM_LAYOUT_MANAGER_H
#define _HAVE_MB_WM_LAYOUT_MANAGER_H

#include <matchbox/core/mb-wm.h>

#define MB_WM_LAYOUT(c) ((MBWMLayout*)(c))
#define MB_WM_LAYOUT_CLASS(c) ((MBWMLayoutClass*)(c))
#define MB_WM_TYPE_LAYOUT (mb_wm_layout_class_type ())

struct MBWMLayout
{
  MBWMObject    parent;

  MBWindowManager *wm;
};

struct MBWMLayoutClass
{
  MBWMObjectClass parent;

  void (*update) (MBWMLayout *layout);
};

int
mb_wm_layout_class_type ();

MBWMLayout*
mb_wm_layout_new (MBWindowManager *wm);

void
mb_wm_layout_update (MBWMLayout *layout);

#endif
