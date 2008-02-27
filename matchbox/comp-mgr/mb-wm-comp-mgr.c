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
#include "mb-wm-theme.h"

#include <X11/Xresource.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>

typedef struct _MBWMCompMgrEffectAssociation MBWMCompMgrEffectAssociation;

/*
 * A helper object used to translate the event-effect association defined
 * by theme into a list of effect instances.
 */
struct _MBWMCompMgrEffectAssociation
{
  MBWMCompMgrEffectEvent   event;
  MBWMList               * effects;
};

static MBWMCompMgrEffectAssociation *
mb_wm_comp_mgr_effect_association_new ();

static void
mb_wm_comp_mgr_effect_association_free (MBWMCompMgrEffectAssociation * a);

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
  XRenderPictFormat     *format;

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

  /* Check visual */
  format = XRenderFindVisualFormat (wm_client->wmref->xdpy,
				    wm_client->window->visual);

  if (format && format->type == PictTypeDirect && format->direct.alphaMask)
    client->is_argb32 = True;

  return 1;
}

static void
mb_wm_comp_mgr_client_destroy (MBWMObject* obj)
{
  MBWMList * l = MB_WM_COMP_MGR_CLIENT (obj)->effects;

  while (l)
    {
      MBWMCompMgrEffectAssociation * a = l->data;
      mb_wm_comp_mgr_effect_association_free (a);

      l = l->next;
    }
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
  MBWMCompMgrClientClass *klass;

  if (!client)
    return;

  klass = MB_WM_COMP_MGR_CLIENT_CLASS (MB_WM_OBJECT_GET_CLASS (client));

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

/*
 * Method for sub-classes to add effects to the (private) list
 */
void
mb_wm_comp_mgr_client_add_effects (MBWMCompMgrClient      * client,
				   MBWMCompMgrEffectEvent   event,
				   MBWMList               * effects)
{
  MBWMCompMgr       * mgr    = client->wm_client->wmref->comp_mgr;
  MBWMCompMgrEffectAssociation * a;

  MBWM_ASSERT (mgr);

  a = mb_wm_comp_mgr_effect_association_new ();

  a->effects = effects;
  a->event   = event;

  client->effects = mb_wm_util_list_prepend (client->effects, a);
}

/*
 * Runs all effects associated with this client for the given events.
 *
 * completed_cb is a callback function that will get called when the effect
 * execution is completed (can be NULL), data is user data that will be passed
 * to the callback function.
 *
 * For exmaple of use see mb_wm_client_iconize().w
 */
void
mb_wm_comp_mgr_client_run_effect (MBWMCompMgrClient         * client,
				  MBWMCompMgrEffectEvent      event,
				  MBWMCompMgrEffectCallback   completed_cb,
				  void                      * data)
{
  MBWMList               * l = client->effects;
  Bool                     done_effect = False;
  MBWMCompMgrEffectClass * eklass;

  if (!client)
    return;

  /*
   * Take a reference to the CM client object, so that it does not disappear
   * underneath us while the effect is running; it is the responsibility
   * of the subclassed run() to release this reference when it is done.
   */
  mb_wm_object_ref (MB_WM_OBJECT (client));

  while (l)
    {
      MBWMCompMgrEffectAssociation * a = l->data;

      if (a->event == event)
	{
	  MBWMList * el = a->effects;
	  MBWMCompMgrEffect * eff = el->data;

	  eklass = MB_WM_COMP_MGR_EFFECT_CLASS (MB_WM_OBJECT_GET_CLASS (eff));

	  if (!eklass)
	    continue;

	  eklass->run (el, client, event, completed_cb, data);
	  done_effect = True;
	}

      l = l->next;
    }

  /*
   * If there were no effects to run for this event, we manually call
   * the callback to signal the effect is completed
   */
  if (!done_effect && completed_cb)
    completed_cb (data);
}

/*
 * MBWMCompMgr object
 *
 * Base class for the composite manager, providing the common interface
 * through which the manager is access by the MBWindowManager object.
 */
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

/*
 * Registers new client with the manager (i.e., when a window is created).
 */
void
mb_wm_comp_mgr_register_client (MBWMCompMgr           * mgr,
				MBWindowManagerClient * client)
{
  MBWMCompMgrClass *klass
    = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  MBWM_ASSERT (klass->register_client != NULL);
  klass->register_client (mgr, client);
}

/*
 * Unregisters existing client (e.g., when window unmaps or is destroyed
 */
void
mb_wm_comp_mgr_unregister_client (MBWMCompMgr           * mgr,
				  MBWindowManagerClient * client)
{
  MBWMCompMgrClass *klass
    = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  if (klass->unregister_client)
    klass->unregister_client (mgr, client);

  mb_wm_object_unref (MB_WM_OBJECT (client->cm_client));
  client->cm_client = NULL;
}

/*
 * Called to render the entire composited scene on screen.
 */
void
mb_wm_comp_mgr_render (MBWMCompMgr *mgr)
{
  MBWMCompMgrClass *klass;

  if (!mgr)
    return;

  klass  = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  MBWM_ASSERT (klass->render != NULL);
  klass->render (mgr);
}

/*
 * Called when a window we are interested in maps.
 */
void
mb_wm_comp_mgr_map_notify (MBWMCompMgr *mgr, MBWindowManagerClient *c)
{
  MBWMCompMgrClass *klass;

  if (!mgr || !c || !c->cm_client)
    return;

  klass = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  if (klass->map_notify)
    klass->map_notify (mgr, c);

  /*
   * Run map event effect *before* we call show() on the actor
   * (the effect will take care of showing the actor, and if not, show() gets
   * called by mb_wm_comp_mgr_map_notify()).
   */
  mb_wm_comp_mgr_client_run_effect (c->cm_client,
				    MBWMCompMgrEffectEventMap, NULL, NULL);

  if (c->cm_client)
    mb_wm_comp_mgr_client_show (c->cm_client);
}

void
mb_wm_comp_mgr_unmap_notify (MBWMCompMgr *mgr, MBWindowManagerClient *c)
{
  MBWMCompMgrClass *klass;

  if (!mgr || !c || !c->cm_client)
    return;

  klass = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  mb_wm_comp_mgr_client_run_effect (c->cm_client,
				    MBWMCompMgrEffectEventUnmap, NULL, NULL);

  if (klass->unmap_notify)
    klass->unmap_notify (mgr, c);

  /*
   * NB: cannot call hide() here, as at this point an effect could be running
   *     -- the subclass needs to take care of this from the effect and/or
   *     unmap_notify() function.
   */
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

/*
 * Carries out any processing of X events that is specific to the
 * compositing manager (such as XDamage events).
 */
Bool
mb_wm_comp_mgr_handle_events (MBWMCompMgr * mgr, XEvent *ev)
{
  MBWMCompMgrClass *klass
    = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  MBWM_ASSERT (klass->handle_events != NULL);
  return klass->handle_events (mgr, ev);
}

/*
 * Returns true if the window identified by id xwin is a special window
 * that belongs to the compositing manager and should, therefore, be left
 * alone by MBWMWindow manager (e.g., the overalay window).
 */
Bool
mb_wm_comp_mgr_is_my_window (MBWMCompMgr * mgr, Window xwin)
{
  MBWMCompMgrClass *klass
    = MB_WM_COMP_MGR_CLASS (MB_WM_OBJECT_GET_CLASS (mgr));

  if (klass->my_window)
    return klass->my_window (mgr, xwin);

  return False;
}

/*
 * Returns a list of MBWMCompMgrEffect objects of given type that are to be
 * associated with the given client for the given event. The type parameter
 * can be made up of more that one MBWMComMgrEffectType value, or-ed.
 *
 * This function provides a public API to translate the effects specified
 * by theme into actual effect instances.
 */
MBWMList *
mb_wm_comp_mgr_client_get_effects (MBWMCompMgrClient      * client,
				   MBWMCompMgrEffectEvent   event,
				   MBWMCompMgrEffectType    type,
				   unsigned long            duration,
				   MBWMGravity              gravity)
{
  MBWMList                *l = NULL;
  MBWMCompMgrEffectType    t = (MBWMCompMgrEffectType) 1;
  MBWMCompMgrClientClass  *klass =
    MB_WM_COMP_MGR_CLIENT_CLASS (MB_WM_OBJECT_GET_CLASS (client));

  if (!klass->effect_new)
    return NULL;

  while (t < _MBWMCompMgrEffectLast)
    {
      MBWMCompMgrEffect *e = NULL;

      if (t & type)
	e = klass->effect_new (client, event, t, duration, gravity);

      if (e)
	l = mb_wm_util_list_prepend (l, e);

      t <<= 1;
    }

  return l;
}


/*
 * Effect
 *
 * Effects are simple one-off transformations carried out on the client in
 * response to some trigger event. The possible events are given by the
 * MBWMCompMgrEffectEvent enum, while the nature effect is defined by
 * MBWMCompMgrEffectType enum; effects are specified per window manager client
 * type by the theme.
 *
 * Events and effect types form a one to many association (and the type
 * enumeration is or-able).
 *
 * The base MBWMCompMgrEffect class provides some common infrastructure,
 * notably the virtual run() method that needs to be implemented by
 * the derrived classes.
 *
 * NB: the base class does not automatically associate effects with clients;
 * this is the responsibility of the subclasses, mostly because the sub classes
 * might want to allocate the effect objects at different times (for example,
 * the MBWMCompMgrClutterEffect objects cannot be allocated until the
 * the associated client has created its ClutterActor, which only happens when
 * the client window maps).
 */
static MBWMCompMgrEffectAssociation *
mb_wm_comp_mgr_effect_association_new ()
{
  void * a;

  a = mb_wm_util_malloc0 (sizeof (MBWMCompMgrEffectAssociation));

  return (MBWMCompMgrEffectAssociation*) a;
}

static void
mb_wm_comp_mgr_effect_association_free (MBWMCompMgrEffectAssociation * a)
{
  MBWMList * l = a->effects;

  while (l)
    {
      MBWMCompMgrEffect * e = l->data;
      mb_wm_object_unref (MB_WM_OBJECT (e));

      l = l->next;
    }

  free (a);
}

static void
mb_wm_comp_mgr_effect_class_init (MBWMObjectClass *klass)
{
  MBWMCompMgrEffectClass *c_klass = MB_WM_COMP_MGR_EFFECT_CLASS (klass);

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMCompMgrEffect";
#endif
}

static int
mb_wm_comp_mgr_effect_init (MBWMObject *obj, va_list vap)
{
  MBWMObjectProp         prop;
  MBWMCompMgrEffectType  type = 0;
  unsigned long          duration = 0;
  MBWMGravity            gravity = MBWMGravityNone;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropCompMgrEffectType:
	  type = va_arg(vap, MBWMCompMgrEffectType);
	  break;
	case MBWMObjectPropCompMgrEffectDuration:
	  duration = va_arg(vap, unsigned long);
	  break;
	case MBWMObjectPropCompMgrEffectGravity:
	  gravity = va_arg(vap, MBWMGravity);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  if (!type)
    return 0;

  MB_WM_COMP_MGR_EFFECT (obj)->type     = type;
  MB_WM_COMP_MGR_EFFECT (obj)->duration = duration;
  MB_WM_COMP_MGR_EFFECT (obj)->gravity  = gravity;

  return 1;
}

static void
mb_wm_comp_mgr_effect_destroy (MBWMObject* obj)
{
}

int
mb_wm_comp_mgr_effect_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMCompMgrEffectClass),
	sizeof (MBWMCompMgrEffect),
	mb_wm_comp_mgr_effect_init,
	mb_wm_comp_mgr_effect_destroy,
	mb_wm_comp_mgr_effect_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}
