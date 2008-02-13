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

#include "mb-wm.h"
#include "mb-wm-client.h"
#include "mb-wm-comp-mgr.h"
#include "mb-wm-comp-mgr-clutter.h"
#include "mb-wm-theme.h"

#include <clutter/clutter.h>
#include <clutter/clutter-x11.h>
#include <clutter/clutter-x11-texture-pixmap.h>
#include <clutter/clutter-glx-texture-pixmap.h>

#include <X11/Xresource.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/shape.h>

static void
mb_wm_comp_mgr_clutter_add_actor (MBWMCompMgrClutter * , ClutterActor *);

/*
 * A helper object to store manager's per-client data
 */
struct MBWMCompMgrClutterClient
{
  MBWMCompMgrClient       parent;
  ClutterActor          * actor;
  Pixmap                  pixmap;
  int                     pxm_width;
  int                     pxm_height;
  int                     pxm_depth;
  Bool                    mapped;
  Damage	          damage;

  /* 1-based array holding timelines for effect events */
  ClutterTimeline       * timelines[_MBWMCompMgrEffectEventLast-1];
};

static void
mb_wm_comp_mgr_clutter_client_show_real (MBWMCompMgrClient * client);

static void
mb_wm_comp_mgr_clutter_client_hide_real (MBWMCompMgrClient * client);

static void
mb_wm_comp_mgr_clutter_client_repair_real (MBWMCompMgrClient * client);

static void
mb_wm_comp_mgr_clutter_client_configure_real (MBWMCompMgrClient * client);

static MBWMCompMgrEffect *
mb_wm_comp_mgr_clutter_client_effect_new_real (MBWMCompMgrClient *client,
					       MBWMCompMgrEffectEvent event,
					       MBWMCompMgrEffectType type,
					       unsigned long duration);
static void
mb_wm_comp_mgr_clutter_client_class_init (MBWMObjectClass *klass)
{
  MBWMCompMgrClientClass *c_klass = MB_WM_COMP_MGR_CLIENT_CLASS (klass);

  c_klass->show       = mb_wm_comp_mgr_clutter_client_show_real;
  c_klass->hide       = mb_wm_comp_mgr_clutter_client_hide_real;
  c_klass->repair     = mb_wm_comp_mgr_clutter_client_repair_real;
  c_klass->configure  = mb_wm_comp_mgr_clutter_client_configure_real;
  c_klass->effect_new = mb_wm_comp_mgr_clutter_client_effect_new_real;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMCompMgrClutterClient";
#endif
}

/*
 * Fetch the entire texture for our client
 */
static void
mb_wm_comp_mgr_clutter_fetch_texture (MBWMCompMgrClient *client)
{
  MBWMCompMgrClutterClient  *cclient  = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);
  MBWindowManagerClient     *wm_client = client->wm_client;
  MBWindowManager           *wm        = wm_client->wmref;
  Window                     xwin;
  Window root;
  int x, y, w, h, bw, depth;

  if (!cclient->mapped)
    return;

  xwin =
    wm_client->xwin_frame ? wm_client->xwin_frame : wm_client->window->xwindow;

  if (cclient->pixmap)
    XFreePixmap (wm->xdpy, cclient->pixmap);

  cclient->pixmap = XCompositeNameWindowPixmap (wm->xdpy, xwin);

  if (!cclient->pixmap)
    return;

  XGetGeometry (wm->xdpy, cclient->pixmap, &root, &x, &y, &w, &h, &bw, &depth);

  cclient->pxm_width  = w;
  cclient->pxm_height = h;
  cclient->pxm_depth  = depth;

  clutter_actor_set_size (cclient->actor, w, h);

  clutter_x11_texture_pixmap_set_pixmap (
				CLUTTER_X11_TEXTURE_PIXMAP (cclient->actor),
				cclient->pixmap,
				w, h, depth);
}

/*
 * Update region x,y,width x height in our client texture.
 */
static void
mb_wm_comp_mgr_clutter_update_texture (MBWMCompMgrClient *client,
				       int x, int y, int width, int height)
{
  MBWMCompMgrClutterClient  *cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);
  MBWindowManagerClient     *wm_client = client->wm_client;
  MBWindowManager           *wm        = wm_client->wmref;

  if (!cclient->mapped)
    return;

  if (!cclient->pixmap)
    {
      mb_wm_comp_mgr_clutter_fetch_texture (client);
      return;
    }

  clutter_x11_texture_pixmap_update_area (
				CLUTTER_X11_TEXTURE_PIXMAP (cclient->actor),
				x, y, width, height);
}

