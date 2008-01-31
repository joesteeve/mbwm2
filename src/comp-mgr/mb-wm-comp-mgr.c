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

#include "mb-wm.h"
#include "mb-wm-client.h"
#include "mb-wm-comp-mgr.h"

static void
mb_wm_comp_mgr_client_class_init (MBWMObjectClass *klass)
{
  MBWMCompMgrClientClass *c_klass = MB_WM_COMP_MGR_CLIENT_CLASS (klass);

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMCompMgrClient";
#endif
}

static int
mb_wm_comp_mgr_client_init (MBWMObject *obj, va_list vap)
{
  MBWMCompMgrClient     *client    = MB_WM_COMP_MGR_CLIENT (obj);
  MBWindowManagerClient *wm_client = NULL;
  MBWMObjectProp         prop;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropClient:
	  wm_client = va_arg(vap, MBWindowManagerClient *);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  if (!wm_client)
    return 0;

  client->wm_client = wm_client;

  return 1;
}

static void
mb_wm_comp_mgr_client_destroy (MBWMObject* obj)
{
}

int
mb_wm_comp_mgr_client_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMCompMgrClientClass),
	sizeof (MBWMCompMgrClient),
	mb_wm_comp_mgr_client_init,
	mb_wm_comp_mgr_client_destroy,
	mb_wm_comp_mgr_client_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

void
mb_wm_comp_mgr_client_hide (MBWMCompMgrClient * client)
{
  MBWMCompMgrClientClass *klass
    = MB_WM_COMP_MGR_CLIENT_CLASS (MB_WM_OBJECT_GET_CLASS (client));

  MBWM_ASSERT (klass->hide != NULL);
  klass->hide (client);
}

void
mb_wm_comp_mgr_client_show (MBWMCompMgrClient * client)
{
  MBWMCompMgrClientClass *klass
    = MB_WM_COMP_MGR_CLIENT_CLASS (MB_WM_OBJECT_GET_CLASS (client));

  MBWM_ASSERT (klass->show != NULL);
  klass->show (client);
}

void
mb_wm_comp_mgr_client_configure (MBWMCompMgrClient * client)
{
  MBWMCompMgrClientClass *klass
    = MB_WM_COMP_MGR_CLIENT_CLASS (MB_WM_OBJECT_GET_CLASS (client));

  MBWM_ASSERT (klass->configure != NULL);
  klass->configure (client);
}

void
mb_wm_comp_mgr_client_repair (MBWMCompMgrClient * client)
{
  MBWMCompMgrClientClass *klass
    = MB_WM_COMP_MGR_CLIENT_CLASS (MB_WM_OBJECT_GET_CLASS (client));

  MBWM_ASSERT (klass->repair != NULL);
  klass->repair (client);
}

static void
mb_wm_comp_mgr_class_init (MBWMObjectClass *klass)
{
#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMCompMgr";
#endif
}

static int
mb_wm_comp_mgr_init (MBWMObject *obj, va_list vap)
{
  MBWMCompMgr           * mgr = MB_WM_COMP_MGR (obj);
  MBWindowManager       * wm  = NULL;
  MBWMObjectProp          prop;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropWm:
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  if (!wm)
    return 0;

  mgr->wm       = wm;
  mgr->disabled = True;

  return 1;
}

static void
mb_wm_comp_mgr_destroy (MBWMObject* obj)
{
  MBWMCompMgr * mgr = MB_WM_COMP_MGR (obj);

  mb_wm_comp_mgr_turn_off (mgr);
}

int
mb_wm_comp_mgr_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMCompMgrClass),
	sizeof (MBWMCompMgr),
	mb_wm_comp_mgr_init,
	mb_wm_comp_mgr_destroy,
	mb_wm_comp_mgr_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}


Bool
mb_wm_comp_mgr_enabled (MBWMCompMgr *mgr)
{
  if (!mgr || mgr->disabled)
    return False;

  return True;
}

void
mb_wm_comp_mgr_register_client (MBWMCompMgr           * mgr,
				MBWindowManagerClient * client)
{
  MBWMCompMgrClass *klass
    = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  MBWM_ASSERT (klass->register_client != NULL);
  klass->register_client (mgr, client);
}

void
mb_wm_comp_mgr_unregister_client (MBWMCompMgr           * mgr,
				  MBWindowManagerClient * client)
{
  MBWMCompMgrClass *klass
    = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  MBWM_ASSERT (klass->unregister_client != NULL);
  klass->unregister_client (mgr, client);
}

void
mb_wm_comp_mgr_render (MBWMCompMgr *mgr)
{
  MBWMCompMgrClass *klass
    = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  MBWM_ASSERT (klass->render != NULL);
  klass->render (mgr);
}

void
mb_wm_comp_mgr_turn_on (MBWMCompMgr *mgr)
{
  MBWMCompMgrClass *klass
    = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  MBWM_ASSERT (klass->turn_on != NULL);
  klass->turn_on (mgr);
}

void
mb_wm_comp_mgr_turn_off (MBWMCompMgr *mgr)
{
  MBWMCompMgrClass *klass
    = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  MBWM_ASSERT (klass->turn_off != NULL);
  klass->turn_off (mgr);
}

Bool
mb_wm_comp_mgr_handle_events (MBWMCompMgr * mgr, XEvent *ev)
{
  MBWMCompMgrClass *klass
    = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  MBWM_ASSERT (klass->handle_events != NULL);
  return klass->handle_events (mgr, ev);
}

