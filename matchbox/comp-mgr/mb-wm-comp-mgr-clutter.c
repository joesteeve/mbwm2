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
mb_wm_comp_mgr_clutter_add_actor (MBWMCompMgrClutter *, ClutterActor *);

static void
mb_wm_comp_mgr_clutter_remove_actor (MBWMCompMgrClutter *, ClutterActor *);

enum
{
  MBWMCompMgrClutterClientMapped        = (1<<0),
  MBWMCompMgrClutterClientDontUpdate    = (1<<1),
  MBWMCompMgrClutterClientDone          = (1<<2),
  MBWMCompMgrClutterClientEffectRunning = (1<<3),
};

/*
 * A helper object to store manager's per-client data
 */
struct MBWMCompMgrClutterClient
{
  MBWMCompMgrClient       parent;
  ClutterActor          * actor;  /* Overall actor */
  ClutterActor          * texture; /* The texture part of our actor */
  Pixmap                  pixmap;
  int                     pxm_width;
  int                     pxm_height;
  int                     pxm_depth;
  unsigned int            flags;
  Damage                  damage;

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
					       unsigned long duration,
					       MBWMGravity gravity);

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
  MBWMCompMgrClutterClient  *cclient  = MB_WM_COMP_MGR_CLUTTER_CLIENT(client);
  MBWindowManagerClient     *wm_client = client->wm_client;
  MBWindowManager           *wm        = wm_client->wmref;
  MBGeometry                 geom;
  Window                     xwin;
  Window root;
  int                        x, y, w, h, bw, depth;
#ifdef HAVE_XEXT
  /* Stuff we need for shaped windows */
  XRectangle                *shp_rect;
  int                        shp_order;
  int                        shp_count;
  int                        i;

  /*
   * This is square of 32-bit comletely transparent values we use to
   * clear bits of the texture from shaped windows; the size is a compromise
   * between how much memory we want to allocate and how much tiling we are
   * happy with.
   *
   * Make this power of 2 for efficient operation
   */
#define SHP_CLEAR_SIZE 4
  static int clear_init = 0;
  static guint32 clear_data[SHP_CLEAR_SIZE * SHP_CLEAR_SIZE];

  if  (!clear_init)
    {
      memset (&clear_data, 0, sizeof (clear_data));
      clear_init = 1;
    }
#endif

  if (!(cclient->flags & MBWMCompMgrClutterClientMapped))
    return;

  xwin =
    wm_client->xwin_frame ? wm_client->xwin_frame : wm_client->window->xwindow;

  if (cclient->pixmap)
    XFreePixmap (wm->xdpy, cclient->pixmap);

  cclient->pixmap = XCompositeNameWindowPixmap (wm->xdpy, xwin);

  if (!cclient->pixmap)
    return;

  XGetGeometry (wm->xdpy, cclient->pixmap, &root,
		&x, &y, &w, &h, &bw, &depth);

  mb_wm_client_get_coverage (wm_client, &geom);

  cclient->pxm_width  = w;
  cclient->pxm_height = h;
  cclient->pxm_depth  = depth;

  clutter_actor_set_position (cclient->actor, geom.x, geom.y);
  clutter_actor_set_size (cclient->texture, geom.width, geom.height);

  clutter_x11_texture_pixmap_set_pixmap (
				CLUTTER_X11_TEXTURE_PIXMAP (cclient->texture),
				cclient->pixmap,
				w, h, depth);