static int
mb_wm_comp_mgr_clutter_client_init (MBWMObject *obj, va_list vap)
{
  return 1;
}

static void
mb_wm_comp_mgr_clutter_client_destroy (MBWMObject* obj)
{
  MBWMCompMgrClient        * c  = MB_WM_COMP_MGR_CLIENT (obj);
  MBWMCompMgrClutterClient * cc = MB_WM_COMP_MGR_CLUTTER_CLIENT (obj);
  MBWindowManager          * wm = c->wm_client->wmref;

  mb_wm_comp_mgr_client_hide (c);

  if (cc->actor)
    g_object_unref (cc->actor);

  if (cc->pixmap)
    XFreePixmap (wm->xdpy, cc->pixmap);

  if (cc->damage)
    XDamageDestroy (wm->xdpy, cc->damage);
}

int
mb_wm_comp_mgr_clutter_client_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMCompMgrClutterClientClass),
	sizeof (MBWMCompMgrClutterClient),
	mb_wm_comp_mgr_clutter_client_init,
	mb_wm_comp_mgr_clutter_client_destroy,
	mb_wm_comp_mgr_clutter_client_class_init
      };

      type =
	mb_wm_object_register_class (&info, MB_WM_TYPE_COMP_MGR_CLIENT, 0);
    }

  return type;
}

/*
 * This is a private method, hence static (all instances of this class are
 * created automatically by the composite manager).
 */
static MBWMCompMgrClient *
mb_wm_comp_mgr_clutter_client_new (MBWindowManagerClient * client)
{
  MBWMObject *c;

  c = mb_wm_object_new (MB_WM_TYPE_COMP_MGR_CLUTTER_CLIENT,
			MBWMObjectPropClient, client,
			NULL);

  return MB_WM_COMP_MGR_CLIENT (c);
}

static void
mb_wm_comp_mgr_clutter_client_hide_real (MBWMCompMgrClient * client)
{
  MBWMCompMgrClutterClient * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);

  clutter_actor_hide (cclient->actor);
}

/*
 * Helper function for manipulating damage. Gets the extents of this client,
 * expressed as XserverRegion.
 */
static XserverRegion
mb_wm_comp_mgr_clutter_client_extents (MBWMCompMgrClient *client)
{
  MBWindowManagerClient     *wm_client = client->wm_client;
  MBWindowManager           *wm = wm_client->wmref;
  MBGeometry                 geom;
  XRectangle	             r;
  XserverRegion              extents;

  mb_wm_client_get_coverage (wm_client, &geom);

  r.x      = geom.x;
  r.y      = geom.y;
  r.width  = geom.width;
  r.height = geom.height;

  extents = XFixesCreateRegion (wm->xdpy, &r, 1);

  return extents;
}

/*
 * Adds region damage to the overall damage of the client.
 */
static void
mb_wm_comp_mgr_clutter_client_add_damage (MBWMCompMgrClutterClient * cclient,
					  XserverRegion damage)
{
  MBWindowManagerClient     *wm_client =
    MB_WM_COMP_MGR_CLIENT (cclient)->wm_client;
  MBWindowManager           *wm = wm_client->wmref;

  XFixesUnionRegion (wm->xdpy,
		     cclient->damage,
		     cclient->damage,
		     damage);

  XFixesDestroyRegion (wm->xdpy, damage);

  mb_wm_display_sync_queue (wm, MBWMSyncVisibility);
}

static void
mb_wm_comp_mgr_clutter_client_show_real (MBWMCompMgrClient * client)
{
  MBWMCompMgrClutterClient * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);
  MBGeometry                 geom;
  XserverRegion              region;
  MBWindowManagerClient     *wm_client =
    MB_WM_COMP_MGR_CLIENT (cclient)->wm_client;
  MBWindowManager           *wm = wm_client->wmref;

  MBWM_NOTE (COMPOSITOR, "showing client");

  if (!cclient->actor)
    {
      /*
       * This can happen if show() is called on our client before it is
       * actually mapped (we only alocate the actor in response to map
       * notification.
       */
      return;
    }

  if (!cclient->damage)
    cclient->damage = XDamageCreate (wm->xdpy,
				     wm_client->xwin_frame ?
				     wm_client->xwin_frame :
				     wm_client->window->xwindow,
				     XDamageReportNonEmpty);

  /*
   * FIXME -- is it really necessary to invalidate the entire client area
   * here ?
   */
  region = mb_wm_comp_mgr_clutter_client_extents (client);

  mb_wm_comp_mgr_clutter_client_add_damage (cclient, region);

  clutter_actor_show (cclient->actor);
}

