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

#include <math.h>

#define SHADOW_RADIUS 6
#define SHADOW_OPACITY	0.9
#define SHADOW_OFFSET_X	(-SHADOW_RADIUS)
#define SHADOW_OFFSET_Y	(-SHADOW_RADIUS)

#define MAX_TILE_SZ 128 	/* make sure size/2 < MAX_TILE_SZ */
#define WIDTH  (3*MAX_TILE_SZ)
#define HEIGHT (3*MAX_TILE_SZ)

static unsigned char *
mb_wm_comp_mgr_clutter_shadow_gaussian_make_tile ();

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

static void
mb_wm_comp_mgr_clutter_client_class_init (MBWMObjectClass *klass)
{
  MBWMCompMgrClientClass *c_klass = MB_WM_COMP_MGR_CLIENT_CLASS (klass);

  c_klass->show       = mb_wm_comp_mgr_clutter_client_show_real;
  c_klass->hide       = mb_wm_comp_mgr_clutter_client_hide_real;
  c_klass->repair     = mb_wm_comp_mgr_clutter_client_repair_real;
  c_klass->configure  = mb_wm_comp_mgr_clutter_client_configure_real;

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

/*
 * MBWMCompMgrClutterEffect
 */
typedef struct MBWMCompMgrClutterEffect
{
  ClutterTimeline        *timeline;
  ClutterBehaviour       *behaviour; /* can be either behaviour or effect */
} MBWMCompMgrClutterEffect;

struct completed_cb_data
{
  gulong                      my_id;
  MBWMCompMgrClutterClient  * cclient;
  MBWMCompMgrEffectEvent      event;
};

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

static MBWMCompMgrClutterEffect *
mb_wm_comp_mgr_clutter_effect_new (MBWMCompMgrClient      * client,
				   MBWMCompMgrEffectEvent   event,
				   unsigned long            duration)
{
  MBWMCompMgrClutterEffect * eff;
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

  switch (event)
    {
    case MBWMCompMgrEffectEventMinimize:
      behaviour =
	clutter_behaviour_scale_newx (alpha, CFX_ONE, CFX_ONE, 0, 0);
      break;
    case MBWMCompMgrEffectEventUnmap:
      behaviour = clutter_behaviour_opacity_new (alpha, 0xff, 0);
      break;
    case MBWMCompMgrEffectEventMap:
      knots[0].x = -wm->xdpy_width;
      knots[0].y = geom.y;
      knots[1].x = geom.x;
      knots[1].y = geom.y;
      behaviour = clutter_behaviour_path_new (alpha, &knots[0], 2);
      break;
    }

  eff = mb_wm_util_malloc0 (sizeof (MBWMCompMgrClutterEffect));
  eff->timeline = timeline;
  eff->behaviour = behaviour;

  clutter_behaviour_apply (behaviour, cclient->actor);

  return eff;
}

/*
 * Implementation of MBWMCompMgrClutter
 */
struct MBWMCompMgrClutterPrivate
{
  ClutterActor * stage;
  ClutterActor * desktop;
  ClutterActor * shadow;

  Window         overlay_window;
  int            damage_event;
};

static void
mb_wm_comp_mgr_clutter_private_free (MBWMCompMgrClutter *mgr)
{
  MBWMCompMgrClutterPrivate * priv = mgr->priv;

  if (priv->shadow)
    g_object_unref (priv->shadow);

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
mb_wm_comp_mgr_clutter_effect_real (MBWMCompMgr            * mgr,
				    MBWindowManagerClient  * client,
				    MBWMCompMgrEffectEvent   event);

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
  cm_klass->effect            = mb_wm_comp_mgr_clutter_effect_real;
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
  MBWMCompMgrShadowType       shadow_type;
  MBWMClientType              ctype = MB_WM_CLIENT_CLIENT_TYPE (c);

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

  if (ctype == MBWMClientTypeDialog   ||
      ctype == MBWMClientTypeMenu     ||
      ctype == MBWMClientTypeNote     ||
      ctype == MBWMClientTypeOverride)
    {
      shadow_type = mb_wm_theme_get_shadow_type (wm->theme);

      if (shadow_type == MBWM_COMP_MGR_SHADOW_NONE)
	{
	  clutter_container_add (CLUTTER_CONTAINER (actor), texture, NULL);
	}
      else
	{
	  if (shadow_type == MBWM_COMP_MGR_SHADOW_SIMPLE)
	    {
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
	    }
	  else
	    {
	      ClutterActor  * txt = cmgr->priv->shadow;
	      if (!txt)
		{
		  unsigned char * data;

		  data = mb_wm_comp_mgr_clutter_shadow_gaussian_make_tile ();

		  txt = g_object_new (CLUTTER_TYPE_TEXTURE, "tiled",
				      FALSE, NULL);

		  clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (txt),
						     data,
						     TRUE,
						     WIDTH,
						     HEIGHT,
						     WIDTH*4,
						     4,
						     0,
						     NULL);
		  free (data);

		  cmgr->priv->shadow = txt;
		}

	      rect = clutter_clone_texture_new (CLUTTER_TEXTURE (txt));

	      clutter_actor_set_position (rect,
					  2*SHADOW_RADIUS, 2*SHADOW_RADIUS);
	    }

	  clutter_actor_set_size (rect, geom.width, geom.height);
	  clutter_actor_show (rect);
	  clutter_container_add (CLUTTER_CONTAINER (actor),
				 rect, texture, NULL);
	}
    }
  else
    {
      clutter_container_add (CLUTTER_CONTAINER (actor), texture, NULL);
    }


  cclient->actor = actor;
  cclient->texture = texture;

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

  mb_wm_object_unref (MB_WM_OBJECT (d->c1));
  mb_wm_object_unref (MB_WM_OBJECT (d->c2));

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

  cb_data.c1 = mb_wm_object_ref (MB_WM_OBJECT (c1));
  cb_data.c2 = mb_wm_object_ref (MB_WM_OBJECT (c2));
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

  mb_wm_comp_mgr_clutter_transition_fade (cc1,
					  cc2,
					  100);
}

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
mb_wm_comp_mgr_clutter_effect_real (MBWMCompMgr            * mgr,
				    MBWindowManagerClient  * client,
				    MBWMCompMgrEffectEvent   event)
{
  static MBWMCompMgrClutterEffect * eff_map      = NULL;
  static MBWMCompMgrClutterEffect * eff_unmap    = NULL;
  static MBWMCompMgrClutterEffect * eff_minimize = NULL;

  MBWMCompMgrClutterEffect * eff;
  MBWMCompMgrClutterClient * cclient =
    MB_WM_COMP_MGR_CLUTTER_CLIENT (client->cm_client);

  if (MB_WM_CLIENT_CLIENT_TYPE (client) != MBWMClientTypeApp)
    return;

  switch (event)
    {
    case MBWMCompMgrEffectEventMap:
      if (!eff_map)
	eff_map = mb_wm_comp_mgr_clutter_effect_new (client->cm_client,
						     event,
						     200);
      eff = eff_map;
      break;
    case MBWMCompMgrEffectEventUnmap:
      if (!eff_unmap)
	eff_unmap = mb_wm_comp_mgr_clutter_effect_new (client->cm_client,
						       event, 200);
      eff = eff_unmap;
      break;
    case MBWMCompMgrEffectEventMinimize:
      if (!eff_minimize)
	eff_minimize = mb_wm_comp_mgr_clutter_effect_new (client->cm_client,
							  event, 200);
      eff = eff_minimize;
      break;
    default:
      ;
    }

  /*
   * Don't attempt to start the timeline if it is already playing
   */
  if (eff->timeline &&
      !clutter_timeline_is_playing (eff->timeline))
    {
      GSList * actors;
      ClutterActor *a;
      Bool dont_run = False;

      struct completed_cb_data * d;

      d = mb_wm_util_malloc0 (sizeof (struct completed_cb_data));

      d->cclient = mb_wm_object_ref (MB_WM_OBJECT (cclient));
      d->event   = event;

      d->my_id = g_signal_connect (eff->timeline, "completed",
		    G_CALLBACK (mb_wm_comp_mgr_clutter_effect_completed_cb),
				   d);

      /*
       * This is bit of a pain; we know that our actor is attached to
       * a single actor, but the current API only provides us with a copy
       * of the actor list; ideally, we would like to be able to access
       * the first actor directly.
       */
      actors = clutter_behaviour_get_actors (eff->behaviour);
      a = actors->data;

      if (CLUTTER_IS_BEHAVIOUR_PATH (eff->behaviour) &&
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
				     CLUTTER_BEHAVIOUR_PATH (eff->behaviour));

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
      else if (event == MBWMCompMgrEffectEventMinimize)
	{
	  /*
	   * This is tied specifically to the unmap scale effect (the
	   * themable version of effects allowed to handle this is a nice
	   * generic fashion. :-(
	   */
	  clutter_actor_move_anchor_point_from_gravity (a,
						CLUTTER_GRAVITY_SOUTH_EAST);
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
	  clutter_timeline_start (eff->timeline);
	}

      g_slist_free (actors);

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

/* ------------------------------- */
/* Shadow Generation */

typedef struct MBGaussianMap
{
  int	   size;
  double * data;
} MBGaussianMap;

static double
gaussian (double r, double x, double y)
{
  return ((1 / (sqrt (2 * M_PI * r))) *
	  exp ((- (x * x + y * y)) / (2 * r * r)));
}


static MBGaussianMap *
mb_wm_comp_mgr_clutter_make_gaussian_map (double r)
{
  MBGaussianMap  *c;
  int	          size = ((int) ceil ((r * 3)) + 1) & ~1;
  int	          center = size / 2;
  int	          x, y;
  double          t = 0.0;
  double          g;

  c = malloc (sizeof (MBGaussianMap) + size * size * sizeof (double));
  c->size = size;

  c->data = (double *) (c + 1);

  for (y = 0; y < size; y++)
    for (x = 0; x < size; x++)
      {
	g = gaussian (r, (double) (x - center), (double) (y - center));
	t += g;
	c->data[y * size + x] = g;
      }

  for (y = 0; y < size; y++)
    for (x = 0; x < size; x++)
      c->data[y*size + x] /= t;

  return c;
}

static unsigned char
mb_wm_comp_mgr_clutter_sum_gaussian (MBGaussianMap * map, double opacity,
				     int x, int y, int width, int height)
{
  int	           fx, fy;
  double         * g_data;
  double         * g_line = map->data;
  int	           g_size = map->size;
  int	           center = g_size / 2;
  int	           fx_start, fx_end;
  int	           fy_start, fy_end;
  double           v;
  unsigned int     r;

  /*
   * Compute set of filter values which are "in range",
   * that's the set with:
   *	0 <= x + (fx-center) && x + (fx-center) < width &&
   *  0 <= y + (fy-center) && y + (fy-center) < height
   *
   *  0 <= x + (fx - center)	x + fx - center < width
   *  center - x <= fx	fx < width + center - x
   */

  fx_start = center - x;
  if (fx_start < 0)
    fx_start = 0;
  fx_end = width + center - x;
  if (fx_end > g_size)
    fx_end = g_size;

  fy_start = center - y;
  if (fy_start < 0)
    fy_start = 0;
  fy_end = height + center - y;
  if (fy_end > g_size)
    fy_end = g_size;

  g_line = g_line + fy_start * g_size + fx_start;

  v = 0;
  for (fy = fy_start; fy < fy_end; fy++)
    {
      g_data = g_line;
      g_line += g_size;

      for (fx = fx_start; fx < fx_end; fx++)
	v += *g_data++;
    }
  if (v > 1)
    v = 1;

  v *= (opacity * 255.0);

  r = (unsigned int) v;

  return (unsigned char) r;
}

static unsigned char *
mb_wm_comp_mgr_clutter_shadow_gaussian_make_tile ()
{
  unsigned char              * data;
  int		               size;
  int		               center;
  int		               x, y;
  unsigned char                d;
  int                          pwidth, pheight;
  double                       opacity = SHADOW_OPACITY;
  static MBGaussianMap       * gaussian_map = NULL;

  struct _mypixel
  {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
  } * _d;


  if (!gaussian_map)
    gaussian_map =
      mb_wm_comp_mgr_clutter_make_gaussian_map (SHADOW_RADIUS);

  size   = gaussian_map->size;
  center = size / 2;

  /* Top & bottom */

  pwidth  = MAX_TILE_SZ;
  pheight = MAX_TILE_SZ;

  data = mb_wm_util_malloc0 (4 * WIDTH * HEIGHT);

  _d = (struct _mypixel*) data;

  /* N */
  for (y = 0; y < pheight; y++)
    {
      d = mb_wm_comp_mgr_clutter_sum_gaussian (gaussian_map, opacity,
					       center, y - center,
					       WIDTH, HEIGHT);
      for (x = 0; x < pwidth; x++)
	{
	  _d[y*3*pwidth + x + pwidth].r = 0;
	  _d[y*3*pwidth + x + pwidth].g = 0;
	  _d[y*3*pwidth + x + pwidth].b = 0;
	  _d[y*3*pwidth + x + pwidth].a = d;
	}

    }

  /* S */
  pwidth = MAX_TILE_SZ;
  pheight = MAX_TILE_SZ;

  for (y = 0; y < pheight; y++)
    {
      d = mb_wm_comp_mgr_clutter_sum_gaussian (gaussian_map, opacity,
					       center, y - center,
					       WIDTH, HEIGHT);
      for (x = 0; x < pwidth; x++)
	{
	  _d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + x + pwidth].r = 0;
	  _d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + x + pwidth].g = 0;
	  _d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + x + pwidth].b = 0;
	  _d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + x + pwidth].a = d;
	}

    }


  /* w */
  pwidth = MAX_TILE_SZ;
  pheight = MAX_TILE_SZ;

  for (x = 0; x < pwidth; x++)
    {
      d = mb_wm_comp_mgr_clutter_sum_gaussian (gaussian_map, opacity,
					       x - center, center,
					       WIDTH, HEIGHT);
      for (y = 0; y < pheight; y++)
	{
	  _d[y*3*pwidth + 3*pwidth*pheight + x].r = 0;
	  _d[y*3*pwidth + 3*pwidth*pheight + x].g = 0;
	  _d[y*3*pwidth + 3*pwidth*pheight + x].b = 0;
	  _d[y*3*pwidth + 3*pwidth*pheight + x].a = d;
	}

    }

  /* E */
  for (x = 0; x < pwidth; x++)
    {
      d = mb_wm_comp_mgr_clutter_sum_gaussian (gaussian_map, opacity,
					       x - center, center,
					       WIDTH, HEIGHT);
      for (y = 0; y < pheight; y++)
	{
	  _d[y*3*pwidth + 3*pwidth*pheight + (pwidth-x-1) + 2*pwidth].r = 0;
	  _d[y*3*pwidth + 3*pwidth*pheight + (pwidth-x-1) + 2*pwidth].g = 0;
	  _d[y*3*pwidth + 3*pwidth*pheight + (pwidth-x-1) + 2*pwidth].b = 0;
	  _d[y*3*pwidth + 3*pwidth*pheight + (pwidth-x-1) + 2*pwidth].a = d;
	}

    }

  /* NW */
  pwidth = MAX_TILE_SZ;
  pheight = MAX_TILE_SZ;

  for (x = 0; x < pwidth; x++)
    for (y = 0; y < pheight; y++)
      {
	d = mb_wm_comp_mgr_clutter_sum_gaussian (gaussian_map, opacity,
						 x-center, y-center,
						 WIDTH, HEIGHT);

	_d[y*3*pwidth + x].r = 0;
	_d[y*3*pwidth + x].g = 0;
	_d[y*3*pwidth + x].b = 0;
	_d[y*3*pwidth + x].a = d;
      }

  /* SW */
  for (x = 0; x < pwidth; x++)
    for (y = 0; y < pheight; y++)
      {
	d = mb_wm_comp_mgr_clutter_sum_gaussian (gaussian_map, opacity,
						 x-center, y-center,
						 WIDTH, HEIGHT);

	_d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + x].r = 0;
	_d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + x].g = 0;
	_d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + x].b = 0;
	_d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + x].a = d;
      }

  /* SE */
  for (x = 0; x < pwidth; x++)
    for (y = 0; y < pheight; y++)
      {
	d = mb_wm_comp_mgr_clutter_sum_gaussian (gaussian_map, opacity,
						 x-center, y-center,
						 WIDTH, HEIGHT);

	_d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + (pwidth-x-1) +
	   2*pwidth].r = 0;
	_d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + (pwidth-x-1) +
	   2*pwidth].g = 0;
	_d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + (pwidth-x-1) +
	   2*pwidth].b = 0;
	_d[(pheight-y-1)*3*pwidth + 6*pwidth*pheight + (pwidth-x-1) +
	   2*pwidth].a = d;
      }

  /* NE */
  for (x = 0; x < pwidth; x++)
    for (y = 0; y < pheight; y++)
      {
	d = mb_wm_comp_mgr_clutter_sum_gaussian (gaussian_map, opacity,
					 x-center, y-center, WIDTH, HEIGHT);

	_d[y*3*pwidth + (pwidth - x - 1) + 2*pwidth].r = 0;
	_d[y*3*pwidth + (pwidth - x - 1) + 2*pwidth].g = 0;
	_d[y*3*pwidth + (pwidth - x - 1) + 2*pwidth].b = 0;
	_d[y*3*pwidth + (pwidth - x - 1) + 2*pwidth].a = d;
      }

  /* center */
  pwidth = MAX_TILE_SZ;
  pheight = MAX_TILE_SZ;

  d = mb_wm_comp_mgr_clutter_sum_gaussian (gaussian_map, opacity,
					   center, center, WIDTH, HEIGHT);

  for (x = 0; x < pwidth; x++)
    for (y = 0; y < pheight; y++)
      {
	_d[y*3*pwidth + 3*pwidth*pheight + x + pwidth].r = 0;
	_d[y*3*pwidth + 3*pwidth*pheight + x + pwidth].g = 0;
	_d[y*3*pwidth + 3*pwidth*pheight + x + pwidth].b = 0;
	_d[y*3*pwidth + 3*pwidth*pheight + x + pwidth].a = d;
      }

  return data;
}

