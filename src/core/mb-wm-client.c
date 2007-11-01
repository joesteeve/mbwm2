/*
 *  Matchbox Window Manager II - A lightweight window manager not for the
 *                               desktop.
 *
 *  Authored By Matthew Allum <mallum@o-hand.com>
 *
 *  Copyright (c) 2005 OpenedHand Ltd - http://o-hand.com
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

struct MBWindowManagerClientPriv
{
  Bool          realized;
  Bool          mapped;
  Bool          iconizing;
  MBWMSyncType  sync_state;
};

static void
mb_wm_client_destroy (MBWMObject *obj)
{
  MBWindowManagerClient * client = MB_WM_CLIENT(obj);
  MBWindowManagerClient * c;
  MBWindowManager       * wm = client->wmref;
  MBWMList              * l = client->decor;

  if (client->sig_theme_change_id)
    mb_wm_object_signal_disconnect (MB_WM_OBJECT (wm),
				    client->sig_theme_change_id);
  client->sig_theme_change_id = 0;

  if (client->sig_prop_change_id)
    mb_wm_object_signal_disconnect (MB_WM_OBJECT (client->window),
				    client->sig_prop_change_id);

  client->sig_prop_change_id = 0;

  if (client->ping_cb_id)
    mb_wm_main_context_timeout_handler_remove (wm->main_ctx,
					       client->ping_cb_id);

#ifdef ENABLE_COMPOSITE
  if (mb_wm_compositing_enabled (wm))
    {
      mb_wm_comp_mgr_unregister_client (wm->comp_mgr, client);
    }
#endif

  mb_wm_object_unref (MB_WM_OBJECT (client->window));

  while (l)
    {
      mb_wm_object_unref (l->data);
      l = l->next;
    }

  if (client->transient_for)
    mb_wm_client_remove_transient (client->transient_for, client);

#if 1
  /* If we have transient windows, we need to make sure they are unmapped; for
   * application  dialogs this will happen automatically, but not for external
   * transients, such as input windows .
   */
  l = client->transients;
  while (l)
    {
      MBWindowManagerClient * c = l->data;
      XUnmapWindow (wm->xdpy, c->window->xwindow);
      l = l->next;
    }
#endif
}

static Bool
mb_wm_client_on_property_change (MBWMClientWindow *window,
				 int               property,
				 void             *userdata);

static Bool
mb_wm_client_on_theme_change (MBWindowManager * wm, int signal,
			      MBWindowManagerClient * client)
{
  mb_wm_client_theme_change (client);
  return False;
}


static int
mb_wm_client_init (MBWMObject *obj, va_list vap)
{
  MBWindowManagerClient *client;
  MBWindowManager       *wm = NULL;
  MBWMClientWindow      *win = NULL;
  MBWMObjectProp         prop;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropWm:
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	case MBWMObjectPropClientWindow:
	  win = va_arg(vap, MBWMClientWindow *);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  MBWM_MARK();

  client = MB_WM_CLIENT(obj);
  client->priv   = mb_wm_util_malloc0(sizeof(MBWindowManagerClientPriv));

  client->window        = win;
  client->wmref         = wm;
  client->ping_timeout  = 1000;
  client->want_focus    = 1;

  /* sync with client window */
  mb_wm_client_on_property_change (win, MBWM_WINDOW_PROP_ALL, client);

  /* Handle underlying property changes */
  client->sig_prop_change_id =
    mb_wm_object_signal_connect (MB_WM_OBJECT (win),
		 MBWM_WINDOW_PROP_ALL,
		 (MBWMObjectCallbackFunc)mb_wm_client_on_property_change,
		 client);

  client->sig_theme_change_id =
    mb_wm_object_signal_connect (MB_WM_OBJECT (wm),
		 MBWindowManagerSignalThemeChange,
		 (MBWMObjectCallbackFunc)mb_wm_client_on_theme_change,
		 client);


  return 1;
}

int
mb_wm_client_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWindowManagerClientClass),
	sizeof (MBWindowManagerClient),
	mb_wm_client_init,
	mb_wm_client_destroy,
	NULL
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

void
mb_wm_client_fullscreen_mark_dirty (MBWindowManagerClient *client)
{
  /* fullscreen implies geometry and visibility sync */
  mb_wm_display_sync_queue (client->wmref,
			    MBWMSyncFullscreen |
			    MBWMSyncVisibility | MBWMSyncGeometry);

  client->priv->sync_state |= (MBWMSyncFullscreen |
			       MBWMSyncGeometry   |
			       MBWMSyncVisibility);
}