static MBWMCompMgrEffect *
mb_wm_comp_mgr_clutter_effect_new (MBWMCompMgrEffectType    type,
				   unsigned long            duration,
				   ClutterTimeline        * timeline,
				   ClutterBehaviour       * behaviour);

/*
 * Helper method to get a timeline for given event (all effects associated with
 * the same event share a single timeline.
 */
static ClutterTimeline *
mb_wm_comp_mgr_clutter_client_get_timeline (MBWMCompMgrClient      *client,
					    MBWMCompMgrEffectEvent  event,
					    unsigned long           duration)
{
  MBWMCompMgrClutterClient * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);

  if (!client || event >= _MBWMCompMgrEffectEventLast)
    return NULL;

  if (!cclient->timelines[event-1])
    {
      cclient->timelines[event-1] =
	clutter_timeline_new_for_duration (duration);
    }

  return cclient->timelines[event-1];
}

/*
 * Implementation of the MBWMCompMgrClient->effect_new() method.
 */
static MBWMCompMgrEffect *
mb_wm_comp_mgr_clutter_client_effect_new_real (MBWMCompMgrClient *client,
					       MBWMCompMgrEffectEvent event,
					       MBWMCompMgrEffectType type,
					       unsigned long duration)
{
  MBWMCompMgrEffect        * eff;
  ClutterTimeline          * timeline;
  ClutterBehaviour         * behaviour;
  ClutterAlpha             * alpha;
  MBWMCompMgrClutterClient * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);

  timeline =
    mb_wm_comp_mgr_clutter_client_get_timeline (client, event, duration);

  if (!timeline)
    return NULL;

  alpha = clutter_alpha_new_full (timeline,
				  CLUTTER_ALPHA_RAMP_INC, NULL, NULL);

  switch (type)
    {
    case MBWMCompMgrEffectScaleUp:
      behaviour =
	clutter_behaviour_scale_newx (alpha, 0, 0, CFX_ONE, CFX_ONE);
      break;
    case MBWMCompMgrEffectScaleDown:
      behaviour =
	clutter_behaviour_scale_newx (alpha, CFX_ONE, CFX_ONE, 0, 0);
      break;
    case MBWMCompMgrEffectSpinXCW:
      behaviour = clutter_behaviour_rotate_newx (alpha,
						 CLUTTER_X_AXIS,
						 CLUTTER_ROTATE_CW,
						 0,
						 CLUTTER_INT_TO_FIXED (360));
      break;
    case MBWMCompMgrEffectSpinXCCW:
      behaviour = clutter_behaviour_rotate_newx (alpha,
						 CLUTTER_X_AXIS,
						 CLUTTER_ROTATE_CCW,
						 0,
						 CLUTTER_INT_TO_FIXED (360));
      break;
    case MBWMCompMgrEffectSpinYCW:
      behaviour = clutter_behaviour_rotate_newx (alpha,
						 CLUTTER_Y_AXIS,
						 CLUTTER_ROTATE_CW,
						 0,
						 CLUTTER_INT_TO_FIXED (360));
      break;
    case MBWMCompMgrEffectSpinYCCW:
      behaviour = clutter_behaviour_rotate_newx (alpha,
						 CLUTTER_Y_AXIS,
						 CLUTTER_ROTATE_CCW,
						 0,
						 CLUTTER_INT_TO_FIXED (360));
      break;
    case MBWMCompMgrEffectSpinZCW:
      behaviour = clutter_behaviour_rotate_newx (alpha,
						 CLUTTER_Z_AXIS,
						 CLUTTER_ROTATE_CW,
						 0,
						 CLUTTER_INT_TO_FIXED (360));
      break;
    case MBWMCompMgrEffectSpinZCCW:
      behaviour = clutter_behaviour_rotate_newx (alpha,
						 CLUTTER_Z_AXIS,
						 CLUTTER_ROTATE_CCW,
						 0,
						 CLUTTER_INT_TO_FIXED (360));
      break;
    case MBWMCompMgrEffectFade:
      behaviour = clutter_behaviour_opacity_new (alpha, 0xff, 0);
      break;
    case MBWMCompMgrEffectUnfade:
      behaviour = clutter_behaviour_opacity_new (alpha, 0, 0xff);
      break;

      /* FIXME -- add the path behaviours here */
    case MBWMCompMgrEffectSlideN:
      break;
    case MBWMCompMgrEffectSlideS:
      break;
    case MBWMCompMgrEffectSlideE:
      break;
    case MBWMCompMgrEffectSlideW:
      break;
    case MBWMCompMgrEffectSlideNW:
      break;
    case MBWMCompMgrEffectSlideNE:
      break;
    case MBWMCompMgrEffectSlideSW:
      break;
    case MBWMCompMgrEffectSlideSE:
      break;
    default:
      ;
    }

  eff =
    mb_wm_comp_mgr_clutter_effect_new (type, duration, timeline, behaviour);

  if (eff)
    {
      /*
       * We assume that the actor already exists -- if not, clutter will
       * issue a warning here
       */
      clutter_behaviour_apply (behaviour, cclient->actor);
    }

  return eff;
}

