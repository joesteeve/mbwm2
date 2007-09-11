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

enum
  {
    StackingNeedsSync      = (1<<1),
    GeometryNeedsSync      = (1<<2),
    VisibilityNeedsSync    = (1<<3),
    DecorNeedsSync         = (1<<4),
    SyntheticConfigEvSync  = (1<<5)
  };

struct MBWindowManagerClientPriv
{
  Bool realized;
  Bool mapped;
  int  sync_state;
};

static void
mb_wm_client_destroy (MBWMObject *obj)
{
  MBWindowManagerClient * client = MB_WM_CLIENT(obj);
  mb_wm_display_sync_queue (client->wmref);

  mb_wm_object_unref (MB_WM_OBJECT (client->window));
  mb_wm_object_unref (MB_WM_OBJECT (client->wmref));
}

static void
mb_wm_client_init (MBWMObject *obj, va_list vap)
{
  MBWindowManagerClient *client;

  MBWM_MARK();

  client = MB_WM_CLIENT(obj);

  client->priv   = mb_wm_util_malloc0(sizeof(MBWindowManagerClientPriv));

  client->pings_pending = -1;

  /*
  if (client->init)
    client->init(wm, client, win);
  else
    mb_wm_client_base_init (wm, client, win);
  */
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
mb_wm_client_stacking_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref);
  client->priv->sync_state |= StackingNeedsSync;
}

void
mb_wm_client_geometry_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref);

  client->priv->sync_state |= GeometryNeedsSync;
}

void
mb_wm_client_visibility_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref);

  client->priv->sync_state |= VisibilityNeedsSync;

  MBWM_DBG(" sync state: %i", client->priv->sync_state);
}

void
mb_wm_client_synthetic_config_event_queue (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref);

  client->priv->sync_state |= SyntheticConfigEvSync;

  MBWM_DBG(" sync state: %i", client->priv->sync_state);
}

Bool
mb_wm_client_needs_synthetic_config_event (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & SyntheticConfigEvSync);
}

void
mb_wm_client_decor_mark_dirty (MBWindowManagerClient *client)
{
  mb_wm_display_sync_queue (client->wmref);

  client->priv->sync_state |= DecorNeedsSync;

  MBWM_DBG(" sync state: %i", client->priv->sync_state);
}

static Bool
mb_wm_client_on_property_change (MBWMClientWindow        *window,
				 int                      property,
				 void                    *userdata)
{
  printf("One of my underlying properties changed - %i\n", property);
}

MBWindowManagerClient* 	/* FIXME: rename to mb_wm_client_base/class_new ? */
mb_wm_client_new (MBWindowManager *wm, MBWMClientWindow *win)
{
  MBWindowManagerClient *client = NULL;

  client = MB_WM_CLIENT(mb_wm_object_new (MB_WM_TYPE_CLIENT,
					  NULL));

  if (!client)
    return NULL; 		/* FIXME: Handle out of memory */

  client->window = win;

  /* Handle underlying property changes */
  mb_wm_object_signal_connect (MB_WM_OBJECT (win),
			       MBWM_WINDOW_PROP_ALL,
			       mb_wm_client_on_property_change,
			       (void*)client);
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
  return (client->priv->sync_state & StackingNeedsSync);
}

void
mb_wm_client_show (MBWindowManagerClient *client)
{
  MBWindowManagerClientClass *klass;

  klass = MB_WM_CLIENT_CLASS(mb_wm_object_get_class (MB_WM_OBJECT(client)));

  MBWM_ASSERT (klass->show != NULL);

  /* FIXME: Need to re-add to stack here ? */

  klass->show(client);

  client->priv->mapped = True;

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

  /* remove it from stack */
  mb_wm_stack_remove (client);

  mb_wm_client_visibility_mark_dirty (client);
}

Bool
mb_wm_client_needs_visibility_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & VisibilityNeedsSync);
}

Bool
mb_wm_client_needs_geometry_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & GeometryNeedsSync);
}

Bool
mb_wm_client_needs_decor_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & DecorNeedsSync);
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
  MBGeometry *geometry;

  if (client->xwin_frame)
    geometry = &client->frame_geometry;
  else
    geometry = &client->window->geometry;

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

static void
mb_wm_client_deliver_wm_protocol (MBWindowManagerClient *client,
				  Atom protocol)
{
  MBWindowManager *wm = client->wmref;

  mb_wm_client_deliver_message (client, wm->atoms[MBWM_ATOM_WM_PROTOCOLS],
				protocol, CurrentTime, 0, 0, 0);
}

static void
mb_wm_client_ping_start (MBWindowManagerClient *client)
{
#ifndef NO_PING
  if (client->pings_pending == -1)
    {
      client->pings_pending       = 0;
      client->pings_sent          = 0;
      client->wmref->n_active_ping_clients++;
    }
#endif
}

static void
mb_wm_client_ping_stop (MBWindowManagerClient *client)
{
#ifndef NO_PING
  if (client->pings_pending != -1)
    {
      client->pings_pending = -1;
      client->wmref->n_active_ping_clients--;
    }
#endif
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

      if (client->pings_pending == -1)
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
  MBWMClientWindowEWMHState state_flag;

  switch (state)
    {
    case MBWM_ATOM_NET_WM_STATE_FULLSCREEN:
      state_flag = MBWMClientWindowEWMHStateFullscreen;
      break;
    case MBWM_ATOM_NET_WM_STATE_ABOVE:
      state_flag = MBWMClientWindowEWMHStateAbove;
      break;

    case MBWM_ATOM_NET_WM_STATE_MODAL:
    case MBWM_ATOM_NET_WM_STATE_STICKY:
    case MBWM_ATOM_NET_WM_STATE_MAXIMIZED_VERT:
    case MBWM_ATOM_NET_WM_STATE_MAXIMIZED_HORZ:
    case MBWM_ATOM_NET_WM_STATE_SHADED:
    case MBWM_ATOM_NET_WM_STATE_SKIP_TASKBAR:
    case MBWM_ATOM_NET_WM_STATE_SKIP_PAGER:
    case MBWM_ATOM_NET_WM_STATE_HIDDEN:
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

  /*
   * FIXME -- resize && move, possibly redraw decors when returning from
   * fullscreen
   */
  mb_wm_activate_client(wm, client);
}
