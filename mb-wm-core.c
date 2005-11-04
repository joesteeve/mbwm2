#include "mb-wm.h"

#ifdef MBWM_WANT_DEBUG

static const char *MBWMDEBUGEvents[] = {
    "error",
    "reply",
    "KeyPress",
    "KeyRelease",
    "ButtonPress",
    "ButtonRelease",
    "MotionNotify",
    "EnterNotify",
    "LeaveNotify",
    "FocusIn",
    "FocusOut",
    "KeymapNotify",
    "Expose",
    "GraphicsExpose",
    "NoExpose",
    "VisibilityNotify",
    "CreateNotify",
    "DestroyNotify",
    "UnmapNotify",
    "MapNotify",
    "MapRequest",
    "ReparentNotify",
    "ConfigureNotify",
    "ConfigureRequest",
    "GravityNotify",
    "ResizeRequest",
    "CirculateNotify",
    "CirculateRequest",
    "PropertyNotify",
    "SelectionClear",
    "SelectionRequest",
    "SelectionNotify",
    "ColormapNotify",
    "ClientMessage",
    "MappingNotify",
};

#endif

void
test_destroy_notify (MBWindowManager      *wm,
		     XDestroyWindowEvent  *xev,
		     void                 *userdata)
{
  MBWindowManagerClient *client = NULL;

  MBWM_MARK();

  client = mb_wm_core_managed_client_from_xwindow(wm, xev->window);

  if (client)
    {
      mb_wm_core_unmanage_client (wm, client);
      mb_wm_client_destroy (client);
    }


}

void 
test_map_request (MBWindowManager   *wm,
		  XMapRequestEvent  *xev,
		  void              *userdata)
{
  MBWindowManagerClient *client = NULL;

  MBWM_MARK();

  client = mb_wm_core_managed_client_from_xwindow(wm, xev->window);

  if (!client)
    {
      MBWindowManagerClientWindow *win = NULL; 

      if (wm->new_client_from_window_func == NULL)
	{
	  MBWM_DBG("### No new client hook exists ###");
	  return;
	}

      win = mb_wm_client_window_new (wm, xev->window);

      if (!win)
	return; 

      client = wm->new_client_from_window_func(wm, win); 
      
      if (client)
	{
	  mb_wm_client_realize(client); 
	  mb_wm_core_manage_client(wm, client);
	}
    }
}


static Window*
stack_get_window_list (MBWindowManager *wm)
{
  Window                *win_list;
  MBWindowManagerClient *client;
  int                    i = 0;

  if (!wm->stack_n_clients) return NULL;

  /* FIXME: could avoid this malloc with a static-ish arr */
  win_list = malloc(sizeof(Window)*(wm->stack_n_clients));

  mb_wm_stack_enumerate_reverse(wm, client)
  {
    win_list[i++] = client->xwin_frame;
  }

  return win_list;
}


static void
stack_sync_to_display (MBWindowManager *wm)
{
  Window *win_list = NULL;

  win_list = stack_get_window_list(wm);
  
  if (win_list)
    {
      mb_wm_util_trap_x_errors();

      XRestackWindows(wm->xdpy, win_list, wm->stack_n_clients);

      free(win_list);

      mb_wm_util_untrap_x_errors();
    }
}


void
mb_wm_core_sync (MBWindowManager *wm)
{
  /* This would sync all pending geometry and stcking changing to 
   * this display.
  */
  MBWindowManagerClient *client = NULL;

  MBWM_MARK();

  /* FIXME: optimise wm sync flags so know if this needs calling */
  stack_sync_to_display (wm);

  mb_wm_layout_manager_update (wm); 

  mb_wm_stack_enumerate(wm, client)
    if (mb_wm_client_needs_sync (client))
      mb_wm_client_display_sync (client);

  wm->need_display_sync = False;
}

void
mb_wm_core_manage_client (MBWindowManager       *wm,
			  MBWindowManagerClient *client)
{
  /* FIXME: set a managed flag in client object ? */

  mb_wm_stack_append_top (client);

  mb_wm_client_stack(client, 0); /* move to top of stack */

  mb_wm_client_show(client); /* set flags to map - should this go elsewhere? */

  /* Call show() and activate here or whatver :/ */
}

void
mb_wm_core_unmanage_client (MBWindowManager       *wm,
			    MBWindowManagerClient *client)
{
  /* FIXME: set a managed flag in client object ? */

  mb_wm_stack_remove (client);

  /* Call show() and activate here or whatver :/ */
}