/*
 * Implementation of MBWMCompMgrClutter
 */
struct MBWMCompMgrClutterPrivate
{
  ClutterActor * stage;
  Window         overlay_window;
  int            damage_event;
};

static void
mb_wm_comp_mgr_clutter_private_free (MBWMCompMgrClutter *mgr)
{
  MBWMCompMgrClutterPrivate * priv = mgr->priv;
  free (priv);
}

static void
mb_wm_comp_mgr_clutter_register_client_real (MBWMCompMgr           * mgr,
					     MBWindowManagerClient * c)
{
  MBWMCompMgrClient  *cc;
  MBWMCompMgrClutter *cmgr = MB_WM_COMP_MGR_CLUTTER (mgr);

  if (c->cm_client)
    return;

  cc = mb_wm_comp_mgr_clutter_client_new (c);
  c->cm_client = cc;
}

static void
mb_wm_comp_mgr_clutter_unregister_client_real (MBWMCompMgr           * mgr,
					       MBWindowManagerClient * client)
{
  if (!client || !client->cm_client)
    return;

  mb_wm_object_unref (MB_WM_OBJECT (client->cm_client));
  client->cm_client = NULL;
}

static void
mb_wm_comp_mgr_clutter_turn_on_real (MBWMCompMgr *mgr);

static void
mb_wm_comp_mgr_clutter_turn_off_real (MBWMCompMgr *mgr);

static void
mb_wm_comp_mgr_clutter_render_real (MBWMCompMgr *mgr);

static void
mb_wm_comp_mgr_clutter_map_notify_real (MBWMCompMgr *mgr,
					MBWindowManagerClient *c);

static Bool
mb_wm_comp_mgr_clutter_handle_events_real (MBWMCompMgr * mgr, XEvent *ev);

static Bool
mb_wm_comp_mgr_is_my_window_real (MBWMCompMgr * mgr, Window xwin);

static void
mb_wm_comp_mgr_clutter_class_init (MBWMObjectClass *klass)
{
  MBWMCompMgrClass *cm_klass = MB_WM_COMP_MGR_CLASS (klass);

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMCompMgrClutter";
#endif

  cm_klass->register_client   = mb_wm_comp_mgr_clutter_register_client_real;
  cm_klass->unregister_client = mb_wm_comp_mgr_clutter_unregister_client_real;
  cm_klass->turn_on           = mb_wm_comp_mgr_clutter_turn_on_real;
  cm_klass->turn_off          = mb_wm_comp_mgr_clutter_turn_off_real;
  cm_klass->render            = mb_wm_comp_mgr_clutter_render_real;
  cm_klass->map_notify        = mb_wm_comp_mgr_clutter_map_notify_real;
  cm_klass->handle_events     = mb_wm_comp_mgr_clutter_handle_events_real;
  cm_klass->my_window         = mb_wm_comp_mgr_is_my_window_real;
}

/*
 * Initializes the extension require by the manager
 */