#ifdef HAVE_XEXT
  /*
   * If the client is shaped, we have to manually clear any pixels in our
   * texture in the non-visible areas.
   */
  if (mb_wm_theme_is_client_shaped (wm->theme, wm_client))
    {
      shp_rect = XShapeGetRectangles (wm->xdpy, xwin,
				     ShapeBounding, &shp_count, &shp_order);

      if (shp_rect && shp_count)
	{
	  XserverRegion clear_rgn;
	  XRectangle rect;
	  XRectangle * clear_rect;
	  int clear_count;

	  XRectangle *r0, r1;
	  int c;

	  rect.x = 0;
	  rect.y = 0;
	  rect.width = geom.width;
	  rect.height = geom.height;

	  clear_rgn   = XFixesCreateRegion (wm->xdpy, shp_rect, shp_count);

	  XFixesInvertRegion (wm->xdpy, clear_rgn, &rect, clear_rgn);

	  clear_rect = XFixesFetchRegion (wm->xdpy, clear_rgn, &clear_count);

	  for (i = 0; i < clear_count; ++i)
	    {
	      int k, l;

	      for (k = 0; k < clear_rect[i].width; k += SHP_CLEAR_SIZE)
		for (l = 0; l < clear_rect[i].height; l += SHP_CLEAR_SIZE)
		  {
		    int w1 = clear_rect[i].width - k;
		    int h1 = clear_rect[i].height - l;

		    if (w1 > SHP_CLEAR_SIZE)
		      w1 = SHP_CLEAR_SIZE;

		    if (h1 > SHP_CLEAR_SIZE)
		      h1 = SHP_CLEAR_SIZE;

		    clutter_texture_set_area_from_rgb_data (
					  CLUTTER_TEXTURE (cclient->texture),
					  (const guchar *)&clear_data,
					  TRUE,
					  clear_rect[i].x + k,
					  clear_rect[i].y + l,
					  w1, h1,
					  SHP_CLEAR_SIZE * 4,
					  4,
					  CLUTTER_TEXTURE_RGB_FLAG_BGR,
                                          NULL);
		  }
	    }

	  XFixesDestroyRegion (wm->xdpy, clear_rgn);

	  XFree (shp_rect);

	  if (clear_rect)
	    XFree (clear_rect);
	}
    }

#endif
}

static int
mb_wm_comp_mgr_clutter_client_init (MBWMObject *obj, va_list vap)
{
  return 1;
}

static void
mb_wm_comp_mgr_clutter_client_destroy (MBWMObject* obj)
{
  MBWMCompMgrClient        * c   = MB_WM_COMP_MGR_CLIENT (obj);
  MBWMCompMgrClutterClient * cc  = MB_WM_COMP_MGR_CLUTTER_CLIENT (obj);
  MBWindowManager          * wm  = c->wm_client->wmref;
  MBWMCompMgrClutter       * mgr = MB_WM_COMP_MGR_CLUTTER (wm->comp_mgr);

  mb_wm_comp_mgr_clutter_remove_actor (mgr, cc->actor);

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

  /*
   * Do not hide the actor if effect is in progress
   */
  if (cclient->flags & MBWMCompMgrClutterClientEffectRunning)
    return;

  clutter_actor_hide (cclient->actor);
}

static void
mb_wm_comp_mgr_clutter_client_show_real (MBWMCompMgrClient * client)
{
  MBWMCompMgrClutterClient * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);

  if (!cclient->actor)
    {
      /*
       * This can happen if show() is called on our client before it is
       * actually mapped (we only alocate the actor in response to map
       * notification.
       */
      return;
    }

  /*
   * Clear the don't update flag, if set
   */
  cclient->flags &= ~MBWMCompMgrClutterClientDontUpdate;
  clutter_actor_show_all (cclient->actor);
}

static MBWMCompMgrEffect *
mb_wm_comp_mgr_clutter_effect_new (MBWMCompMgrEffectType    type,
				   unsigned long            duration,
				   MBWMGravity              gravity,
				   ClutterTimeline        * timeline,
				   ClutterBehaviour       * behaviour);

