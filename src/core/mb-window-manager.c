#include "mb-wm.h"
#include "../client-types/mb-wm-client-app.h"
#include "../client-types/mb-wm-client-panel.h"
#include "../client-types/mb-wm-client-dialog.h"
#include "../client-types/mb-wm-client-desktop.h"
#include "../client-types/mb-wm-client-input.h"
#include "../theme-engines/mb-wm-theme.h"

#include <stdarg.h>

#include <X11/Xmd.h>

static void
mb_wm_process_cmdline (MBWindowManager *wm, int argc, char **argv);

static void
mb_wm_focus_client (MBWindowManager *wm, MBWindowManagerClient *client);

static void
mb_wm_update_root_win_rectangles (MBWindowManager *wm);

static MBWindowManagerClient*
mb_wm_client_new_func (MBWindowManager *wm, MBWMClientWindow *win)
{
  if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DOCK])
    {
      printf("### is panel ###\n");
      return mb_wm_client_panel_new(wm, win);
    }
  else if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DIALOG])
    {
      printf("### is dialog ###\n");
      return mb_wm_client_dialog_new(wm, win);
    }
  else if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DESKTOP])
    {
      printf("### is desktop ###\n");
      /* Only one desktop allowed */
      if (wm->desktop)
	return NULL;

      return mb_wm_client_desktop_new (wm, win);
    }
  else if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_TOOLBAR])
    {
      printf("### is input ###\n");
      return mb_wm_client_input_new (wm, win);
    }
  else
    {
      return mb_wm_client_app_new(wm, win);
    }
}

static MBWMTheme *
mb_wm_real_theme_new (MBWindowManager * wm, const char * path)
{
  /*
   *  FIXME -- load the selected theme from some configuration
   */
  return mb_wm_theme_new (wm, path);
}

static void
mb_wm_class_init (MBWMObjectClass *klass)
{
  MBWindowManagerClass *wm_class;

  MBWM_MARK();

  wm_class = (MBWindowManagerClass *)klass;

  wm_class->process_cmdline = mb_wm_process_cmdline;
  wm_class->client_new      = mb_wm_client_new_func;
  wm_class->theme_new       = mb_wm_real_theme_new;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWindowManager";
#endif
}

static void
mb_wm_destroy (MBWMObject *this)
{
  MBWindowManager * wm = MB_WINDOW_MANAGER (this);
  MBWMList *l = wm->clients;

  while (l)
    {
      MBWMList * old = l;
      mb_wm_object_unref (l->data);

      l = l->next;

      free (old);
    }

  l = wm->unmapped_clients;
  while (l)
    {
      MBWMList * old = l;
      mb_wm_object_unref (l->data);

      l = l->next;
      free (old);
    }

  mb_wm_object_unref (MB_WM_OBJECT (wm->root_win));
  mb_wm_object_unref (MB_WM_OBJECT (wm->theme));
  mb_wm_object_unref (MB_WM_OBJECT (wm->layout));
  mb_wm_object_unref (MB_WM_OBJECT (wm->main_ctx));
}

static int
mb_wm_init (MBWMObject *this, va_list vap);

int
mb_wm_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWindowManagerClass),
	sizeof (MBWindowManager),
	mb_wm_init,
	mb_wm_destroy,
	mb_wm_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

MBWindowManager*
mb_wm_new (int argc, char **argv)
{
  MBWindowManager      *wm = NULL;

  wm = MB_WINDOW_MANAGER (mb_wm_object_new (MB_TYPE_WINDOW_MANAGER,
					    MBWMObjectPropArgc, argc,
					    MBWMObjectPropArgv, argv,
					    NULL));

  if (!wm)
    return wm;

  return wm;
}

Bool
test_key_press (XKeyEvent       *xev,
		void            *userdata)
{
  MBWindowManager *wm = (MBWindowManager*)userdata;

  mb_wm_keys_press (wm,
		    XKeycodeToKeysym(wm->xdpy, xev->keycode, 0),
		    xev->state);

  return True;
}

Bool
test_button_press (XButtonEvent *xev, void *userdata)
{
  MBWindowManager *wm = (MBWindowManager*)userdata;
  MBWindowManagerClient *client = NULL;

  if (xev->button != 1)
    return True;

  client = mb_wm_managed_client_from_xwindow(wm, xev->window);

  if (!client)
    return True;

  mb_wm_focus_client (wm, client);

  return True;
}