void
mb_wm_client_stacking_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref, MBWMSyncStacking);
  client->priv->sync_state |= MBWMSyncStacking;
}

void
mb_wm_client_geometry_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref, MBWMSyncGeometry);

  client->priv->sync_state |= MBWMSyncGeometry;
}

void
mb_wm_client_visibility_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref, MBWMSyncVisibility);

  client->priv->sync_state |= MBWMSyncVisibility;

  MBWM_DBG(" sync state: %i", client->priv->sync_state);
}

void
mb_wm_client_synthetic_config_event_queue (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref, MBWMSyncSyntheticConfigEv);

  client->priv->sync_state |= MBWMSyncSyntheticConfigEv;

  MBWM_DBG(" sync state: %i", client->priv->sync_state);
}

Bool
mb_wm_client_needs_synthetic_config_event (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & MBWMSyncSyntheticConfigEv);
}

void
mb_wm_client_decor_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref, MBWMSyncDecor);

  client->priv->sync_state |= MBWMSyncDecor;

  MBWM_DBG(" sync state: %i", client->priv->sync_state);
}

Bool
mb_wm_client_needs_fullscreen_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & MBWMSyncFullscreen);
}

static Bool
mb_wm_client_on_property_change (MBWMClientWindow        *window,
				 int                      property,
				 void                    *userdata)
{
  MBWindowManagerClient * client = MB_WM_CLIENT (userdata);

  if (property & MBWM_WINDOW_PROP_NAME)
    {
      MBWMList * l = client->decor;
      while (l)
	{
	  MBWMDecor * decor = l->data;

	  if (mb_wm_decor_get_type (decor) == MBWMDecorTypeNorth)
	    {
	      mb_wm_decor_mark_dirty (decor);
	      break;
	    }

	  l = l->next;
	}
    }

  if (property & MBWM_WINDOW_PROP_GEOMETRY)
    mb_wm_client_geometry_mark_dirty (client);

#ifdef ENABLE_COMPOSITE
  if ((property & MBWM_WINDOW_PROP_CM_TRANSLUCENCY) &&
      client->cm_client && mb_wm_comp_mgr_enabled (client->wmref->comp_mgr))
    {
      mb_wm_comp_mgr_client_repair (client->cm_client);
    }
#endif

  return False;
}

MBWindowManagerClient* 	/* FIXME: rename to mb_wm_client_base/class_new ? */
mb_wm_client_new (MBWindowManager *wm, MBWMClientWindow *win)
{
  MBWindowManagerClient *client = NULL;

  client = MB_WM_CLIENT(mb_wm_object_new (MB_WM_TYPE_CLIENT,
					  MBWMObjectPropWm,           wm,
					  MBWMObjectPropClientWindow, win,
					  NULL));

  return client;
}

void
mb_wm_client_realize (MBWindowManagerClient *client)
{
  MBWindowManagerClientClass *klass;

  klass = MB_WM_CLIENT_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(client)));

  MBWM_ASSERT (klass->realize != NULL);

  klass->realize(client);

  client->priv->realized = True;
}

Bool
mb_wm_client_is_realized (MBWindowManagerClient *client)
{
  return client->priv->realized;
}


/* ## Stacking ## */


void
mb_wm_client_stack (MBWindowManagerClient *client,
		    int                    flags)
{
  MBWindowManagerClientClass *klass;

  klass = MB_WM_CLIENT_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(client)));

  MBWM_ASSERT (klass->stack != NULL);

  klass->stack(client, flags);

  mb_wm_client_stacking_mark_dirty (client);
}

Bool
mb_wm_client_needs_stack_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & MBWMSyncStacking);
}

void
mb_wm_client_show (MBWindowManagerClient *client)
{
  MBWindowManagerClientClass *klass;

  klass = MB_WM_CLIENT_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(client)));

  MBWM_ASSERT (klass->show != NULL);

  klass->show(client);

  client->priv->mapped = True;

  /* Make sure any Hidden state flag is cleared */
  mb_wm_client_set_state (client,
			  MBWM_ATOM_NET_WM_STATE_HIDDEN,
			  MBWMClientWindowStateChangeRemove);

  mb_wm_client_visibility_mark_dirty (client);
}