/*
 * Helper method to get a timeline for given event (all effects associated
 * with the same event share a single timeline.
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
					       unsigned long duration,
					       MBWMGravity   gravity)
{
  MBWMCompMgrEffect        * eff;
  ClutterTimeline          * timeline;
  ClutterBehaviour         * behaviour;
  ClutterAlpha             * alpha;
  MBWMCompMgrClutterClient * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);
  MBWindowManagerClient    * wm_client = client->wm_client;
  MBWindowManager          * wm = wm_client->wmref;
  ClutterKnot                knots[2];
  MBGeometry                 geom;

  timeline =
    mb_wm_comp_mgr_clutter_client_get_timeline (client, event, duration);

  if (!timeline)
    return NULL;

  alpha = clutter_alpha_new_full (timeline,
				  CLUTTER_ALPHA_RAMP_INC, NULL, NULL);

  mb_wm_client_get_coverage (wm_client, &geom);

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

      /*
       * Currently ClutterBehaviourPath does not handle negative coords,
       * so anything here that needs them is broken.
       */
    case MBWMCompMgrEffectSlideIn:
      switch (gravity)
	{
	case MBWMGravityNorth:
	  knots[0].x = geom.x;
	  knots[0].y = wm->xdpy_height;
	  knots[1].x = geom.x;
	  knots[1].y = geom.y;
	  break;
	case MBWMGravitySouth:
	  knots[0].x = geom.x;
	  knots[0].y = -geom.height;
	  knots[1].x = geom.x;
	  knots[1].y = geom.y;
	  break;
	case MBWMGravityWest:
	  knots[0].x = wm->xdpy_width;
	  knots[0].y = geom.y;
	  knots[1].x = geom.x;
	  knots[1].y = geom.y;
	  break;
	case MBWMGravityEast:
	  knots[0].x = -geom.width;
	  knots[0].y = geom.y;
	  knots[1].x = geom.x;
	  knots[1].y = geom.y;
	  break;
	case MBWMGravityNorthWest:
	case MBWMGravityNone:
	default:
	  knots[0].x = wm->xdpy_width;
	  knots[0].y = wm->xdpy_height;
	  knots[1].x = geom.x;
	  knots[1].y = geom.y;
	  break;
	case MBWMGravityNorthEast:
	  knots[0].x = -geom.width;
	  knots[0].y = wm->xdpy_height;
	  knots[1].x = geom.x;
	  knots[1].y = geom.y;
	  break;
	case MBWMGravitySouthWest:
	  knots[0].x = wm->xdpy_width;
	  knots[0].y = -geom.height;
	  knots[1].x = geom.x;
	  knots[1].y = geom.y;
	  break;
	case MBWMGravitySouthEast:
	  knots[0].x = -geom.width;
	  knots[0].y = -geom.height;
	  knots[1].x = geom.x;
	  knots[1].y = geom.y;
	  break;
	}

      behaviour = clutter_behaviour_path_new (alpha, &knots[0], 2);
      break;
    case MBWMCompMgrEffectSlideOut:
      switch (gravity)
	{
	case MBWMGravityNorth:
	  knots[0].x = geom.x;
	  knots[0].y = geom.y;
	  knots[1].x = geom.x;
	  knots[1].y = -(geom.y + geom.height);
	  break;
	case MBWMGravitySouth:
	  knots[0].x = geom.x;
	  knots[0].y = geom.y;
	  knots[1].x = geom.x;
	  knots[1].y = wm->xdpy_height;
	  break;
	case MBWMGravityEast:
	  knots[0].x = geom.x;
	  knots[0].y = geom.y;
	  knots[1].x = wm->xdpy_width;
	  knots[1].y = geom.y;
	  break;
	case MBWMGravityWest:
	  knots[0].x = geom.x;
	  knots[0].y = geom.y;
	  knots[1].x = -(geom.x + geom.width);
	  knots[1].y = geom.y;
	  break;
	case MBWMGravityNorthWest:
	case MBWMGravityNone:
	default:
	  knots[0].x = geom.x;
	  knots[0].y = geom.y;
	  knots[1].x = -(geom.x + geom.width);
	  knots[1].y = -(geom.y + geom.height);
	  break;
	case MBWMGravityNorthEast:
	  knots[0].x = geom.x;
	  knots[0].y = geom.y;
	  knots[1].x = wm->xdpy_width;
	  knots[1].y = -(geom.y + geom.height);
	  break;
	case MBWMGravitySouthWest:
	  knots[0].x = geom.x;
	  knots[0].y = geom.y;
	  knots[1].x = -(geom.x + geom.width);
	  knots[1].y = wm->xdpy_height;
	  break;
	case MBWMGravitySouthEast:
	  knots[0].x = geom.x;
	  knots[0].y = geom.y;
	  knots[1].x = wm->xdpy_width;
	  knots[1].y = wm->xdpy_height;
	  break;
	}

      behaviour = clutter_behaviour_path_new (alpha, &knots[0], 2);
    }

  eff =
    mb_wm_comp_mgr_clutter_effect_new (type, duration, gravity,
				       timeline, behaviour);

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
  ClutterActor * desktop;
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
mb_wm_comp_mgr_clutter_turn_on_real (MBWMCompMgr *mgr);