Bool
test_destroy_notify (XDestroyWindowEvent  *xev,
		     void                 *userdata)
{
  MBWindowManager       *wm = (MBWindowManager*)userdata;
  MBWindowManagerClient *client = NULL;

  MBWM_MARK();

  client = mb_wm_managed_client_from_xwindow(wm, xev->window);

  if (client)
    {
      MBWM_DBG("Found client, unmanaing!");
      mb_wm_unmanage_client (wm, client, True);
    }
  else
    {
      client = mb_wm_unmapped_client_from_xwindow(wm, xev->window);

      if (client)
	{
	  wm->unmapped_clients =
	    mb_wm_util_list_remove (wm->unmapped_clients,
				    client);

	  mb_wm_object_unref (MB_WM_OBJECT (client));
	}
    }

  return True;
}

Bool
test_unmap_notify (XUnmapEvent          *xev,
		   void                 *userdata)
{
  MBWindowManager       *wm = (MBWindowManager*)userdata;
  MBWindowManagerClient *client = NULL;

  MBWM_MARK();

  client = mb_wm_managed_client_from_xwindow(wm, xev->window);

  if (client)
    {
      if (client->skip_unmaps)
	{
	  client->skip_unmaps--;
	}
      else
	{
 	  mb_wm_unmanage_client (wm, client, False);
	}
    }

  return True;
}

static Bool
mb_wm_handle_property_notify (XPropertyEvent          *xev,
			      void                    *userdata)
{
  MBWindowManager       *wm = (MBWindowManager*)userdata;
  MBWindowManagerClient *client;
  int flag = 0;

  if (xev->window == wm->root_win->xwindow)
    {
      if (xev->atom == wm->atoms[MBWM_ATOM_MB_THEME])
	{
	  Atom type;
	  int  format;
	  unsigned long items;
	  unsigned long left;
	  char         *theme_path;

	  XGetWindowProperty (wm->xdpy, wm->root_win->xwindow,
			      xev->atom, 0, 8192, False,
			      XA_STRING, &type, &format,
			      &items, &left,
			      (unsigned char **)&theme_path);

	  if (!type || !items)
	    return True;

	  mb_wm_set_theme_from_path (wm, theme_path);

	  XFree (theme_path);
	}

      return True;
    }

  client = mb_wm_managed_client_from_xwindow(wm, xev->window);

  if (!client)
    return True;

  if (xev->atom == wm->atoms[MBWM_ATOM_NET_WM_USER_TIME])
    flag = MBWM_WINDOW_PROP_NET_USER_TIME;
  else if (xev->atom == wm->atoms[MBWM_ATOM_WM_NAME])
    flag = MBWM_WINDOW_PROP_NAME;
  else if (xev->atom == wm->atoms[MBWM_ATOM_WM_HINTS])
    flag = MBWM_WINDOW_PROP_WM_HINTS;
  else if (xev->atom == wm->atoms[MBWM_ATOM_NET_WM_ICON])
    flag = MBWM_WINDOW_PROP_NET_ICON;
  else if (xev->atom == wm->atoms[MBWM_ATOM_WM_PROTOCOLS])
    flag = MBWM_WINDOW_PROP_PROTOS;
  else if (xev->atom == wm->atoms[MBWM_ATOM_WM_TRANSIENT_FOR])
    flag = MBWM_WINDOW_PROP_TRANSIENCY;
  else if (xev->atom == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE])
    flag = MBWM_WINDOW_PROP_WIN_TYPE;
  else if (xev->atom == wm->atoms[MBWM_ATOM_WM_CLIENT_MACHINE])
    flag = MBWM_WINDOW_PROP_CLIENT_MACHINE;
  else if (xev->atom == wm->atoms[MBWM_ATOM_NET_WM_PID])
    flag = MBWM_WINDOW_PROP_NET_PID;

  if (flag)
    mb_wm_client_window_sync_properties (client->window, flag);

  return True;
}

