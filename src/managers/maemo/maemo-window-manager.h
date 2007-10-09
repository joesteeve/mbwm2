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

#ifndef _HAVE_MAEMO_WINDOW_MANAGER_H
#define _HAVE_MAEMO_WINDOW_MANAGER_H

#include "mb-wm.h"

typedef struct MaemoWindowManagerClass   MaemoWindowManagerClass;
typedef struct MaemoWindowManagerPriv    MaemoWindowManagerPriv;

#define MAEMO_WINDOW_MANAGER(c)       ((MaemoWindowManager*)(c))
#define MAEMO_WINDOW_MANAGER_CLASS(c) ((MaemoWindowManagerClass*)(c))
#define MB_TYPE_MAEMO_WINDOW_MANAGER     (maemo_window_manager_class_type ())

struct MaemoWindowManager
{
  MBWindowManager              parent;
};

struct MaemoWindowManagerClass
{
  MBWindowManagerClass parent;
};

MBWindowManager *
maemo_window_manager_new (int argc, char **argv);

#endif
