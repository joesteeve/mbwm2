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

static void
mb_wm_handle_x_event (MBWindowManager *wm,
		      XEvent          *xev);

void
test_key_press (MBWindowManager *wm,
		XKeyEvent       *xev,
		void            *userdata)
{
  mb_wm_keys_press (wm, 
		    XKeycodeToKeysym(wm->xdpy, xev->keycode, 0),
		    xev->state);
}

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
      MBWM_DBG("Found client, unmanaing!");
      mb_wm_core_unmanage_client (wm, client);
      mb_wm_object_unref (MB_WM_OBJECT(client));
    }
}
static void
mb_wm_core_handle_property_notify (MBWindowManager         *wm,
				   XPropertyEvent          *xev,
				   void                    *userdata)
{
  MBWindowManagerClient *client;

  client = mb_wm_core_managed_client_from_xwindow(wm, xev->window);

  if (!client)
    return;

}

static void 
mb_wm_core_handle_config_request (MBWindowManager        *wm,
				  XConfigureRequestEvent *xev,
				  void                   *userdata)
{
  MBWindowManagerClient *client;
  unsigned long          value_mask;
  int                    req_x, req_y, req_w, req_h;
  MBGeometry             req_geom, *win_geom;

  client = mb_wm_core_managed_client_from_xwindow(wm, xev->window);

  if (!client)
    {
      MBWM_DBG("### No client found for configure event ###");
      return;
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
}

static void 
mb_wm_core_handle_map_request (MBWindowManager   *wm,
			       XMapRequestEvent  *xev,
			       void              *userdata)
{
  MBWindowManagerClient *client = NULL;

  MBWM_MARK();

  client = mb_wm_core_managed_client_from_xwindow(wm, xev->window);

  if (!client)
    {
      MBWMWindow *win = NULL; 

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
	mb_wm_core_manage_client(wm, client);
      else
	{
	  /* Free up window object */
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
  /* Sync all changes to display */
  MBWindowManagerClient *client = NULL;

  MBWM_MARK();

  XGrabServer(wm->xdpy);

  /* Size stuff first assume newly managed windows unmapped ?
   * 
  */
  mb_wm_layout_manager_update (wm); 

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

void
mb_wm_core_manage_client (MBWindowManager       *wm,
			  MBWindowManagerClient *client)
{
  /* Add to our list of managed clients */

  if (client == NULL)
    return;

  wm->clients = mb_wm_util_list_append(wm->clients, (void*)client);

  /* move to position in stack */

  mb_wm_client_stack(client, 0);

  /* set flags to map - should this go elsewhere? */

  mb_wm_client_show(client);   
}

void
mb_wm_core_unmanage_client (MBWindowManager       *wm,
			    MBWindowManagerClient *client)
{
  /* FIXME: set a managed flag in client object ? */

  wm->clients = mb_wm_util_list_remove(wm->clients, (void*)client);

  mb_wm_stack_remove (client);

  /* Call show() and activate here or whatver :/ */
}

MBWindowManagerClient*
mb_wm_core_managed_client_from_xwindow(MBWindowManager *wm, Window win)
{
  MBWindowManagerClient *client = NULL;

  if (win == wm->xwin_root)
    return NULL;

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

  while (TRUE)
    {
      XEvent xev;
      XNextEvent(wm->xdpy, &xev);

#if (MBWM_WANT_DEBUG)
 {
   MBWindowManagerClient *ev_client;
   
   ev_client = mb_wm_core_managed_client_from_xwindow(wm, xev.xany.window);
   
   MBWM_DBG("@ XEvent: '%s:%i' for %lx %s%s", 
	    MBWMDEBUGEvents[xev.type], 
	    xev.type, 
	    xev.xany.window,
	    xev.xany.window == wm->xwin_root ? "(root)" : "",
	    ev_client ? ev_client->name : ""
	    );
 }
#endif

      mb_wm_handle_x_event (wm, &xev);
 
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

void
mb_wm_manage_preexistsing_wins (MBWindowManager* wm)
{
   unsigned int      nwins, i;
   Window            foowin1, foowin2, *wins;
   XWindowAttributes attr;

   if (!wm->new_client_from_window_func)
     return;

   XQueryTree(wm->xdpy, wm->xwin_root, 
	      &foowin1, &foowin2, &wins, &nwins);

   for (i = 0; i < nwins; i++) 
     {
       XGetWindowAttributes(wm->xdpy, wins[i], &attr);

       if (!attr.override_redirect && attr.map_state == IsViewable)
	 {
	   MBWMWindow            *win = NULL; 
	   MBWindowManagerClient *client = NULL;

	   win = mb_wm_client_window_new (wm, wins[i]);

	   if (!win)
	     continue; 

	   client = wm->new_client_from_window_func(wm, win); 
      
	   if (client)
	     mb_wm_core_manage_client(wm, client);
	 }
     }
   XFree(wins);
}

int
mb_wm_register_client_type (void)
{
  static int type_cnt = 0;
  return ++type_cnt;
}

MBWindowManager*
mb_wm_new(void)
{
  return mb_wm_util_malloc0(sizeof(MBWindowManager));
}

void
mb_wm_x_event_handler_set_userdata (MBWindowManager *wm,
				    void            *userdata)
{
  wm->event_funcs->userdata = userdata;
}


void
mb_wm_add_x_event_handler (MBWindowManager *wm,
			   int              type,
			   MBWMXEventFunc   func)
{
  switch (type)
    {
    case Expose:
      break;
    case MapRequest:
      wm->event_funcs->map_request = 
	mb_wm_util_list_append (wm->event_funcs->map_request, 
				(MBWindowManagerMapRequestFunc)func);
      break;
    case MapNotify:
      wm->event_funcs->map_notify= 
	mb_wm_util_list_append (wm->event_funcs->map_notify, 
				(MBWindowManagerMapNotifyFunc)func);
      break;
    case DestroyNotify:
      wm->event_funcs->destroy_notify = 
	mb_wm_util_list_append (wm->event_funcs->destroy_notify, 
				(MBWindowManagerDestroyNotifyFunc)func);
      break;
    case ConfigureNotify:
      wm->event_funcs->configure_notify = 
	mb_wm_util_list_append (wm->event_funcs->configure_notify, 
				(MBWindowManagerConfigureNotifyFunc)func);
      break;
    case ConfigureRequest:
      wm->event_funcs->configure_request = 
	mb_wm_util_list_append (wm->event_funcs->configure_request, 
				(MBWindowManagerConfigureRequestFunc)func);
      break;
    case KeyPress:
      wm->event_funcs->key_press =
	mb_wm_util_list_append (wm->event_funcs->key_press, 
				(MBWindowManagerKeyPressFunc)func);
      break;
    case PropertyNotify:
      wm->event_funcs->property_notify = 
	mb_wm_util_list_append (wm->event_funcs->property_notify, 
				(MBWindowManagerPropertyNotifyFunc)func);
      break;
    case ButtonPress:
      wm->event_funcs->button_press = 
	mb_wm_util_list_append (wm->event_funcs->button_press, 
				(MBWindowManagerButtonPressFunc)func);
      break;
    default:
      break;
    }
}

void
mb_wm_remove_x_event_handler (MBWindowManager *wm,
			      int              type,
			      MBWMXEventFunc   func)
{
  switch (type)
    {
    case Expose:
      break;
    case MapRequest:
      wm->event_funcs->map_request = 
	mb_wm_util_list_remove (wm->event_funcs->map_request, 
				(MBWindowManagerMapRequestFunc)func);
      break;
    case MapNotify:
      wm->event_funcs->map_notify= 
	mb_wm_util_list_remove (wm->event_funcs->map_notify, 
				(MBWindowManagerMapNotifyFunc)func);
      break;
    case DestroyNotify:
      wm->event_funcs->destroy_notify = 
	mb_wm_util_list_remove (wm->event_funcs->destroy_notify, 
				(MBWindowManagerDestroyNotifyFunc)func);
      break;
    case ConfigureNotify:
      wm->event_funcs->configure_notify = 
	mb_wm_util_list_remove (wm->event_funcs->configure_notify, 
				(MBWindowManagerConfigureNotifyFunc)func);
      break;
    case ConfigureRequest:
      wm->event_funcs->configure_request = 
	mb_wm_util_list_remove (wm->event_funcs->configure_request, 
				(MBWindowManagerConfigureRequestFunc)func);
      break;
    case KeyPress:
      wm->event_funcs->key_press =
	mb_wm_util_list_remove (wm->event_funcs->key_press, 
				(MBWindowManagerKeyPressFunc)func);
      break;
    case PropertyNotify:
      wm->event_funcs->property_notify = 
	mb_wm_util_list_remove (wm->event_funcs->property_notify, 
				(MBWindowManagerPropertyNotifyFunc)func);
      break;
    case ButtonPress:
      wm->event_funcs->button_press = 
	mb_wm_util_list_remove (wm->event_funcs->button_press, 
				(MBWindowManagerButtonPressFunc)func);
      break;
    default:
      break;
    }
}

void
mb_wm_add_timeout_handler (MBWindowManager *wm,
			   MBWMXEventFunc   func)
{

}

static void
mb_wm_handle_x_event (MBWindowManager *wm,
		      XEvent          *xev)
{
  MBWindowManagerEventFuncs *xev_funcs = wm->event_funcs;
  MBWMList *iter;

  switch (xev->type)
    {
    case Expose:
      break;
    case MapRequest:
      iter = wm->event_funcs->map_request;

      while (iter) 
	{
	  ((MBWindowManagerMapRequestFunc)(iter->data))
	    (wm, (XMapRequestEvent*)&xev->xmaprequest, xev_funcs->userdata);
	  iter = iter->next;
	}
      break;
    case MapNotify:
      iter = wm->event_funcs->map_notify;

      while (iter) 
	{
	  ((MBWindowManagerMapNotifyFunc)(iter->data))
	    (wm, (XMapEvent*)&xev->xmap, xev_funcs->userdata);
	  iter = iter->next;
	}
      break;
    case DestroyNotify:
      iter = wm->event_funcs->destroy_notify;

      while (iter) 
	{
	  ((MBWindowManagerDestroyNotifyFunc)(iter->data))
	    (wm, (XDestroyWindowEvent*)&xev->xdestroywindow, xev_funcs->userdata);
	  iter = iter->next;
	}
      break;
    case ConfigureNotify:
      iter = wm->event_funcs->configure_notify;

      while (iter) 
	{
	  ((MBWindowManagerConfigureNotifyFunc)(iter->data))
	    (wm, (XConfigureEvent*)&xev->xconfigure, xev_funcs->userdata);
	  iter = iter->next;
	}
      break;
    case ConfigureRequest:
      iter = wm->event_funcs->configure_request;

      while (iter) 
	{
	  ((MBWindowManagerConfigureRequestFunc)(iter->data))
	    (wm, (XConfigureRequestEvent*)&xev->xconfigurerequest, xev_funcs->userdata);
	  iter = iter->next;
	}
      break;
    case KeyPress:
      iter = wm->event_funcs->key_press;

      while (iter) 
	{
	  ((MBWindowManagerKeyPressFunc)(iter->data))
	    (wm, (XKeyEvent*)&xev->xkey, xev_funcs->userdata);
	  iter = iter->next;
	}
      break;
    case PropertyNotify:
      iter = wm->event_funcs->property_notify;

      while (iter) 
	{
	  ((MBWindowManagerPropertyNotifyFunc)(iter->data))
	    (wm, (XPropertyEvent*)&xev->xproperty, xev_funcs->userdata);
	  iter = iter->next;
	}
      break;
    case ButtonPress:
      iter = wm->event_funcs->button_press;

      while (iter) 
	{
	  ((MBWindowManagerButtonPressFunc)(iter->data))
	    (wm, (XButtonEvent*)&xev->xbutton, xev_funcs->userdata);
	  iter = iter->next;
	}
      break;
    }
}

Status
mb_wm_init (MBWindowManager *wm, int *argc, char ***argv)
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

  mb_wm_add_x_event_handler (wm, MapRequest, 
			     (MBWMXEventFunc)mb_wm_core_handle_map_request);

  mb_wm_add_x_event_handler (wm, ConfigureRequest, 
			     (MBWMXEventFunc)mb_wm_core_handle_config_request);

  mb_wm_add_x_event_handler (wm, PropertyNotify, 
			     (MBWMXEventFunc)mb_wm_core_handle_property_notify);
  mb_wm_add_x_event_handler (wm, DestroyNotify, 
			     (MBWMXEventFunc)test_destroy_notify);

  mb_wm_add_x_event_handler (wm, KeyPress, 
			     (MBWMXEventFunc)test_key_press);

  mb_wm_atoms_init(wm);

  mb_wm_keys_init(wm);

  /* mb_wm_decor_init (wm); */

  base_foo (); 

  return True;
}
	   

