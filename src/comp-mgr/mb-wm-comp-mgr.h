/*
 *  Matchbox Window Manager - A lightweight window manager not for the
 *                            desktop.
 *
 *  Authored By Matthew Allum <mallum@o-hand.com>
 *              Tomas Frydrych <tf@o-hand.com>
 *
 *  Copyright (c) 2002, 2004, 2007 OpenedHand Ltd - http://o-hand.com
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

#ifndef _HAVE_MB_WM_COMP_MGR_H
#define _HAVE_MB_WM_COMP_MGR_H

#include <X11/extensions/Xdamage.h>

#define MB_WM_COMP_MGR(c) ((MBWMCompMgr*)(c))
#define MB_WM_COMP_MGR_CLASS(c) ((MBWMCompMgrClass*)(c))
#define MB_WM_TYPE_COMP_MGR (mb_wm_comp_mgr_class_type ())

#define MB_WM_COMP_MGR_CLIENT(c) ((MBWMCompMgrClient*)(c))
#define MB_WM_COMP_MGR_CLIENT_CLASS(c) ((MBWMCompMgrClientClass*)(c))
#define MB_WM_TYPE_COMP_MGR_CLIENT (mb_wm_comp_mgr_client_class_type ())

struct MBWMCompMgr
{
  MBWMObject           parent;

  MBWindowManager     *wm;
  Bool                 disabled;

  MBWMCompMgrPrivate  *priv;
};

struct MBWMCompMgrClass
{
  MBWMObjectClass        parent;

  void   (*register_client)   (MBWMCompMgr * mgr, MBWindowManagerClient *c);
  void   (*unregister_client) (MBWMCompMgr * mgr, MBWindowManagerClient *c);
  void   (*turn_on)           (MBWMCompMgr * mgr);
  void   (*turn_off)          (MBWMCompMgr * mgr);
  void   (*render)            (MBWMCompMgr * mgr);
  Bool   (*handle_events)     (MBWMCompMgr * mgr, XEvent *ev);
};

int
mb_wm_comp_mgr_class_type ();

MBWMCompMgr*
mb_wm_comp_mgr_new (MBWindowManager *wm);

void
mb_wm_comp_mgr_register_client (MBWMCompMgr * mgr, MBWindowManagerClient *c);

void
mb_wm_comp_mgr_unregister_client (MBWMCompMgr * mgr,
				  MBWindowManagerClient *client);

void
mb_wm_comp_mgr_turn_off (MBWMCompMgr *mgr);

void
mb_wm_comp_mgr_turn_on (MBWMCompMgr *mgr);

void
mb_wm_comp_mgr_render (MBWMCompMgr *mgr);

Bool
mb_wm_comp_mgr_enabled (MBWMCompMgr *mgr);

Bool
mb_wm_comp_mgr_handle_events (MBWMCompMgr * mgr, XEvent *ev);

struct MBWMCompMgrClientClass
{
  MBWMObjectClass        parent;

  void (*show)      (MBWMCompMgrClient * client);
  void (*hide)      (MBWMCompMgrClient * client);
  void (*repair)    (MBWMCompMgrClient * client);  
  void (*configure) (MBWMCompMgrClient * client);  
};

int
mb_wm_comp_mgr_client_class_type ();

void
mb_wm_comp_mgr_client_show (MBWMCompMgrClient * client);

void
mb_wm_comp_mgr_client_hide (MBWMCompMgrClient * client);

void
mb_wm_comp_mgr_client_repair (MBWMCompMgrClient * client);

void
mb_wm_comp_mgr_client_configure (MBWMCompMgrClient * client);

#endif