static  Bool
mb_wm_handle_config_request (XConfigureRequestEvent *xev,
			     void                   *userdata)
{
  MBWindowManager       *wm = (MBWindowManager*)userdata;
  MBWindowManagerClient *client;
  unsigned long          value_mask;
  int                    req_x, req_y, req_w, req_h;
  MBGeometry             req_geom, *win_geom;

  client = mb_wm_managed_client_from_xwindow(wm, xev->window);

  if (!client)
    {
      MBWM_DBG("### No client found for configure event ###");
      return True;
    }

  value_mask = xev->value_mask;
  win_geom   = &client->window->geometry;

  req_geom.x      = (value_mask & CWX) ? xev->x : win_geom->x;
  req_geom.y      = (value_mask & CWY) ? xev->y : win_geom->y;
  req_geom.width  = (value_mask & CWWidth) ? xev->width : win_geom->width;
  req_geom.height = (value_mask & CWHeight) ? xev->height : win_geom->height;


#if 0  /* stacking to sort */
  if (value_mask & (CWSibling|CWStackMode))
    {

    }
#endif

  if (mb_geometry_compare (&req_geom, win_geom))
    {
      /* No change in window geometry, but needs configure request
       * per ICCCM.
      */
      mb_wm_client_synthetic_config_event_queue (client);
      return;
    }

  if (mb_wm_client_request_geometry (client,
				     &req_geom,
				     MBWMClientReqGeomIsViaConfigureReq))
    {
      if (req_geom.width == win_geom->width
	  && req_geom.height == win_geom->height)
	{
	  /* ICCCM says if window only moved - not size change,
	   * then we send a fake one too.
	   */
	  mb_wm_client_synthetic_config_event_queue (client);
	}

    }

  return True;
}

static Bool
mb_wm_handle_map_request (XMapRequestEvent  *xev,
			  void              *userdata)
{
  MBWindowManager       *wm = (MBWindowManager*)userdata;
  MBWindowManagerClient *client = NULL;
  MBWindowManagerClass  *wm_class =
    MB_WINDOW_MANAGER_CLASS (MB_WM_OBJECT_GET_CLASS (wm));

  MBWM_MARK();

  if ((client = mb_wm_managed_client_from_xwindow(wm, xev->window)))
    {
      /* This client is already managed, but for some reason not mapped
       * -- try to activate it.
       */
      mb_wm_activate_client (wm, client);
      return;
    }

  client = mb_wm_unmapped_client_from_xwindow(wm, xev->window);

  if (client)
    {
      wm->unmapped_clients = mb_wm_util_list_remove (wm->unmapped_clients,
						      client);
    }
  else
    {
      MBWMClientWindow *win = NULL;

      if (!wm_class->client_new)
	{
	  MBWM_DBG("### No new client hook exists ###");
	  return True;
	}

      win = mb_wm_client_window_new (wm, xev->window);

      if (!win)
	return True;

      client = wm_class->client_new (wm, win);

      if (!client)
	{
	  mb_wm_object_unref (MB_WM_OBJECT (win));
	  return True;
	}
    }

  mb_wm_manage_client(wm, client, True);

  return True;
}


static void
stack_get_window_list (MBWindowManager *wm, Window * win_list)
{
  MBWindowManagerClient *client;
  int                    i = 0;

  if (!wm->stack_n_clients)
    return;

  if ((wm->flags & MBWindowManagerFlagDesktop) && wm->desktop)
    {
      if (wm->desktop->xwin_frame)
	win_list[i++] = wm->desktop->xwin_frame;
      else
	win_list[i++] = MB_WM_CLIENT_XWIN(wm->desktop);
    }

  mb_wm_stack_enumerate_reverse(wm, client)
  {
    if ((wm->flags & MBWindowManagerFlagDesktop) && wm->desktop == client)
      continue;

    if (client->xwin_frame)
      win_list[i++] = client->xwin_frame;
    else
      win_list[i++] = MB_WM_CLIENT_XWIN(client);
  }
}

static void
stack_sync_to_display (MBWindowManager *wm)
{
  Window *win_list = NULL;

  if (!wm->stack_n_clients)
    return;

  mb_wm_stack_ensure (wm);

  win_list = alloca (sizeof(Window)*(wm->stack_n_clients));
  stack_get_window_list(wm, win_list);

  mb_wm_util_trap_x_errors();
  XRestackWindows(wm->xdpy, win_list, wm->stack_n_clients);
  mb_wm_util_untrap_x_errors();
}

void
mb_wm_sync (MBWindowManager *wm)
{
  /* Sync all changes to display */
  MBWindowManagerClient *client = NULL;

  MBWM_MARK();

  XGrabServer(wm->xdpy);

  /* Size stuff first assume newly managed windows unmapped ?
   *
  */
  if (wm->layout)
    mb_wm_layout_update (wm->layout);

  /* Create the actual windows */
  mb_wm_stack_enumerate(wm, client)
    if (!mb_wm_client_is_realized (client))
      mb_wm_client_realize (client);

  /* FIXME: optimise wm sync flags so know if this needs calling */
  /* FIXME: Can we restack an unmapped window ? - problem of new
   *        clients mapping below existing ones.
  */
  if (wm->sync_type & MBWMSyncStacking)
    stack_sync_to_display (wm);

  /* Now do updates per individual client - maps, paints etc, main work here */
  mb_wm_stack_enumerate(wm, client)
    if (mb_wm_client_needs_sync (client))
      mb_wm_client_display_sync (client);

  /* FIXME: New clients now managed will likely need some propertys
   *        synced up here.
  */

  XUngrabServer(wm->xdpy);

  wm->sync_type = 0;
}

