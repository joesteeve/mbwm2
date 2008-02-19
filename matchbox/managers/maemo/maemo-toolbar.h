/*
 *  Matchbox Window Manager II - A lightweight window manager not for the
 *                               desktop.
 *
 *  Authored by Tomas Frydrych <tf@o-hand.com>
 *
 *  Copyright (c) 2007 OpenedHand Ltd - http://o-hand.com
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

#ifndef _HAVE_MAEMO_TOOLBAR_H
#define _HAVE_MAEMO_TOOLBAR_H

#include <matchbox/core/mb-wm.h>
#include <matchbox/client-types/mb-wm-client-panel.h>

typedef struct MaemoToolbar      MaemoToolbar;
typedef struct MaemoToolbarClass MaemoToolbarClass;

#define MAEMO_TOOLBAR(c) ((MaemoToolbar*)(c))
#define MAEMO_TOOLBAR_CLASS(c) ((MaemoToolbarClass*)(c))
#define MB_WM_TYPE_MAEMO_TOOLBAR (maemo_toolbar_class_type ())
#define MB_WM_IS_MAEMO_TOOLBAR(c) \
                           (MB_WM_OBJECT_TYPE(c)==MB_WM_TYPE_MAEMO_TOOLBAR)

struct MaemoToolbar
{
  MBWMClientPanel parent;

  Bool            task_navigator;
};

struct MaemoToolbarClass
{
  MBWMClientPanelClass parent;
};

MBWindowManagerClient*
maemo_toolbar_new(MBWindowManager *wm, MBWMClientWindow *win);

int
maemo_toolbar_class_type ();

#endif