MBWindowManagerClient*
mb_wm_core_managed_client_from_xwindow(MBWindowManager *wm, Window win)
{
  MBWindowManagerClient *client = NULL;

  mb_wm_stack_enumerate(wm, client)
    if (client->window && client->window->xwindow == win)
      return client;

  return NULL;
}

void
mb_wm_run(MBWindowManager *wm)
{
  MBWindowManagerEventFuncs *xev_funcs = wm->event_funcs;

  MBWM_DBG("entering event loop");

  /*  change to while((evt = XCBPollForEvent(conn, 0))) */
  while (TRUE)
    {
      XEvent xev;
      XNextEvent(wm->xdpy, &xev);

      MBWM_DBG("@ XEvent: '%s:%i' for %lx", 
	       MBWMDEBUGEvents[xev.type], 
	       xev.type, 
	       xev.xany.window
	       );

      switch (xev.type)
	{
	case Expose:
	  break;
	case MapRequest:
	  if (xev_funcs->map_request)
	    xev_funcs->map_request(wm, 
				   (XMapRequestEvent*)&xev.xmaprequest, 
				   xev_funcs->user_data);
	  break;
	case MapNotify:
	  if (xev_funcs->map_notify)
	    xev_funcs->map_notify(wm, 
				  (XMapEvent*)&xev.xmap, 
				  xev_funcs->user_data);
	  break;
	case DestroyNotify:
	  if (xev_funcs->destroy_notify)
	    xev_funcs->destroy_notify(wm, 
				      (XDestroyWindowEvent*)&xev.xdestroywindow, 
				      xev_funcs->user_data);
	  break;
	case ConfigureNotify:
	  if (xev_funcs->configure_notify)
	    xev_funcs->configure_notify(wm, 
				      (XConfigureEvent*)&xev.xconfigure, 
				      xev_funcs->user_data);
	  break;
	case ConfigureRequest:
	  if (xev_funcs->configure_request)
	    xev_funcs->configure_request(wm, 
					 (XConfigureRequestEvent*)&xev.xconfigurerequest, 
					 xev_funcs->user_data);

	  break;

	default:


	  break;
	}

      if (wm->need_display_sync)
	mb_wm_core_sync (wm);

    }
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

int
mb_wm_register_client_type (MBWindowManager* wm)
{
  return wm->client_type_cnt++;
}

MBWindowManager*
mb_wm_new(void)
{
  return mb_wm_util_malloc0(sizeof(MBWindowManager));
}

Status
mb_wm_init(MBWindowManager *wm, int *argc, char ***argv)
{
  XSetWindowAttributes sattr;

  wm->xdpy = XOpenDisplay(getenv("DISPLAY"));
  
  if (!wm->xdpy)
    {
      /* FIXME: Error codes */
      mb_wm_util_fatal_error("Display connection failed");
      return False;
    }

  if (getenv("MB_SYNC")) 
    XSynchronize (wm->xdpy, True);
  
  /* FIXME: Multiple screen handling */
  
  wm->xscreen     = DefaultScreen(wm->xdpy);
  wm->xwin_root   = RootWindow(wm->xdpy, wm->xscreen); 
  wm->xdpy_width  = DisplayWidth(wm->xdpy, wm->xscreen);
  wm->xdpy_height = DisplayHeight(wm->xdpy, wm->xscreen);
  
  wm->xas_context = xas_context_new(wm->xdpy);
  
  sattr.event_mask =  SubstructureRedirectMask
                      |SubstructureNotifyMask
                      |StructureNotifyMask
                      |PropertyChangeMask;

  mb_wm_util_trap_x_errors();  

  XChangeWindowAttributes(wm->xdpy, wm->xwin_root, CWEventMask, &sattr);

  XSync(wm->xdpy, False);

  if (mb_wm_util_untrap_x_errors())
    {
      /* FIXME: Error codes */
      mb_wm_util_fatal_error("Unable to manage display - another window manager already active?");
      return False;
    }

  XSelectInput(wm->xdpy, wm->xwin_root, sattr.event_mask);

  wm->new_client_from_window_func = mb_wm_client_new;

  wm->event_funcs = mb_wm_util_malloc0(sizeof(MBWindowManagerEventFuncs));

  wm->event_funcs->map_request = test_map_request;
  wm->event_funcs->destroy_notify = test_destroy_notify;

  mb_wm_atoms_init(wm);

  return True;
}
	   