void
mb_wm_client_hide (MBWindowManagerClient *client)
{
  MBWindowManagerClientClass *klass;

  klass = MB_WM_CLIENT_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(client)));

  MBWM_ASSERT (klass->hide != NULL);

  klass->hide(client);

  client->priv->mapped = False;

  mb_wm_unfocus_client (client->wmref, client);
  mb_wm_client_visibility_mark_dirty (client);
}

/*
 * The focus processing is split into two stages, client and WM
 *
 * The client stage is handled by this function, with the return value
 * of True indicating that the focus has changed.
 *
 * NB: This function should be considered protected and only called by the
 * MBWindowManager object code.
 */
Bool
mb_wm_client_focus (MBWindowManagerClient *client)
{
  MBWindowManagerClientClass *klass;

  klass = MB_WM_CLIENT_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(client)));

  MBWM_ASSERT (klass->focus != NULL);

  return klass->focus(client);
}

Bool
mb_wm_client_want_focus (MBWindowManagerClient *client)
{
  return (client->want_focus != False);
}

Bool
mb_wm_client_needs_visibility_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & MBWMSyncVisibility);
}

Bool
mb_wm_client_needs_geometry_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & MBWMSyncGeometry);
}

Bool
mb_wm_client_needs_decor_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & MBWMSyncDecor);
}

Bool
mb_wm_client_needs_sync (MBWindowManagerClient *client)
{
  MBWM_DBG("sync_state: %i", client->priv->sync_state);

  return client->priv->sync_state;
}

Bool
mb_wm_client_is_mapped (MBWindowManagerClient *client)
{
  return client->priv->mapped;
}

void
mb_wm_client_display_sync (MBWindowManagerClient *client)
{
  MBWindowManagerClientClass *klass;

  klass = MB_WM_CLIENT_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(client)));

  MBWM_ASSERT (klass->sync != NULL);

  klass->sync(client);

#ifdef ENABLE_COMPOSITE
  if (client->cm_client)
    {
      if (client->priv->mapped)
	mb_wm_comp_mgr_client_show (client->cm_client);
      else
	mb_wm_comp_mgr_client_hide (client->cm_client);
    }
#endif

  client->priv->sync_state = 0;
}


Bool
mb_wm_client_request_geometry (MBWindowManagerClient *client,
			       MBGeometry            *new_geometry,
			       MBWMClientReqGeomType  flags)
{
  MBWindowManagerClientClass *klass;

  klass = MB_WM_CLIENT_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(client)));

  MBWM_ASSERT (klass->geometry != NULL);

  return klass->geometry(client, new_geometry, flags);
}

MBWMClientLayoutHints
mb_wm_client_get_layout_hints (MBWindowManagerClient *client)
{
  return client->layout_hints;
}

void
mb_wm_client_set_layout_hints (MBWindowManagerClient *client,
			       MBWMClientLayoutHints  hints)
{
  client->layout_hints = hints;
}

void
mb_wm_client_set_layout_hint (MBWindowManagerClient *client,
			      MBWMClientLayoutHints  hint,
			      Bool                   state)
{
  if (state)
    client->layout_hints |= hint;
  else
    client->layout_hints &= ~hint;
}

void  /* needs to be boolean, client may not have any coverage */
mb_wm_client_get_coverage (MBWindowManagerClient *client,
			   MBGeometry            *coverage)
{
  MBGeometry *geometry = &client->frame_geometry;

  if (!client->xwin_frame)
    {
      MBGeometry *win_geo = &client->window->geometry;
      geometry->x      = win_geo->x;
      geometry->y      = win_geo->y;
      geometry->width  = win_geo->width;
      geometry->height = win_geo->height;
    }

  coverage->x      = geometry->x;
  coverage->y      = geometry->y;
  coverage->width  = geometry->width;
  coverage->height = geometry->height;
}

void
mb_wm_client_add_transient (MBWindowManagerClient *client,
			    MBWindowManagerClient *transient)
{
  if (transient == NULL || client == NULL)
    return;

  transient->transient_for = client;
  client->transients = mb_wm_util_list_append(client->transients, transient);
}

void
mb_wm_client_remove_transient (MBWindowManagerClient *client,
			       MBWindowManagerClient *transient)
{
  if (transient == NULL || client == NULL)
    return;

  transient->transient_for = NULL;
  client->transients = mb_wm_util_list_remove(client->transients, transient);
}

const MBWMList*
mb_wm_client_get_transients (MBWindowManagerClient *client)
{
  return client->transients;
}

MBWindowManagerClient*
mb_wm_client_get_transient_for (MBWindowManagerClient *client)
{
  return client->transient_for;
}