static Bool
mb_wm_comp_mgr_clutter_init_extensions (MBWMCompMgr *mgr)
{
  MBWindowManager             * wm = mgr->wm;
  MBWMCompMgrClutterPrivate   * priv = MB_WM_COMP_MGR_CLUTTER (mgr)->priv;
  int		                event_base, error_base;
  int		                damage_error;
  int		                xfixes_event, xfixes_error;

  if (!XCompositeQueryExtension (wm->xdpy, &event_base, &error_base))
    {
      fprintf (stderr, "matchbox: No composite extension\n");
      return False;
    }

  if (!XDamageQueryExtension (wm->xdpy, &priv->damage_event, &damage_error))
    {
      fprintf (stderr, "matchbox: No damage extension\n");
      return False;
    }

  if (!XFixesQueryExtension (wm->xdpy, &xfixes_event, &xfixes_error))
    {
      fprintf (stderr, "matchbox: No XFixes extension\n");
      return False;
    }

  XCompositeRedirectSubwindows (wm->xdpy, wm->root_win->xwindow,
				CompositeRedirectManual);

  return True;
}

static int
mb_wm_comp_mgr_clutter_init (MBWMObject *obj, va_list vap)
{
  MBWMCompMgr                * mgr  = MB_WM_COMP_MGR (obj);
  MBWMCompMgrClutter         * cmgr = MB_WM_COMP_MGR_CLUTTER (obj);
  MBWMCompMgrClutterPrivate  * priv;

  priv = mb_wm_util_malloc0 (sizeof (MBWMCompMgrClutterPrivate));
  cmgr->priv = priv;

  if (!mb_wm_comp_mgr_clutter_init_extensions (mgr))
    return 0;

  priv->stage = clutter_stage_get_default ();

  return 1;
}

static void
mb_wm_comp_mgr_clutter_destroy (MBWMObject * obj)
{
  MBWMCompMgr        * mgr  = MB_WM_COMP_MGR (obj);
  MBWMCompMgrClutter * cmgr = MB_WM_COMP_MGR_CLUTTER (obj);

  mb_wm_comp_mgr_turn_off (mgr);
  mb_wm_comp_mgr_clutter_private_free (cmgr);
}

int
mb_wm_comp_mgr_clutter_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMCompMgrClutterClass),
	sizeof (MBWMCompMgrClutter),
	mb_wm_comp_mgr_clutter_init,
	mb_wm_comp_mgr_clutter_destroy,
	mb_wm_comp_mgr_clutter_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_COMP_MGR, 0);
    }

  return type;
}

/* Shuts the compositing down */
static void
mb_wm_comp_mgr_clutter_turn_off_real (MBWMCompMgr *mgr)
{
  MBWindowManager            * wm = mgr->wm;
  MBWMCompMgrClutterPrivate  * priv;

  if (!mgr)
    return;

  priv = MB_WM_COMP_MGR_CLUTTER (mgr)->priv;

  if (mgr->disabled)
    return;

  if (!mb_wm_stack_empty (wm))
    {
      MBWindowManagerClient * c;

      mb_wm_stack_enumerate (wm, c)
	{
	  mb_wm_comp_mgr_clutter_unregister_client_real (mgr, c);
	}
    }

  XCompositeReleaseOverlayWindow (wm->xdpy, wm->root_win->xwindow);
  priv->overlay_window = None;

  mgr->disabled = True;
}

static void
mb_wm_comp_mgr_clutter_turn_on_real (MBWMCompMgr *mgr)
{
  MBWindowManager            * wm;
  MBWMCompMgrClutterPrivate  * priv;

  if (!mgr || !mgr->disabled)
    return;

  priv = MB_WM_COMP_MGR_CLUTTER (mgr)->priv;
  wm = mgr->wm;

  mgr->disabled = False;

  if (priv->overlay_window == None)
    {
      ClutterActor    * stage = priv->stage;
#ifdef MBWM_WANT_DEBUG
      /*
       * Special colour to please Iain's eyes ;)
       */
      ClutterColor      clr = {0xff, 0, 0xff, 0xff };
#else
      ClutterColor      clr = {0, 0, 0, 0xff };
#endif
      Window            xwin;
      XserverRegion     region;

      /*
       * Fetch the overlay window
       */
      xwin = clutter_x11_get_stage_window (CLUTTER_STAGE (stage));

      priv->overlay_window =
	XCompositeGetOverlayWindow (wm->xdpy, wm->root_win->xwindow);

      /*
       * Reparent the stage window to the overlay window, this makes it
       * magically work :)
       */
      XReparentWindow (wm->xdpy, xwin, priv->overlay_window, 0, 0);

      /*
       * Use xfixes shape to make events pass through the overlay window
       *
       * TODO -- this has certain drawbacks, notably when our client is
       * tranformed (rotated, scaled, etc), the events will not be landing in
       * the right place. The answer to that is event forwarding with
       * translation.
       */
      region = XFixesCreateRegion (wm->xdpy, NULL, 0);

      XFixesSetWindowShapeRegion (wm->xdpy, priv->overlay_window,
				  ShapeBounding, 0, 0, 0);
      XFixesSetWindowShapeRegion (wm->xdpy, priv->overlay_window,
				  ShapeInput, 0, 0, region);

      XFixesDestroyRegion (wm->xdpy, region);

      clutter_actor_set_size (stage, wm->xdpy_width, wm->xdpy_height);
      clutter_stage_set_color (CLUTTER_STAGE (stage), &clr);

      clutter_actor_show (stage);
    }

  /*
   * Take care of any pre-existing windows
   */
  if (!mb_wm_stack_empty (wm))
    {
      MBWindowManagerClient * c;

      mb_wm_stack_enumerate (wm, c)
	{
	  mb_wm_comp_mgr_clutter_register_client_real (mgr, c);

	  /*
	   * Need to call map_notify here manually, since we will have missed
	   * the original map notification when this client mapped, and
	   * we relly on it to create our actor.
	   */
	  mb_wm_comp_mgr_clutter_map_notify_real (mgr, c);
	}
    }
}