static void
mb_wm_update_root_win_lists (MBWindowManager *wm)
{
  Window root_win = wm->root_win->xwindow;

  if (!mb_wm_stack_empty(wm))
    {
      Window                *wins = NULL;
      Window                *app_wins = NULL;
      int                    app_win_cnt = 0;
      int                    cnt = 0;
      int                    list_size;
      MBWindowManagerClient *c;
      MBWMList              *l;

      list_size = mb_wm_util_list_length (wm->clients) +
	mb_wm_util_list_length (wm->unmapped_clients);

      wins      = alloca (sizeof(Window) * list_size);
      app_wins  = alloca (sizeof(Window) * list_size);

      if ((wm->flags & MBWindowManagerFlagDesktop) && wm->desktop)
	{
	  if (wm->desktop->xwin_frame)
	    wins[cnt++] = wm->desktop->xwin_frame;
	  else
	    wins[cnt++] = MB_WM_CLIENT_XWIN(wm->desktop);
	}

      mb_wm_stack_enumerate (wm,c)
	{
	  if (!(wm->flags & MBWindowManagerFlagDesktop) || c != wm->desktop)
	    wins[cnt++] = c->window->xwindow;

	  if (MB_WM_IS_CLIENT_APP (c))
	    app_wins[app_win_cnt++] = c->window->xwindow;
	}

      /* The MB_APP_WINDOW_LIST_STACKING list is used to construct
       * application switching menus -- we append anything we have
       * in the unmapped_windows list that is marked hidden (i.e., mimimized
       * apps)
       */
      l = wm->unmapped_clients;
      while (l)
	{
	  c = l->data;

	  if (MB_WM_IS_CLIENT_APP (c) &&
	      mb_wm_client_window_is_state_set(c->window,
				       MBWMClientWindowEWMHStateHidden))
	    {
	      app_wins[app_win_cnt++] = c->window->xwindow;
	    }

	  l = l->next;
	}

      XChangeProperty(wm->xdpy, root_win,
		      wm->atoms[MBWM_ATOM_NET_CLIENT_LIST_STACKING],
		      XA_WINDOW, 32, PropModeReplace,
		      (unsigned char *)wins, cnt);

      XChangeProperty(wm->xdpy, root_win,
		      wm->atoms[MBWM_ATOM_MB_APP_WINDOW_LIST_STACKING],
		      XA_WINDOW, 32, PropModeReplace,
		      (unsigned char *)app_wins, app_win_cnt);

      /* Update _NET_CLIENT_LIST but with 'age' order rather than stacking */
      cnt = 0;
      l = wm->clients;
      while (l)
	{
	  c = l->data;
	  wins[cnt++] = c->window->xwindow;

	  l = l->next;
	}

      XChangeProperty(wm->xdpy, root_win,
		      wm->atoms[MBWM_ATOM_NET_CLIENT_LIST] ,
		      XA_WINDOW, 32, PropModeReplace,
		      (unsigned char *)wins, cnt);
    }
  else
    {
      /* No managed windows */
      XChangeProperty(wm->xdpy, root_win,
		      wm->atoms[MBWM_ATOM_NET_CLIENT_LIST_STACKING] ,
		      XA_WINDOW, 32, PropModeReplace,
		      NULL, 0);

      XChangeProperty(wm->xdpy, root_win,
		      wm->atoms[MBWM_ATOM_MB_APP_WINDOW_LIST_STACKING],
		      XA_WINDOW, 32, PropModeReplace,
		      NULL, 0);

      XChangeProperty(wm->xdpy, root_win,
		      wm->atoms[MBWM_ATOM_NET_CLIENT_LIST] ,
		      XA_WINDOW, 32, PropModeReplace,
		      NULL, 0);
    }
}