const char*
mb_wm_client_get_name (MBWindowManagerClient *client)
{
  if (!client->window)
    return NULL;

  return client->window->name;
}

static void
mb_wm_client_deliver_message (MBWindowManagerClient   *client,
			      Atom          delivery_atom,
			      unsigned long data0,
			      unsigned long data1,
			      unsigned long data2,
			      unsigned long data3,
			      unsigned long data4)
{
  MBWindowManager *wm = client->wmref;
  Window xwin = client->window->xwindow;
  XEvent ev;

  memset(&ev, 0, sizeof(ev));

  ev.xclient.type = ClientMessage;
  ev.xclient.window = xwin;
  ev.xclient.message_type = delivery_atom;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = data0;
  ev.xclient.data.l[1] = data1;
  ev.xclient.data.l[2] = data2;
  ev.xclient.data.l[3] = data3;
  ev.xclient.data.l[4] = data4;

  XSendEvent(wm->xdpy, xwin, False, NoEventMask, &ev);

  XSync(wm->xdpy, False);
}

void
mb_wm_client_deliver_wm_protocol (MBWindowManagerClient *client,
				  Atom protocol)
{
  MBWindowManager *wm = client->wmref;

  mb_wm_client_deliver_message (client, wm->atoms[MBWM_ATOM_WM_PROTOCOLS],
				protocol, CurrentTime, 0, 0, 0);
}

static void
mb_wm_client_deliver_ping_protocol (MBWindowManagerClient *client)
{
  MBWindowManager *wm = client->wmref;

  mb_wm_client_deliver_message (client,
				wm->atoms[MBWM_ATOM_WM_PROTOCOLS],
				wm->atoms[MBWM_ATOM_NET_WM_PING],
				CurrentTime,
				client->window->xwindow,
				0, 0);
}

static Bool
mb_wm_client_ping_timeout_cb (void * userdata)
{
  MBWindowManagerClient * client = userdata;

  mb_wm_client_shutdown (client);
  return True;
}

static void
mb_wm_client_ping_start (MBWindowManagerClient *client)
{
  MBWindowManager * wm = client->wmref;
  MBWMMainContext * ctx = wm->main_ctx;

  if (client->ping_cb_id)
    return;

  client->ping_cb_id =
    mb_wm_main_context_timeout_handler_add (ctx, client->ping_timeout,
					    mb_wm_client_ping_timeout_cb,
					    client);

  mb_wm_client_deliver_ping_protocol (client);
}

void
mb_wm_client_ping_stop (MBWindowManagerClient *client)
{
  MBWMMainContext * ctx = client->wmref->main_ctx;

  if (!client->ping_cb_id)
    return;

  mb_wm_main_context_timeout_handler_remove (ctx, client->ping_cb_id);
  client->ping_cb_id = 0;
}

void
mb_wm_client_shutdown (MBWindowManagerClient *client)
{
  char               buf[257];
  int                sig     = 9;
  MBWindowManager   *wm      = client->wmref;
  MBWMClientWindow  *win     = client->window;
  Window             xwin    = client->window->xwindow;
  const char        *machine = win->machine;
  pid_t              pid     = win->pid;

  if (!machine || !pid)
    return;

  if (gethostname (buf, sizeof(buf)-1) == 0)
    {
      if (!strcmp (buf, machine))
	{
	  if (kill (pid, sig) >= 0)
	    return;
	}
    }

  XKillClient(wm->xdpy, xwin);
}

void
mb_wm_client_deliver_delete (MBWindowManagerClient *client)
{
  MBWindowManager        *wm     = client->wmref;
  Window                  xwin   = client->window->xwindow;
  MBWMClientWindowProtos  protos = client->window->protos;

  if ((protos & MBWMClientWindowProtosDelete))
    {
      mb_wm_client_deliver_wm_protocol (client,
				wm->atoms[MBWM_ATOM_WM_DELETE_WINDOW]);

      mb_wm_client_ping_start (client);
    }
  else
    mb_wm_client_shutdown (client);
}