static void
mb_wm_comp_mgr_clutter_client_repair_real (MBWMCompMgrClient * client)
{
  MBWindowManagerClient    * wm_client = client->wm_client;
  MBWMCompMgrClutterClient * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);

  MBWM_NOTE (COMPOSITOR, "REPAIRING %x", wm_client->window->xwindow);

  if (!cclient->actor)
    return;

  XDamageSubtract (wm_client->wmref->xdpy,
		   cclient->damage, None, None );

  mb_wm_comp_mgr_clutter_fetch_texture (client);
}

static void
mb_wm_comp_mgr_clutter_client_configure_real (MBWMCompMgrClient * client)
{
  MBWindowManagerClient    * wm_client = client->wm_client;
  MBWMCompMgrClutterClient * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);

  MBWM_NOTE (COMPOSITOR, "CONFIGURE request");

  mb_wm_comp_mgr_clutter_fetch_texture (client);
}

static Bool
mb_wm_comp_mgr_clutter_handle_events_real (MBWMCompMgr * mgr, XEvent *ev)
{
  MBWMCompMgrClutterPrivate * priv = MB_WM_COMP_MGR_CLUTTER (mgr)->priv;
  MBWindowManager           * wm   = mgr->wm;

  if (ev->type == priv->damage_event + XDamageNotify)
    {
      XDamageNotifyEvent    * de = (XDamageNotifyEvent*) ev;
      MBWindowManagerClient * c;

      c = mb_wm_managed_client_from_frame (wm, de->drawable);

      if (c && c->cm_client)
	{
	  XserverRegion   parts;
	  int             i, r_count;
	  XRectangle    * r_damage;
	  XRectangle      r_bounds;

	  MBWMCompMgrClutterClient *cclient =
	    MB_WM_COMP_MGR_CLUTTER_CLIENT (c->cm_client);

	  if (!cclient->actor)
	    return False;

	  /* FIXME -- see if we can make some use of the 'more' parameter
	   * to compress the damage events
	   */
	  MBWM_NOTE (COMPOSITOR,
		     "Reparing window %x, geometry %d,%d;%dx%d; more %d\n",
		     de->drawable,
		     de->geometry.x,
		     de->geometry.y,
		     de->geometry.width,
		     de->geometry.height,
		     de->more);

	  /*
	   * Retrieve the damaged region and break it down into individual
	   * rectangles so we do not have to update the whole shebang.
	   */
	  parts = XFixesCreateRegion (wm->xdpy, 0, 0);
	  XDamageSubtract (wm->xdpy, de->damage, None, parts);

	  r_damage = XFixesFetchRegionAndBounds (wm->xdpy, parts,
						 &r_count,
						 &r_bounds);

	  if (r_damage)
	    {
	      for (i = 0; i < r_count; ++i)
		{
		  mb_wm_comp_mgr_clutter_update_texture (c->cm_client,
							 r_damage[i].x,
							 r_damage[i].y,
							 r_damage[i].width,
							 r_damage[i].height);
		}

	      XFree (r_damage);
	    }

	  XFixesDestroyRegion (wm->xdpy, parts);
	}
      else
	{
	  MBWM_NOTE (COMPOSITOR, "Failed to find client for window %x\n",
		     de->drawable);
	}
    }

  return False;
}