void
mb_wm_manage_client (MBWindowManager       *wm,
		     MBWindowManagerClient *client,
		     Bool                   activate)
{
  /* Add to our list of managed clients */

  if (client == NULL)
    return;

  wm->clients = mb_wm_util_list_append(wm->clients, (void*)client);

  /* add to stack and move to position in stack */

  mb_wm_stack_append_top (client);
  mb_wm_client_stack(client, 0);
  mb_wm_update_root_win_lists (wm);

  if (MB_WM_IS_CLIENT_PANEL(client))
    mb_wm_update_root_win_rectangles (wm);
  else if (MB_WM_IS_CLIENT_DESKTOP(client))
    wm->desktop = client;

  /* set flags to map - should this go elsewhere? */

  if (activate)
    mb_wm_activate_client (wm, client);
  else
    mb_wm_client_show (client);

  mb_wm_display_sync_queue (client->wmref,
			    MBWMSyncStacking | MBWMSyncVisibility);
}

void
mb_wm_unmanage_client (MBWindowManager       *wm,
		       MBWindowManagerClient *client,
		       Bool                   destroy)
{
  /* FIXME: set a managed flag in client object ? */
  MBWindowManagerClient *c;

  wm->clients = mb_wm_util_list_remove(wm->clients, (void*)client);

  mb_wm_stack_remove (client);
  mb_wm_update_root_win_lists (wm);

  if (MB_WM_IS_CLIENT_PANEL(client))
    mb_wm_update_root_win_rectangles (wm);

  if (wm->focused_client == client)
    mb_wm_unfocus_client (wm, client);

  /*
   * Must remove client from any transient list, otherwise when we call
   * _stack_enumerate() everything will go pear shape
   */
  mb_wm_client_detransitise (client);

  mb_wm_stack_enumerate (wm, c)
    {
      if (c->next_focused_client == client)
	c->next_focused_client = client->next_focused_client;

      /* FIXME -- handle transients ? */
    }

  if (client == wm->desktop)
    wm->desktop = NULL;

  if (destroy)
    mb_wm_object_unref (MB_WM_OBJECT(client));
  else
    wm->unmapped_clients = mb_wm_util_list_append (wm->unmapped_clients,
						  client);

  mb_wm_display_sync_queue (client->wmref, MBWMSyncStacking);
}

MBWindowManagerClient*
mb_wm_managed_client_from_xwindow(MBWindowManager *wm, Window win)
{
  MBWindowManagerClient *client = NULL;
  MBWMList *l;

  if (win == wm->root_win->xwindow)
    return NULL;

  l = wm->clients;
  while (l)
    {
      client = l->data;

      if (client->window && client->window->xwindow == win)
	return client;

      l = l->next;
    }

  return NULL;
}

MBWindowManagerClient*
mb_wm_unmapped_client_from_xwindow(MBWindowManager *wm, Window win)
{
  MBWindowManagerClient *client = NULL;
  MBWMList *l;

  if (win == wm->root_win->xwindow)
    return NULL;

  l = wm->unmapped_clients;
  while (l)
    {
      client = l->data;

      if (client->window && client->window->xwindow == win)
	return client;

      l = l->next;
    }

  return NULL;
}

void
mb_wm_main_loop(MBWindowManager *wm)
{
  mb_wm_main_context_loop (wm->main_ctx);
}

void
mb_wm_get_display_geometry (MBWindowManager  *wm,
			    MBGeometry       *geometry)
{
  geometry->x = 0;
  geometry->y = 0;
  geometry->width  = wm->xdpy_width;
  geometry->height = wm->xdpy_height;
}

void
mb_wm_display_sync_queue (MBWindowManager* wm, MBWMSyncType sync)
{
  wm->sync_type |= sync;
}

static void
mb_wm_manage_preexistsing_wins (MBWindowManager* wm)
{
   unsigned int      nwins, i;
   Window            foowin1, foowin2, *wins;
   XWindowAttributes attr;
   MBWindowManagerClass * wm_class =
     MB_WINDOW_MANAGER_CLASS (MB_WM_OBJECT_GET_CLASS (wm));

   if (!wm_class->client_new)
     return;

   XQueryTree(wm->xdpy, wm->root_win->xwindow,
	      &foowin1, &foowin2, &wins, &nwins);

   for (i = 0; i < nwins; i++)
     {
       XGetWindowAttributes(wm->xdpy, wins[i], &attr);

       if (!attr.override_redirect && attr.map_state == IsViewable)
	 {
	   MBWMClientWindow      *win = NULL;
	   MBWindowManagerClient *client = NULL;

	   win = mb_wm_client_window_new (wm, wins[i]);

	   if (!win)
	     continue;

	   client = wm_class->client_new (wm, win);

	   if (client)
	     mb_wm_manage_client(wm, client, False);
	   else
	     mb_wm_object_unref (MB_WM_OBJECT (win));
	 }
     }
   XFree(wins);
}