void
mb_wm_client_set_state (MBWindowManagerClient *client,
			Atom state,
			MBWMClientWindowStateChange state_op)
{
  MBWindowManager   *wm  = client->wmref;
  MBWMClientWindow  *win = client->window;
  Bool               old_state;
  Bool               new_state;
  Bool               activate = True;
  MBWMClientWindowEWMHState state_flag;

  switch (state)
    {
    case MBWM_ATOM_NET_WM_STATE_FULLSCREEN:
      state_flag = MBWMClientWindowEWMHStateFullscreen;
      break;
    case MBWM_ATOM_NET_WM_STATE_ABOVE:
      state_flag = MBWMClientWindowEWMHStateAbove;
      break;
    case MBWM_ATOM_NET_WM_STATE_HIDDEN:
      state_flag = MBWMClientWindowEWMHStateHidden;
      break;
    case MBWM_ATOM_NET_WM_STATE_MODAL:
    case MBWM_ATOM_NET_WM_STATE_STICKY:
    case MBWM_ATOM_NET_WM_STATE_MAXIMIZED_VERT:
    case MBWM_ATOM_NET_WM_STATE_MAXIMIZED_HORZ:
    case MBWM_ATOM_NET_WM_STATE_SHADED:
    case MBWM_ATOM_NET_WM_STATE_SKIP_TASKBAR:
    case MBWM_ATOM_NET_WM_STATE_SKIP_PAGER:
    case MBWM_ATOM_NET_WM_STATE_BELOW:
    case MBWM_ATOM_NET_WM_STATE_DEMANDS_ATTENTION:
    default:
      return; /* not handled yet */
    }

  old_state = (win->ewmh_state & state_flag);

  switch (state_op)
    {
    case MBWMClientWindowStateChangeRemove:
      new_state = False;
      break;
    case MBWMClientWindowStateChangeAdd:
      new_state = True;
      break;
    case MBWMClientWindowStateChangeToggle:
      new_state != old_state;
      break;
    }

  if (new_state == old_state)
    return;

  if (new_state)
    {
      win->ewmh_state |= state_flag;
    }
  else
    {
      win->ewmh_state &= ~state_flag;
    }

  if (new_state && (state_flag & MBWMClientWindowEWMHStateHidden))
    {
      mb_wm_client_hide (client);
      return;
    }

  if ((state_flag & MBWMClientWindowEWMHStateFullscreen))
    {
      mb_wm_client_fullscreen_mark_dirty (client);
    }

  /*
   * FIXME -- resize && move, possibly redraw decors when returning from
   * fullscreen
   */
  if (activate)
    mb_wm_activate_client(wm, client);
}

Bool
mb_wm_client_ping_in_progress (MBWindowManagerClient * client)
{
  return (client->ping_cb_id != 0);
}

void
mb_wm_client_theme_change (MBWindowManagerClient *client)
{
  MBWindowManagerClientClass *klass;

  klass = MB_WM_CLIENT_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(client)));

  if (klass->theme_change)
    klass->theme_change (client);
}

/*
 * Cleanup any transient relationships this client might have
 * (we need to do this when the client window unmaps to ensure correct
 * functioning of the stack).
 */
void
mb_wm_client_detransitise (MBWindowManagerClient *client)
{
  MBWindowManagerClientClass *klass;

  if (!client->transient_for)
    return;

  klass = MB_WM_CLIENT_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(client)));

  if (klass->detransitise)
    klass->detransitise (client);

  mb_wm_client_remove_transient (client->transient_for, client);
}

Bool
mb_wm_client_is_iconizing (MBWindowManagerClient *client)
{
  return client->priv->iconizing;
}

void
mb_wm_client_reset_iconizing (MBWindowManagerClient *client)
{
  client->priv->iconizing = False;
}

void
mb_wm_client_iconize (MBWindowManagerClient *client)
{
  /*
   * Set the iconizing flag and put the window into hidden state
   * This triggers an umap event, at which point the client gets unmanaged
   * by the window manager.
   */
  client->priv->iconizing = True;

  mb_wm_client_set_state (client,
			  MBWM_ATOM_NET_WM_STATE_HIDDEN,
			  MBWMClientWindowStateChangeAdd);
}

int
mb_wm_client_title_height (MBWindowManagerClient *client)
{
  MBWMClientWindow * win = client->window;
  MBWindowManager  * wm  = client->wmref;
  int                north;

  if (!wm->theme ||
      mb_wm_client_window_is_state_set (win,
					MBWMClientWindowEWMHStateFullscreen))
    {
      return 0;
    }


  mb_wm_theme_get_decor_dimensions (wm->theme, client,
				    &north, NULL, NULL, NULL);

  return north;
}

Bool
mb_wm_client_is_modal (MBWindowManagerClient *client)
{
  return mb_wm_client_window_is_state_set (client->window,
					   MBWMClientWindowEWMHStateModal);
}

