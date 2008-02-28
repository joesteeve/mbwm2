/*
 *  Matchbox Window Manager - A lightweight window manager not for the
 *                            desktop.
 *
 *  Authored By Tomas Frydrych <tf@o-hand.com>
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

#define MB_WM_COMP_MGR_EFFECT(c) ((MBWMCompMgrEffect*)(c))
#define MB_WM_COMP_MGR_EFFECT_CLASS(c) ((MBWMCompMgrEffectClass*)(c))
#define MB_WM_TYPE_COMP_MGR_EFFECT (mb_wm_comp_mgr_effect_class_type ())

struct MBWMCompMgr
{
  MBWMObject           parent;

  MBWindowManager     *wm;
  Bool                 disabled;
};

struct MBWMCompMgrClass
{
  MBWMObjectClass        parent;

  void   (*register_client)   (MBWMCompMgr * mgr, MBWindowManagerClient *c);
  void   (*unregister_client) (MBWMCompMgr * mgr, MBWindowManagerClient *c);
  void   (*turn_on)           (MBWMCompMgr * mgr);
  void   (*turn_off)          (MBWMCompMgr * mgr);
  void   (*render)            (MBWMCompMgr * mgr);
  void   (*map_notify)        (MBWMCompMgr * mgr, MBWindowManagerClient *c);
  void   (*unmap_notify)      (MBWMCompMgr * mgr, MBWindowManagerClient *c);
  Bool   (*handle_events)     (MBWMCompMgr * mgr, XEvent *ev);
  Bool   (*my_window)         (MBWMCompMgr * mgr, Window xwin);
};

int
mb_wm_comp_mgr_class_type ();

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

void
mb_wm_comp_mgr_map_notify (MBWMCompMgr *mgr, MBWindowManagerClient *c);

void
mb_wm_comp_mgr_unmap_notify (MBWMCompMgr *mgr, MBWindowManagerClient *c);

Bool
mb_wm_comp_mgr_enabled (MBWMCompMgr *mgr);

Bool
mb_wm_comp_mgr_handle_events (MBWMCompMgr * mgr, XEvent *ev);

Bool
mb_wm_comp_mgr_is_my_window (MBWMCompMgr * mgr, Window xwin);

struct MBWMCompMgrClient
{
  MBWMObject              parent;

  MBWindowManagerClient * wm_client;

  /* Make private ? */
  MBWMList              * effects; /* List of MBWMCompMgrEffectAssociation */
  Bool                    is_argb32;
};

struct MBWMCompMgrClientClass
{
  MBWMObjectClass        parent;

  void (*show)      (MBWMCompMgrClient * client);
  void (*hide)      (MBWMCompMgrClient * client);
  void (*repair)    (MBWMCompMgrClient * client);
  void (*configure) (MBWMCompMgrClient * client);

  MBWMCompMgrEffect * (*effect_new)    (MBWMCompMgrClient * client,
					MBWMCompMgrEffectEvent event,
					MBWMCompMgrEffectType type,
					unsigned long duration,
					MBWMGravity   gravity);

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

void
mb_wm_comp_mgr_client_add_effects (MBWMCompMgrClient      * client,
				   MBWMCompMgrEffectEvent   event,
				   MBWMList               * effects);

void
mb_wm_comp_mgr_client_run_effect (MBWMCompMgrClient         * client,
				  MBWMCompMgrEffectEvent      event);

MBWMList *
mb_wm_comp_mgr_client_get_effects (MBWMCompMgrClient      * client,
				   MBWMCompMgrEffectEvent   event,
				   MBWMCompMgrEffectType    type,
				   unsigned long            duration,
				   MBWMGravity              gravity);

/*
 * Generic effect that can applied to a client
 */
struct MBWMCompMgrEffect
{
  MBWMObject              parent;
  MBWMCompMgrEffectType   type;
  unsigned long           duration;
  MBWMGravity             gravity;
};

struct MBWMCompMgrEffectClass
{
  MBWMObjectClass        parent;

  void (*run)  (MBWMList                  * effects,
		MBWMCompMgrClient         * cm_client,
		MBWMCompMgrEffectEvent      event);
};

int
mb_wm_comp_mgr_effect_class_type ();

#endif