static void
mb_wm_get_desktop_geometry (MBWindowManager *wm, MBGeometry * geom)
{
  MBWindowManagerClient *c;
  int     x, y, width, height, result = 0;
  MBGeometry p_geom;
  MBWMClientLayoutHints hints;

  geom->x      = 0;
  geom->y      = 0;
  geom->width  = wm->xdpy_width;
  geom->height = wm->xdpy_height;

  if (mb_wm_stack_empty(wm))
    return;

  mb_wm_stack_enumerate(wm, c)
     {
       if (!mb_wm_client_is_mapped (c) || !MB_WM_IS_CLIENT_PANEL (c))
	 continue;

       mb_wm_client_get_coverage (c, & p_geom);

       hints = mb_wm_client_get_layout_hints (c);

       if (LayoutPrefReserveEdgeNorth & hints)
	 geom->x += p_geom.height;

       if (LayoutPrefReserveEdgeSouth & hints)
	 geom->height -= p_geom.height;

       if (LayoutPrefReserveEdgeWest & hints)
	 geom->y += p_geom.width;

       if (LayoutPrefReserveEdgeEast & hints)
	 geom->width -= p_geom.width;
     }
}

static void
mb_wm_update_root_win_rectangles (MBWindowManager *wm)
{
  Display * dpy = wm->xdpy;
  Window    root = wm->root_win->xwindow;
  MBGeometry d_geom;
  CARD32 val[4];

  mb_wm_get_desktop_geometry (wm, &d_geom);

  val[0] = d_geom.x;
  val[1] = d_geom.y;
  val[2] = d_geom.width;
  val[3] = d_geom.height;

  /* FIXME -- handle decorated desktops */

  XChangeProperty(dpy, root, wm->atoms[MBWM_ATOM_NET_WORKAREA],
		  XA_CARDINAL, 32, PropModeReplace,
		  (unsigned char *)val, 4);

  XChangeProperty(dpy, root, wm->atoms[MBWM_ATOM_NET_DESKTOP_GEOMETRY],
		  XA_CARDINAL, 32, PropModeReplace,
		  (unsigned char *)&val[2], 2);

}

int
mb_wm_register_client_type (void)
{
  static int type_cnt = 0;
  return ++type_cnt;
}

static int
mb_wm_init (MBWMObject *this, va_list vap)
{
  MBWindowManager      *wm = MB_WINDOW_MANAGER (this);
  MBWindowManagerClass *wm_class;
  MBWMObjectProp        prop;
  int                   argc = 0;
  char                **argv = NULL;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropArgc:
	  argc = va_arg(vap, int);
	  break;
	case MBWMObjectPropArgv:
	  argv = va_arg(vap, char **);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  wm_class = (MBWindowManagerClass *) MB_WM_OBJECT_GET_CLASS (wm);

  if (argc && argv && wm_class->process_cmdline)
    wm_class->process_cmdline (wm, argc, argv);

  if (!wm->xdpy)
    wm->xdpy = XOpenDisplay(getenv("DISPLAY"));

  if (!wm->xdpy)
    {
      /* FIXME: Error codes */
      mb_wm_util_fatal_error("Display connection failed");
      return 0;
    }

  if (getenv("MB_SYNC"))
    XSynchronize (wm->xdpy, True);

  mb_wm_debug_init (getenv("MB_DEBUG"));

  /* FIXME: Multiple screen handling */

  wm->xscreen     = DefaultScreen(wm->xdpy);
  wm->xdpy_width  = DisplayWidth(wm->xdpy, wm->xscreen);
  wm->xdpy_height = DisplayHeight(wm->xdpy, wm->xscreen);

  wm->xas_context = xas_context_new(wm->xdpy);

  mb_wm_atoms_init(wm);

  mb_wm_set_theme_from_path (wm, NULL);

  wm->root_win = mb_wm_root_window_get (wm);

  mb_wm_update_root_win_rectangles (wm);

  wm->main_ctx = mb_wm_main_context_new (wm);
  mb_wm_main_context_x_event_handler_add (wm->main_ctx,
			     None,
			     MapRequest,
			     (MBWMXEventFunc)mb_wm_handle_map_request,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx,
			     None,
			     ConfigureRequest,
			     (MBWMXEventFunc)mb_wm_handle_config_request,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx,
  			     None,
			     PropertyNotify,
			     (MBWMXEventFunc)mb_wm_handle_property_notify,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx,
			     None,
			     DestroyNotify,
			     (MBWMXEventFunc)test_destroy_notify,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx,
			     None,
			     UnmapNotify,
			     (MBWMXEventFunc)test_unmap_notify,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx,
			     None,
			     KeyPress,
			     (MBWMXEventFunc)test_key_press,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx,
			     None,
			     ButtonPress,
			     (MBWMXEventFunc)test_button_press,
			     wm);

  mb_wm_keys_init(wm);

  /* mb_wm_decor_init (wm); */

  base_foo ();

  mb_wm_manage_preexistsing_wins (wm);

  return 1;
}