static void
mb_wm_comp_mgr_clutter_turn_off_real (MBWMCompMgr *mgr);

static void
mb_wm_comp_mgr_clutter_render_real (MBWMCompMgr *mgr);

static void
mb_wm_comp_mgr_clutter_map_notify_real (MBWMCompMgr *mgr,
					MBWindowManagerClient *c);

static void
mb_wm_comp_mgr_clutter_transition_real (MBWMCompMgr * mgr,
					MBWindowManagerClient *c1,
					MBWindowManagerClient *c2,
					Bool reverse);

static void
mb_wm_comp_mgr_clutter_restack_real (MBWMCompMgr *mgr);

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
  cm_klass->turn_on           = mb_wm_comp_mgr_clutter_turn_on_real;
  cm_klass->turn_off          = mb_wm_comp_mgr_clutter_turn_off_real;
  cm_klass->render            = mb_wm_comp_mgr_clutter_render_real;
  cm_klass->map_notify        = mb_wm_comp_mgr_clutter_map_notify_real;
  cm_klass->handle_events     = mb_wm_comp_mgr_clutter_handle_events_real;
  cm_klass->my_window         = mb_wm_comp_mgr_is_my_window_real;
  cm_klass->transition        = mb_wm_comp_mgr_clutter_transition_real;
  cm_klass->restack           = mb_wm_comp_mgr_clutter_restack_real;
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
  priv->desktop = clutter_group_new ();

  clutter_actor_show (priv->desktop);
  clutter_container_add_actor (CLUTTER_CONTAINER (priv->stage), priv->desktop);

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
	  mb_wm_comp_mgr_unregister_client (mgr, c);
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
      ClutterColor      clr = {0, 0, 0, 0xff };
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
}

static void
mb_wm_comp_mgr_clutter_client_repair_real (MBWMCompMgrClient * client)
{
  MBWindowManagerClient    * wm_client = client->wm_client;
  MBWMCompMgrClutterClient * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);
  MBWindowManager          * wm   = wm_client->wmref;
  XserverRegion              parts;
  int                        i, r_count;
  XRectangle               * r_damage;
  XRectangle                 r_bounds;

  MBWM_NOTE (COMPOSITOR, "REPAIRING %x", wm_client->window->xwindow);

  if (!cclient->actor)
    return;

  if (!cclient->pixmap)
    {
      /*
       * First time we have been called since creation/configure,
       * fetch the whole texture.
       */
      MBWM_NOTE (DAMAGE, "Full screen repair.");
      XDamageSubtract (wm->xdpy, cclient->damage, None, None);
      mb_wm_comp_mgr_clutter_fetch_texture (client);
      return;
    }

  /*
   * Retrieve the damaged region and break it down into individual
   * rectangles so we do not have to update the whole shebang.
   */
  parts = XFixesCreateRegion (wm->xdpy, 0, 0);
  XDamageSubtract (wm->xdpy, cclient->damage, None, parts);

  r_damage = XFixesFetchRegionAndBounds (wm->xdpy, parts,
					 &r_count,
					 &r_bounds);

  if (r_damage)
    {
      for (i = 0; i < r_count; ++i)
	{
	  MBWM_NOTE (DAMAGE, "Repairing %d,%d;%dx%d",
		     r_damage[i].x,
		     r_damage[i].y,
		     r_damage[i].width,
		     r_damage[i].height);

	  clutter_x11_texture_pixmap_update_area (
				CLUTTER_X11_TEXTURE_PIXMAP (cclient->texture),
				r_damage[i].x,
				r_damage[i].y,
				r_damage[i].width,
				r_damage[i].height);
	}

      XFree (r_damage);
    }

  XFixesDestroyRegion (wm->xdpy, parts);
}