static void
mb_wm_comp_mgr_clutter_render_real (MBWMCompMgr *mgr)
{
  MBWMCompMgrClutterPrivate * priv = MB_WM_COMP_MGR_CLUTTER (mgr)->priv;
  MBWindowManagerClient     * wm_client;
  MBWindowManager           * wm = mgr->wm;

  MBWM_NOTE (COMPOSITOR, "Rendering");

  /*
   * We do not need to do anything, as rendering is done automatically for us
   * by clutter stage.
   */
}

static void
mb_wm_comp_mgr_clutter_map_notify_real (MBWMCompMgr *mgr,
					MBWindowManagerClient *c)
{
  MBWMCompMgrClutter        * cmgr    = MB_WM_COMP_MGR_CLUTTER (mgr);
  MBWMCompMgrClient         * client  = c->cm_client;
  MBWMCompMgrClutterClient  * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);
  ClutterActor *actor;
  MBGeometry                  geom;
  const MBWMList            * l;

  /*
   * We get called for windows as well as their children, so once we are
   * mapped do nothing.
   */
  if (cclient->mapped)
    return;

  cclient->mapped = True;
  actor = g_object_ref (clutter_x11_texture_pixmap_new ());
  cclient->actor = actor;

  l =
    mb_wm_theme_get_client_effects (c->wmref->theme, c);

  while (l)
    {
      MBWMThemeEffects * t_effects = l->data;
      MBWMList         * m_effects;

      m_effects = mb_wm_comp_mgr_client_get_effects (client,
						     t_effects->event,
						     t_effects->type,
						     t_effects->duration);

      mb_wm_comp_mgr_client_add_effects (client, t_effects->event, m_effects);
      l = l->next;
    }


  g_object_set_data (G_OBJECT (actor), "MBWMCompMgrClutterClient", cclient);

  mb_wm_comp_mgr_clutter_fetch_texture (MB_WM_COMP_MGR_CLIENT (cclient));

  mb_wm_client_get_coverage (c, &geom);
  clutter_actor_set_position (actor, geom.x, geom.y);

  mb_wm_comp_mgr_clutter_client_show_real (c->cm_client);

  mb_wm_comp_mgr_clutter_add_actor (cmgr, actor);
}

/*
 * Our windows which we need the WM to ingore are the overlay and the stage
 * window.
 */
static Bool
mb_wm_comp_mgr_is_my_window_real (MBWMCompMgr * mgr, Window xwin)
{
  MBWMCompMgrClutterPrivate * priv = MB_WM_COMP_MGR_CLUTTER (mgr)->priv;

  if (priv->overlay_window == xwin)
    return True;

  if (priv->stage &&
      (xwin == clutter_x11_get_stage_window (CLUTTER_STAGE (priv->stage))))
    return True;

  return False;
}

static void
mb_wm_comp_mgr_clutter_remove_actor (MBWMCompMgrClutter * cmgr,
				     ClutterActor * a)
{
  clutter_container_remove_actor (CLUTTER_CONTAINER (cmgr->priv->stage), a);
}

static void
mb_wm_comp_mgr_clutter_add_actor (MBWMCompMgrClutter * cmgr, ClutterActor * a)
{
  clutter_container_add_actor (CLUTTER_CONTAINER (cmgr->priv->stage), a);
}

MBWMCompMgr *
mb_wm_comp_mgr_clutter_new (MBWindowManager *wm)
{
  MBWMObject *mgr;

  mgr = mb_wm_object_new (MB_WM_TYPE_COMP_MGR_CLUTTER,
			  MBWMObjectPropWm, wm,
			  NULL);

  return MB_WM_COMP_MGR (mgr);
}

/*
 * MBWMCompMgrClutterEffect
 */
struct MBWMCompMgrClutterEffect
{
  MBWMCompMgrEffect       parent;
  ClutterTimeline        *timeline;
  ClutterBehaviour       *behaviour;
};

struct completed_cb_data
{
  MBWMCompMgrEffectCallback   completed_cb;
  void                      * cb_data;
  gulong                      my_id;
};


/*
 * Callback for ClutterTimeline::completed signal.
 *
 * One-off; get connected when the timeline is started, and disconnected
 * again when it finishes.
 */
static void
mb_wm_comp_mgr_clutter_effect_completed_cb (ClutterTimeline * t, void * data)
{
  struct completed_cb_data * d = data;

  if (d->completed_cb)
    d->completed_cb (d->cb_data);

  g_signal_handler_disconnect (t, d->my_id);

  free (d);
}