static void
mb_wm_process_cmdline (MBWindowManager *wm, int argc, char **argv)
{
  int i;

  for (i = 0; i < argc; ++i)
    {
      /* These need to have a value after the name parameter */
      if (i < argc - 1)
	{
	  if (!strcmp(argv[i], "-display"))
	    {
	      wm->xdpy = XOpenDisplay(argv[++i]);
	    }
	  else if (!strcmp ("-sm-client-id", argv[i]))
	    {
	      wm->sm_client_id = argv[++i];
	    }
	}
    }
}

void
mb_wm_activate_client(MBWindowManager * wm, MBWindowManagerClient *c)
{
  MBWindowManagerClass  *wm_klass;
  Bool was_desktop;
  Bool is_desktop;

  if (c == NULL)
    return;

  wm_klass = MB_WINDOW_MANAGER_CLASS (MB_WM_OBJECT_GET_CLASS (wm));

  if (wm_klass->client_activate && wm_klass->client_activate (wm, c))
    return; /* Handled by derived class */

  was_desktop = (wm->flags & MBWindowManagerFlagDesktop);
  is_desktop  = (MB_WM_IS_CLIENT_DESKTOP(c));

  if (is_desktop)
    wm->flags |= MBWindowManagerFlagDesktop;
  else
    wm->flags &= ~MBWindowManagerFlagDesktop;

  mb_wm_client_show (c);

  mb_wm_focus_client (wm, c);

  if (is_desktop != was_desktop)
    {
      CARD32 card = is_desktop ? 1 : 0;

      XChangeProperty(wm->xdpy, wm->root_win->xwindow,
		      wm->atoms[MBWM_ATOM_NET_SHOWING_DESKTOP],
		      XA_CARDINAL, 32, PropModeReplace,
		      (unsigned char *)&card, 1);
    }

  mb_wm_display_sync_queue (wm, MBWMSyncStacking | MBWMSyncVisibility);
}

MBWindowManagerClient*
mb_wm_get_visible_main_client(MBWindowManager *wm)
{
  if ((wm->flags & MBWindowManagerFlagDesktop) && wm->desktop)
    return wm->desktop;

  return mb_wm_stack_get_highest_by_type (wm, MBWMClientTypeApp);
}

void
mb_wm_handle_ping_reply (MBWindowManager * wm, MBWindowManagerClient *c)
{
  if (c == NULL)
    return;

  if (mb_wm_client_ping_in_progress (c))
    {
      MBWindowManagerClass  *wm_klass;

      mb_wm_client_ping_stop (c);

      wm_klass = MB_WINDOW_MANAGER_CLASS (MB_WM_OBJECT_GET_CLASS (wm));

      if (wm_klass->client_responding)
	wm_klass->client_responding (wm, c);
    }
}

void
mb_wm_handle_hang_client (MBWindowManager * wm, MBWindowManagerClient *c)
{
  MBWindowManagerClass  *wm_klass;

  if (c == NULL)
    return;

  wm_klass = MB_WINDOW_MANAGER_CLASS (MB_WM_OBJECT_GET_CLASS (wm));

  if (!wm_klass->client_hang || !wm_klass->client_hang (wm, c))
    {
      mb_wm_client_shutdown (c);
    }
}

void
mb_wm_toggle_desktop (MBWindowManager * wm)
{
  Bool show = !(wm->flags & MBWindowManagerFlagDesktop);
  mb_wm_handle_show_desktop (wm, show);
}

void
mb_wm_handle_show_desktop (MBWindowManager * wm, Bool show)
{
  if (!wm->desktop)
    return;

  if (show)
    mb_wm_activate_client (wm, wm->desktop);
  else
    {
      MBWindowManagerClient * c =
	mb_wm_stack_get_highest_by_type (wm, MBWMClientTypeApp);

      if (c)
	mb_wm_activate_client (wm, c);
    }
}

