/*
 *  Matchbox Window Manager - A lightweight window manager not for the
 *                            desktop.
 *
 *  Authored By Tomas Frydrych <tf@o-hand.com>
 *
 *  Copyright (c) 2008 OpenedHand Ltd - http://o-hand.com
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

#ifndef _HAVE_MB_WM_COMP_MGR_CLUTTER_H
#define _HAVE_MB_WM_COMP_MGR_CLUTTER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define MB_WM_COMP_MGR_CLUTTER(c) ((MBWMCompMgrClutter*)(c))
#define MB_WM_COMP_MGR_CLUTTER_CLASS(c) ((MBWMCompMgrClutterClass*)(c))
#define MB_WM_TYPE_COMP_MGR_CLUTTER (mb_wm_comp_mgr_clutter_class_type ())

#define MB_WM_COMP_MGR_CLUTTER_CLIENT(c) ((MBWMCompMgrClutterClient*)(c))
#define MB_WM_COMP_MGR_CLUTTER_CLIENT_CLASS(c) ((MBWMCompMgrClutterClientClass*)(c))
#define MB_WM_TYPE_COMP_MGR_CLUTTER_CLIENT (mb_wm_comp_mgr_clutter_client_class_type ())

#define MB_WM_COMP_MGR_CLUTTER_EFFECT(c) ((MBWMCompMgrClutterEffect*)(c))
#define MB_WM_COMP_MGR_CLUTTER_EFFECT_CLASS(c) ((MBWMCompMgrClutterEffectClass*)(c))
#define MB_WM_TYPE_COMP_MGR_CLUTTER_EFFECT (mb_wm_comp_mgr_clutter_effect_class_type ())


struct MBWMCompMgrClutter
{
  MBWMCompMgr                 parent;
  MBWMCompMgrClutterPrivate  *priv;
};

struct MBWMCompMgrClutterClass
{
  MBWMCompMgrClass  parent;
};

int
mb_wm_comp_mgr_clutter_class_type ();

MBWMCompMgr*
mb_wm_comp_mgr_clutter_new (MBWindowManager *wm);

struct MBWMCompMgrClutterClientClass
{
  MBWMCompMgrClientClass parent;
};

int
mb_wm_comp_mgr_clutter_client_class_type ();

struct MBWMCompMgrClutterEffectClass
{
  MBWMCompMgrEffectClass parent;
};

int
mb_wm_comp_mgr_clutter_effect_class_type ();

#endif
