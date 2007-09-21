#include "mb-wm.h"
#include "../client-types/mb-wm-client-app.h"
#include "../client-types/mb-wm-client-panel.h"
#include "../client-types/mb-wm-client-dialog.h"
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
  else
    {
      return mb_wm_client_app_new(wm, win);
    }
}

static MBWMTheme *
mb_wm_theme_init (MBWindowManager * wm)
{
  return mb_wm_theme_new (wm);
}

static void
mb_wm_class_init (MBWMObjectClass *klass)
{
  MBWindowManagerClass *wm_class;

  MBWM_MARK();

  wm_class = (MBWindowManagerClass *)klass;

  wm_class->process_cmdline = mb_wm_process_cmdline;
  wm_class->client_new      = mb_wm_client_new_func;
  wm_class->theme_init      = mb_wm_theme_init;
}

static void
mb_wm_destroy (MBWMObject *this)
{
  MBWindowManager * wm = MB_WINDOW_MANAGER (this);
  MBWMList *l = wm->clients;

  while (l)
    {
      mb_wm_object_unref (l->data);

      l = l->next;
    }

  mb_wm_object_unref (MB_WM_OBJECT (wm->root_win));
  mb_wm_object_unref (MB_WM_OBJECT (wm->theme));
  mb_wm_object_unref (MB_WM_OBJECT (wm->layout));
  mb_wm_object_unref (MB_WM_OBJECT (wm->main_ctx));
}

static void
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
      mb_wm_unmanage_client (wm, client);
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
	  mb_wm_unmanage_client (wm, client);
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

  client = mb_wm_managed_client_from_xwindow(wm, xev->window);

  if (!client)
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

      if (client)
	mb_wm_manage_client(wm, client);
      else
	mb_wm_object_unref (MB_WM_OBJECT (win));
    }

  return True;
}


static Window*
stack_get_window_list (MBWindowManager *wm)
{
  Window                *win_list;
  MBWindowManagerClient *client;
  int                    i = 0;

  if (!wm->stack_n_clients) return NULL;

  /* FIXME: avoid mallocing all the time - cache the memory */
  win_list = malloc(sizeof(Window)*(wm->stack_n_clients));

  MBWM_DBG("start");

  mb_wm_stack_enumerate_reverse(wm, client)
  {
    if (client->xwin_frame)
      win_list[i++] = client->xwin_frame;
    else
      win_list[i++] = MB_WM_CLIENT_XWIN(client);

    MBWM_DBG("added %lx", win_list[i-1]);
  }

  MBWM_DBG("end");

  return win_list;
}

static void
stack_sync_to_display (MBWindowManager *wm)
{
  Window *win_list = NULL;

  mb_wm_stack_ensure (wm);

  win_list = stack_get_window_list(wm);

  if (win_list)
    {
      mb_wm_util_trap_x_errors();
      XRestackWindows(wm->xdpy, win_list, wm->stack_n_clients);
      mb_wm_util_untrap_x_errors();

      free(win_list);
    }
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

  /* Create the actual window */
  mb_wm_stack_enumerate(wm, client)
    if (!mb_wm_client_is_realized (client))
      mb_wm_client_realize (client);

  /* FIXME: optimise wm sync flags so know if this needs calling */
  /* FIXME: Can we restack an unmapped window ? - problem of new
   *        clients mapping below existing ones.
  */
  stack_sync_to_display (wm);

  /* Now do updates per individual client - maps, paints etc, main work here */
  mb_wm_stack_enumerate(wm, client)
    if (mb_wm_client_needs_sync (client))
      mb_wm_client_display_sync (client);

  /* FIXME: New clients now managed will likely need some propertys
   *        synced up here.
  */

  XUngrabServer(wm->xdpy);

  wm->need_display_sync = False;
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
      int                    stack_size = mb_wm_stack_size (wm);
      MBWindowManagerClient *c;
      MBWMList              *l;

      wins     = mb_wm_util_malloc0 (sizeof(Window) * stack_size);
      app_wins = mb_wm_util_malloc0 (sizeof(Window) * stack_size);

      mb_wm_stack_enumerate (wm,c)
	{
	  wins[cnt++] = c->window->xwindow;
	  if (MB_WM_IS_CLIENT_APP (c))
	    app_wins[app_win_cnt++] = c->window->xwindow;
	}

      XChangeProperty(wm->xdpy, root_win,
		      wm->atoms[MBWM_ATOM_NET_CLIENT_LIST_STACKING],
		      XA_WINDOW, 32, PropModeReplace,
		      (unsigned char *)wins, stack_size);

      XChangeProperty(wm->xdpy, root_win,
		      wm->atoms[MBWM_ATOM_MB_APP_WINDOW_LIST_STACKING],
		      XA_WINDOW, 32, PropModeReplace,
		      (unsigned char *)app_wins, app_win_cnt);

      free(app_wins);

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
		      (unsigned char *)wins, stack_size);

      free(wins);
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

      XChangeProperty(wm->xdpy, root_win, wm
		      ->atoms[MBWM_ATOM_NET_CLIENT_LIST] ,
		      XA_WINDOW, 32, PropModeReplace,
		      NULL, 0);
    }
}