void
mb_wm_set_layout (MBWindowManager *wm, MBWMLayout *layout)
{
  wm->layout = layout;
  wm->sync_type = MBWMSyncGeometry | MBWMSyncVisibility;
}

static void
mb_wm_focus_client (MBWindowManager *wm, MBWindowManagerClient *client)
{
  if (!mb_wm_client_want_focus (client))
    return;

  if (!mb_wm_client_is_realized (client))
    {
      /* We need the window mapped before we can focus it, but do not
       * want to do a full-scale mb_wm_sync ():
       *
       * First We need to update layout, othewise the window will get frame
       * size of 0x0; then we can realize it, and do a display sync (i.e.,
       * map).
       */
      if (wm->layout)
	mb_wm_layout_update (wm->layout);

      mb_wm_client_realize (client);
      mb_wm_client_display_sync (client);
    }

  if (mb_wm_client_focus (client))
    {
      if (wm->focused_client)
	{
	  MBWindowManagerClient *trans_old = wm->focused_client;
	  MBWindowManagerClient *trans_new = client;

	  while (trans_old->transient_for)
	    trans_old = trans_old->transient_for;

	  while (trans_new->transient_for)
	    trans_new = trans_new->transient_for;

	  client->next_focused_client = NULL;

	  /*
	   * Are we both transient for the same thing ?
	   */
	  if (trans_new && trans_new == trans_old)
	    client->next_focused_client = wm->focused_client;

	  /* From regular dialog to transient for root dialogs */
	  if (MB_WM_IS_CLIENT_DIALOG (client) &&
	      !client->transient_for &&
	      MB_WM_IS_CLIENT_DIALOG (wm->focused_client))
	    client->next_focused_client = wm->focused_client;
	}

      wm->focused_client = client;
    }
}

void
mb_wm_unfocus_client (MBWindowManager *wm, MBWindowManagerClient *client)
{
  MBWindowManagerClient *next = NULL;

  if (client != wm->focused_client)
    return;

  if (client->next_focused_client)
    next = client->next_focused_client;
  else if (wm->stack_top)
    {
      MBWindowManagerClient *c;

      mb_wm_stack_enumerate_reverse (wm, c)
	{
	  if (c != client && mb_wm_client_want_focus (c))
	    {
	      next = c;
	      break;
	    }
	}
    }

  if (next)
    mb_wm_activate_client (wm, next);
}

void
mb_wm_cycle_apps (MBWindowManager *wm)
{
  MBWindowManagerClient *old_top, *new_top;

  if (wm->flags & MBWindowManagerFlagDesktop)
    {
      mb_wm_handle_show_desktop (wm, False);
      return;
    }

  old_top = mb_wm_stack_get_highest_by_type (wm, MBWMClientTypeApp);

  new_top = mb_wm_stack_cycle_by_type(wm, MBWMClientTypeApp);

  if (old_top != new_top)
    {
      mb_wm_activate_client (wm, new_top);
    }
}

void
mb_wm_set_theme (MBWindowManager *wm, MBWMTheme * theme)
{
  if (!theme)
    return;

  XGrabServer(wm->xdpy);

  if (wm->theme)
    mb_wm_object_unref (MB_WM_OBJECT (wm->theme));

  wm->theme = theme;
  wm->sync_type |= (MBWMSyncGeometry | MBWMSyncVisibility | MBWMSyncDecor);

  /* When initializing the MBWindowManager object, the theme gets created
   * before the root window, so that the root window can interogate it,
   * so we can get here before the window is in place
   */
  if (wm->root_win)
    mb_wm_root_window_update_supported_props (wm->root_win);

  mb_wm_object_signal_emit (MB_WM_OBJECT (wm),
			    MBWindowManagerSignalThemeChange);


  XUngrabServer(wm->xdpy);
}

void
mb_wm_set_theme_from_path (MBWindowManager *wm, const char *theme_path)
{
  MBWMTheme            *theme;
  MBWindowManagerClass *wm_class;

  wm_class = MB_WINDOW_MANAGER_CLASS (MB_WM_OBJECT_GET_CLASS (wm));

  if (wm->theme)
    {
      if (wm->theme->path && theme_path &&
	  !strcmp (theme_path, wm->theme->path))
	return;

      if (!wm->theme->path && !theme_path)
	return;
    }

  theme = wm_class->theme_new (wm, theme_path);

  mb_wm_set_theme (wm, theme);
}

