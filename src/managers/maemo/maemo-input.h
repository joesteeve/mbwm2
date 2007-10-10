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

#ifndef _HAVE_MAEMO_INPUT_H
#define _HAVE_MAEMO_INPUT_H

#include "mb-wm.h"
#include "mb-wm-client-input.h"

typedef struct MaemoInput      MaemoInput;
typedef struct MaemoInputClass MaemoInputClass;

#define MAEMO_INPUT(c) ((MaemoInput*)(c))
#define MAEMO_INPUT_CLASS(c) ((MaemoInputClass*)(c))
#define MAEMO_TYPE_INPUT (maemo_input_class_type ())
#define MAMEO_IS_INPUT(c) (MB_WM_OBJECT_TYPE(c)==MAEMO_TYPE_INPUT)

struct MaemoInput
{
  MBWMClientInput    parent;
};

struct MaemoInputClass
{
  MBWMClientInputClass parent;
};

MBWindowManagerClient*
maemo_input_new (MBWindowManager *wm, MBWMClientWindow *win);

int
maemo_input_class_type ();

#endif