void
mb_wm_manage_client (MBWindowManager       *wm,
		     MBWindowManagerClient *client)
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

  /* set flags to map - should this go elsewhere? */

  mb_wm_activate_client (wm, client);
}

void
mb_wm_unmanage_client (MBWindowManager       *wm,
		       MBWindowManagerClient *client)
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

  mb_wm_stack_enumerate (wm, c)
    {
      if (c->next_focused_client == client)
	c->next_focused_client = client->next_focused_client;

      /* FIXME -- handle transients ? */
    }

  mb_wm_object_unref (MB_WM_OBJECT(client));
}

MBWindowManagerClient*
mb_wm_managed_client_from_xwindow(MBWindowManager *wm, Window win)
{
  MBWindowManagerClient *client = NULL;

  if (win == wm->root_win->xwindow)
    return NULL;

  mb_wm_stack_enumerate(wm, client)
    if (client->window && client->window->xwindow == win)
	return client;

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
mb_wm_display_sync_queue (MBWindowManager* wm)
{
  wm->need_display_sync = True;
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
	     mb_wm_manage_client(wm, client);
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

static void
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
      return;
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

  wm->theme = wm_class->theme_init (wm);

  wm->root_win = mb_wm_root_window_get (wm);

  mb_wm_update_root_win_rectangles (wm);

  wm->main_ctx = mb_wm_main_context_new (wm);
  mb_wm_main_context_x_event_handler_add (wm->main_ctx, MapRequest,
			     (MBWMXEventFunc)mb_wm_handle_map_request,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx, ConfigureRequest,
			     (MBWMXEventFunc)mb_wm_handle_config_request,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx, PropertyNotify,
			     (MBWMXEventFunc)mb_wm_handle_property_notify,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx, DestroyNotify,
			     (MBWMXEventFunc)test_destroy_notify,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx, UnmapNotify,
			     (MBWMXEventFunc)test_unmap_notify,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx, KeyPress,
			     (MBWMXEventFunc)test_key_press,
			     wm);

  mb_wm_main_context_x_event_handler_add (wm->main_ctx, ButtonPress,
			     (MBWMXEventFunc)test_button_press,
			     wm);

  mb_wm_keys_init(wm);

  /* mb_wm_decor_init (wm); */

  base_foo ();

  mb_wm_manage_preexistsing_wins (wm);
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

  if (c == NULL)
    return;

  wm_klass = MB_WINDOW_MANAGER_CLASS (MB_WM_OBJECT_GET_CLASS (wm));

  if (wm_klass->client_activate && wm_klass->client_activate (wm, c))
    return; /* Handled by derived class */

  XGrabServer(wm->xdpy);

  mb_wm_client_show (c);
  mb_wm_focus_client (wm, c);

  /* FIXME -- might need some magic here ...*/

  XSync(wm->xdpy, False);
  XUngrabServer(wm->xdpy);
}

MBWindowManagerClient*
mb_wm_get_visible_main_client(MBWindowManager *wm)
{
#if 0
  if (w->flags & DESKTOP_RAISED_FLAG)
    {
      return wm->desktop;
    }
#endif

  if (wm->stack_top_app)
    {
      return wm->stack_top_app;
    }

  return NULL;
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
mb_wm_handle_show_desktop (MBWindowManager * wm, Bool show)
{
  MBWindowManagerClass  *wm_klass =
    MB_WINDOW_MANAGER_CLASS (MB_WM_OBJECT_GET_CLASS (wm));

  if (wm_klass->show_desktop)
      wm_klass->show_desktop (wm, show);
}

void
mb_wm_set_layout (MBWindowManager *wm, MBWMLayout *layout)
{
  wm->layout = layout;
  wm->need_display_sync = True;
}

static void
mb_wm_focus_client (MBWindowManager *wm, MBWindowManagerClient *client)
{
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
  if (client != wm->focused_client)
    return;

  if (client->next_focused_client)
    mb_wm_activate_client (wm, client->next_focused_client);
  else if (wm->stack_top)
    mb_wm_activate_client (wm, wm->stack_top);
}