static void
mb_wm_comp_mgr_clutter_effect_run_real (MBWMList                * effects,
					MBWMCompMgrEffectEvent    event,
					MBWMCompMgrEffectCallback completed_cb,
					void                    * data)
{
  /*
   * Since the entire effect group for a single event type shares
   * a timeline, we just need to start it.
   *
   * TODO -- there is no need for the effect objects to carry the timeline
   * point, so remove it; will need to change this API to take
   * MBWMCompMgrClient pointer.
   */
  if (effects && effects->data)
    {
      MBWMCompMgrClutterEffect * eff = effects->data;
      printf ("^^^^ running effect %p\n", eff);

      if (eff->timeline)
	{
	  GSList * actors;
	  ClutterActor *a;

	  if (completed_cb)
	    {
	      struct completed_cb_data * d =
		mb_wm_util_malloc0 (sizeof (struct completed_cb_data));

	      d->completed_cb = completed_cb;
	      d->cb_data = data;

	      d->my_id = g_signal_connect (eff->timeline, "completed",
		      G_CALLBACK (mb_wm_comp_mgr_clutter_effect_completed_cb),
		      d);
	    }

	  actors = clutter_behaviour_get_actors (eff->behaviour);
	  a = actors->data;

	  clutter_actor_move_anchor_point_from_gravity (a,
						CLUTTER_GRAVITY_NORTH_EAST);

	  clutter_timeline_start (eff->timeline);
	}
    }
}

static void
mb_wm_comp_mgr_clutter_effect_class_init (MBWMObjectClass *klass)
{
  MBWMCompMgrEffectClass *c_klass = MB_WM_COMP_MGR_EFFECT_CLASS (klass);

  c_klass->run = mb_wm_comp_mgr_clutter_effect_run_real;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMCompMgrClutterEffect";
#endif
}

static int
mb_wm_comp_mgr_clutter_effect_init (MBWMObject *obj, va_list vap)
{
  MBWMObjectProp         prop;
  ClutterTimeline       *timeline;
  ClutterBehaviour      *behaviour;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropCompMgrClutterEffectTimeline:
	  timeline = va_arg(vap, ClutterTimeline *);
	  break;
	case MBWMObjectPropCompMgrClutterEffectBehaviour:
	  behaviour = va_arg(vap, ClutterBehaviour *);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  if (!timeline || !behaviour)
    return 0;

  MB_WM_COMP_MGR_CLUTTER_EFFECT (obj)->timeline  = timeline;
  MB_WM_COMP_MGR_CLUTTER_EFFECT (obj)->behaviour = behaviour;

  return 1;
}

static void
mb_wm_comp_mgr_clutter_effect_destroy (MBWMObject* obj)
{
  MBWMCompMgrClutterEffect * e = MB_WM_COMP_MGR_CLUTTER_EFFECT (obj);

  if (!e || !e->behaviour)
    return;

  g_object_unref (e->behaviour);
  g_object_unref (e->timeline);
}

int
mb_wm_comp_mgr_clutter_effect_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMCompMgrClutterEffectClass),
	sizeof (MBWMCompMgrClutterEffect),
	mb_wm_comp_mgr_clutter_effect_init,
	mb_wm_comp_mgr_clutter_effect_destroy,
	mb_wm_comp_mgr_clutter_effect_class_init
      };

      type =
	mb_wm_object_register_class (&info, MB_WM_TYPE_COMP_MGR_EFFECT, 0);
    }

  return type;
}

/*
 * This is private method for use by the manager, hence static.
 */
static MBWMCompMgrEffect *
mb_wm_comp_mgr_clutter_effect_new (MBWMCompMgrEffectType    type,
				   unsigned long            duration,
				   ClutterTimeline        * timeline,
				   ClutterBehaviour       * behaviour)
{
  MBWMObject *e;

  e =
    mb_wm_object_new (MB_WM_TYPE_COMP_MGR_CLUTTER_EFFECT,
		      MBWMObjectPropCompMgrEffectType,             type,
		      MBWMObjectPropCompMgrEffectDuration,         duration,
		      MBWMObjectPropCompMgrClutterEffectTimeline,  timeline,
		      MBWMObjectPropCompMgrClutterEffectBehaviour, behaviour,
		      NULL);

  return MB_WM_COMP_MGR_EFFECT (e);
}