static void
mb_wm_comp_mgr_clutter_client_configure_real (MBWMCompMgrClient * client)
{
  MBWindowManagerClient    * wm_client = client->wm_client;
  MBWMCompMgrClutterClient * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT (client);

  MBWM_NOTE (COMPOSITOR, "CONFIGURE request");

  /*
   * Release the backing pixmap; we will recreate it next time we get damage
   * notification for this window.
   */
  if (cclient->pixmap)
    {
      XFreePixmap (wm_client->wmref->xdpy, cclient->pixmap);
      cclient->pixmap = None;
    }
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
	  MBWMCompMgrClutterClient *cclient =
	    MB_WM_COMP_MGR_CLUTTER_CLIENT (c->cm_client);

	  if (!cclient->actor ||
	      (cclient->flags & MBWMCompMgrClutterClientDontUpdate))
	    return False;

	  MBWM_NOTE (COMPOSITOR,
		     "Reparing window %x, geometry %d,%d;%dx%d; more %d\n",
		     de->drawable,
		     de->geometry.x,
		     de->geometry.y,
		     de->geometry.width,
		     de->geometry.height,
		     de->more);

	  mb_wm_comp_mgr_clutter_client_repair_real (c->cm_client);
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
mb_wm_comp_mgr_clutter_restack_real (MBWMCompMgr *mgr)
{
  MBWindowManager *wm = mgr->wm;

  if (!mb_wm_stack_empty (wm))
    {
      MBWindowManagerClient * c;
      ClutterActor * prev = NULL;

      mb_wm_stack_enumerate (wm, c)
	{
	  MBWMCompMgrClutterClient * cc =
	    MB_WM_COMP_MGR_CLUTTER_CLIENT (c->cm_client);

	  ClutterActor * a = cc->actor;

	  if (!a)
	    continue;

	  clutter_actor_raise (a, prev);
	  prev = a;
	}
    }

}

static void
mb_wm_comp_mgr_clutter_render_real (MBWMCompMgr *mgr)
{
  MBWMCompMgrClutterPrivate * priv = MB_WM_COMP_MGR_CLUTTER (mgr)->priv;
  MBWindowManagerClient     * wm_client;
  MBWindowManager           * wm = mgr->wm;

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
  MBWMCompMgrClutterClient  * cclient = MB_WM_COMP_MGR_CLUTTER_CLIENT(client);
  MBWindowManager           * wm      = c->wmref;
  ClutterActor              * actor;
  ClutterActor              * texture;
  ClutterActor              * rect;
  MBGeometry                  geom;
  const MBWMList            * l;
  unsigned int                shadow_clr[4];
  ClutterColor                shadow_cclr;

  /*
   * We get called for windows as well as their children, so once we are
   * mapped do nothing.
   */
  if (cclient->flags & MBWMCompMgrClutterClientMapped)
    return;

  cclient->flags |= MBWMCompMgrClutterClientMapped;

  cclient->damage = XDamageCreate (wm->xdpy,
				   c->xwin_frame ?
				   c->xwin_frame :
				   c->window->xwindow,
				   XDamageReportNonEmpty);

  mb_wm_client_get_coverage (c, &geom);

  actor = g_object_ref (clutter_group_new ());
  texture = clutter_x11_texture_pixmap_new ();
  clutter_actor_show (texture);

  mb_wm_theme_get_shadow_color (wm->theme,
				&shadow_clr[0],
				&shadow_clr[1],
				&shadow_clr[2],
				&shadow_clr[3]);

  shadow_cclr.red   = 0xff * shadow_clr[0] / 0xffff;
  shadow_cclr.green = 0xff * shadow_clr[1] / 0xffff;
  shadow_cclr.blue  = 0xff * shadow_clr[2] / 0xffff;
  shadow_cclr.alpha = 0xff * shadow_clr[3] / 0xffff;

  rect = clutter_rectangle_new_with_color (&shadow_cclr);
  clutter_actor_set_position (rect, 4, 4);
  clutter_actor_set_size (rect, geom.width, geom.height);

  clutter_actor_show (rect);

  clutter_container_add (CLUTTER_CONTAINER (actor), rect, texture, NULL);

  cclient->actor = actor;
  cclient->texture = texture;

  l =
    mb_wm_theme_get_client_effects (wm->theme, c);

  while (l)
    {
      MBWMThemeEffects * t_effects = l->data;
      MBWMList         * m_effects;

      m_effects = mb_wm_comp_mgr_client_get_effects (client,
						     t_effects->event,
						     t_effects->type,
						     t_effects->duration,
						     t_effects->gravity);

      mb_wm_comp_mgr_client_add_effects (client, t_effects->event, m_effects);
      l = l->next;
    }


  g_object_set_data (G_OBJECT (actor), "MBWMCompMgrClutterClient", cclient);

  clutter_actor_set_position (actor, geom.x, geom.y);

  mb_wm_comp_mgr_clutter_add_actor (cmgr, actor);
}

struct _fade_cb_data
{
  MBWMCompMgrClutterClient *c1;
  MBWMCompMgrClutterClient *c2;
  ClutterTimeline  * timeline;
  ClutterBehaviour * beh;
};

static void
mb_wm_comp_mgr_clutter_transtion_fade_cb (ClutterTimeline * t, void * data)
{
  struct _fade_cb_data * d  = data;
  ClutterActor   * a1 = d->c1->actor;
  ClutterActor   * a2 = d->c2->actor;

  clutter_actor_set_opacity (a1, 0xff);

  d->c1->flags &= ~MBWMCompMgrClutterClientEffectRunning;
  d->c2->flags &= ~MBWMCompMgrClutterClientEffectRunning;

  g_object_unref (d->timeline);
  g_object_unref (d->beh);
}

static void
_fade_apply_behaviour_to_client (MBWindowManagerClient * wc,
				 ClutterBehaviour      * b)
{
  MBWMList * l;
  ClutterActor * a = MB_WM_COMP_MGR_CLUTTER_CLIENT (wc->cm_client)->actor;

  clutter_actor_set_opacity (a, 0);
  clutter_behaviour_apply (b, a);

  l = mb_wm_client_get_transients (wc);
  while (l)
    {
      MBWindowManagerClient * c = l->data;

      _fade_apply_behaviour_to_client (c, b);
      l = l->next;
    }
}

static void
mb_wm_comp_mgr_clutter_transition_fade (MBWMCompMgrClutterClient *c1,
					MBWMCompMgrClutterClient *c2,
					unsigned long duration)
{
  ClutterTimeline             * timeline;
  ClutterAlpha                * alpha;
  static struct _fade_cb_data   cb_data;
  ClutterBehaviour            * b;

  /*
   * Fade is simple -- we only need to animate the second actor and its
   * children, as the stacking order automatically takes care of the
   * actor appearing to fade out from the first one
   */
  timeline = clutter_timeline_new_for_duration (duration);

  alpha = clutter_alpha_new_full (timeline,
				  CLUTTER_ALPHA_RAMP_DEC, NULL, NULL);

  b = clutter_behaviour_opacity_new (alpha, 0xff, 0);

  cb_data.c1 = c1;
  cb_data.c2 = c2;
  cb_data.timeline = timeline;
  cb_data.beh = b;

  _fade_apply_behaviour_to_client (MB_WM_COMP_MGR_CLIENT (c2)->wm_client, b);

  /*
   * Must restore the opacity on the 'from' actor
   */
  g_signal_connect (timeline, "completed",
		    G_CALLBACK (mb_wm_comp_mgr_clutter_transtion_fade_cb),
		    &cb_data);

  c1->flags |= MBWMCompMgrClutterClientEffectRunning;
  c2->flags |= MBWMCompMgrClutterClientEffectRunning;

  clutter_timeline_start (timeline);
}

static void
mb_wm_comp_mgr_clutter_transition_real (MBWMCompMgr * mgr,
					MBWindowManagerClient *c1,
					MBWindowManagerClient *c2,
					Bool reverse)
{
  MBWMCompMgrClutterClient * cc1 =
    MB_WM_COMP_MGR_CLUTTER_CLIENT (c1->cm_client);
  MBWMCompMgrClutterClient * cc2 =
    MB_WM_COMP_MGR_CLUTTER_CLIENT (c2->cm_client);

  MBWMThemeTransition * trs =
    mb_wm_theme_get_client_transition (c1->wmref->theme, c1);

  if (!trs)
    return;

  switch (trs->type)
    {
    case MBWMCompMgrTransitionFade:
      mb_wm_comp_mgr_clutter_transition_fade (cc1,
					      cc2,
					      trs->duration);
      break;
    default:
      MBWM_DBG ("Unimplemented transition type %d", trs->type);
    }
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
  clutter_container_remove_actor (CLUTTER_CONTAINER (cmgr->priv->desktop), a);
}

static void
mb_wm_comp_mgr_clutter_add_actor (MBWMCompMgrClutter * cmgr, ClutterActor * a)
{
  clutter_container_add_actor (CLUTTER_CONTAINER (cmgr->priv->desktop), a);
}

ClutterActor *
mb_wm_comp_mgr_clutter_get_desktop (MBWMCompMgrClutter * cmgr)
{
  return cmgr->priv->desktop;
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
  ClutterBehaviour       *behaviour; /* can be either behaviour or effect */
};

struct completed_cb_data
{
  gulong                      my_id;
  MBWMCompMgrClutterClient  * cclient;
  MBWMCompMgrEffectEvent      event;
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

  d->cclient->flags &= ~MBWMCompMgrClutterClientEffectRunning;

  g_signal_handler_disconnect (t, d->my_id);

  switch (d->event)
    {
    case MBWMCompMgrEffectEventUnmap:
    case MBWMCompMgrEffectEventMinimize:
      clutter_actor_hide (d->cclient->actor);
      break;

    default:
      break;
    }

  /*
   * Release the extra reference on the CM client that was added for the sake
   * of the effect
   */
  mb_wm_object_unref (MB_WM_OBJECT (d->cclient));

  free (d);
}

static void
mb_wm_comp_mgr_clutter_effect_run_real (MBWMList                * effects,
					MBWMCompMgrClient       * cm_client,
					MBWMCompMgrEffectEvent    event)
{
  MBWMCompMgrClutterClient *cclient =
    MB_WM_COMP_MGR_CLUTTER_CLIENT (cm_client);

  /*
   * Since the entire effect group for a single event type shares
   * a timeline, we just need to start it for one of the behaviours.
   *
   * TODO -- there is no need for the effect objects to carry the timeline
   * pointer, so remove it.
   */
  if (effects && effects->data)
    {
      MBWMCompMgrEffect        * eff = effects->data;
      MBWMCompMgrClutterEffect * ceff = MB_WM_COMP_MGR_CLUTTER_EFFECT (eff);

      /*
       * Don't attempt to start the timeline if it is already playing
       */
      if (ceff->timeline &&
	  !clutter_timeline_is_playing (ceff->timeline))
	{
	  GSList * actors;
	  ClutterActor *a;
	  Bool dont_run = False;

	  struct completed_cb_data * d;

	  d = mb_wm_util_malloc0 (sizeof (struct completed_cb_data));

	  d->cclient = cclient;
	  d->event   = event;

	  d->my_id = g_signal_connect (ceff->timeline, "completed",
		      G_CALLBACK (mb_wm_comp_mgr_clutter_effect_completed_cb),
		      d);

	  /*
	   * This is bit of a pain; we know that our actor is attached to
	   * a single actor, but the current API only provides us with a copy
	   * of the actor list; ideally, we would like to be able to access
	   * the first actor directly.
	   */
	  actors = clutter_behaviour_get_actors (ceff->behaviour);
	  a = actors->data;

	  /*
	   * Deal with gravity, but not for path behaviours (there the
	   * gravity translates into the path itself, and is already
	   * set up).
	   */
	  if (eff->gravity != CLUTTER_GRAVITY_NONE &&
	      !CLUTTER_IS_BEHAVIOUR_PATH (ceff->behaviour))
	    {
	      clutter_actor_move_anchor_point_from_gravity (a, eff->gravity);
	    }
	  else if (CLUTTER_IS_BEHAVIOUR_PATH (ceff->behaviour) &&
		   !CLUTTER_ACTOR_IS_VISIBLE (a))
	    {
	      /*
	       * At this stage, if the actor is not yet visible, move it to
	       * the starting point of the path (this is mostly because of
	       * 'map' effects,  where the clutter_actor_show () is delayed
	       * until this point, so that the actor can be positioned in the
	       * correct location without visible artefacts).
	       *
	       * FIXME -- this is very clumsy; we need clutter API to query
	       * the first knot of the path to avoid messing about with copies
	       * of the list.
	       */

	      GSList * knots =
		clutter_behaviour_path_get_knots (
				  CLUTTER_BEHAVIOUR_PATH (ceff->behaviour));

	      if (knots)
		{
		  ClutterKnot * k = knots->data;

		  clutter_actor_set_position (a, k->x, k->y);

		  g_slist_free (knots);
		}
	    }

	  if (event == MBWMCompMgrEffectEventUnmap)
	    {
	      cclient->flags |= MBWMCompMgrClutterClientDontUpdate;

	      if (cclient->flags & MBWMCompMgrClutterClientDone)
		dont_run = True;
	      else
		cclient->flags |= MBWMCompMgrClutterClientDone;
	    }

	  /*
	   * Make sure the actor is showing (for example with 'map' effects,
	   * the show() is delayed until the effect had chance to
	   * set up the actor postion).
	   */
	  if (!dont_run)
	    {
	      cclient->flags |= MBWMCompMgrClutterClientEffectRunning;
	      clutter_actor_show (a);
	      clutter_timeline_start (ceff->timeline);
	    }

	  g_slist_free (actors);

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
				   MBWMGravity              gravity,
				   ClutterTimeline        * timeline,
				   ClutterBehaviour       * behaviour)
{
  MBWMObject *e;

  e =
    mb_wm_object_new (MB_WM_TYPE_COMP_MGR_CLUTTER_EFFECT,
		      MBWMObjectPropCompMgrEffectType,             type,
		      MBWMObjectPropCompMgrEffectDuration,         duration,
		      MBWMObjectPropCompMgrEffectGravity,          gravity,
		      MBWMObjectPropCompMgrClutterEffectTimeline,  timeline,
		      MBWMObjectPropCompMgrClutterEffectBehaviour, behaviour,
		      NULL);

  return MB_WM_COMP_MGR_EFFECT (e);
}
