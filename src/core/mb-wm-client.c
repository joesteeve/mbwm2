#include "mb-wm.h"

/* 
 * base client 'class'
 */

enum
  {
    StackingNeedsSync   = (1<<1),
    GeometryNeedsSync   = (1<<2),
    VisibilityNeedsSync = (1<<3),
  };

struct MBWindowManagerClientPriv
{
  Bool realized;
  Bool mapped;
  int  sync_state;
};

static void
mb_wm_client_base_init (MBWindowManager             *wm, 
			MBWindowManagerClient       *client,
			MBWMWindow                  *win);
static void
mb_wm_client_base_realize (MBWindowManagerClient *client);

static void
mb_wm_client_base_stack (MBWindowManagerClient *client,
			 int                    flags);
static void
mb_wm_client_base_show (MBWindowManagerClient *client);

static void
mb_wm_client_base_hide (MBWindowManagerClient *client);

static void
mb_wm_client_base_destroy (MBWindowManagerClient *client);

static void
mb_wm_client_base_display_sync (MBWindowManagerClient *client);

static Bool
mb_wm_client_base_request_geometry (MBWindowManagerClient *client,
				    MBGeometry            *new_geometry,
				    MBWMClientReqGeomType  flags);

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

/* Window new */

#if 0
MBWMWindow*
mb_wm_client_window_new (MBWindowManager *wm, Window xwin)
{
  MBWMWindow *win = NULL;

  enum {
    COOKIE_WIN_TYPE = 0,
    COOKIE_WIN_ATTR,
    COOKIE_WIN_GEOM,
    N_COOKIES
  };

  MBWMCookie       cookies[N_COOKIES];
  Atom             actual_type_return, *result_atom = NULL;
  int              actual_format_return;
  unsigned long    nitems_return;
  unsigned long    bytes_after_return;
  unsigned int     foo;
  int              x_error_code;

  MBWMWindowAttributes *xwin_attr;

  win = mb_wm_util_malloc0(sizeof(MBWindowManagerClient));

  if (!win)
    return NULL; 		/* FIXME: Handle out of memory */

  win->xwindow = xwin;

  cookies[COOKIE_WIN_TYPE] 
    = mb_wm_property_atom_req(wm, xwin, wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE]);
  cookies[COOKIE_WIN_ATTR]
    = mb_wm_xwin_get_attributes (wm, xwin);

  cookies[COOKIE_WIN_GEOM]
    = mb_wm_xwin_get_geometry (wm, (Drawable)xwin);

  XSync(wm->xdpy, False);

  mb_wm_property_reply (wm,
			cookies[COOKIE_WIN_TYPE],
			&actual_type_return, 
			&actual_format_return, 
			&nitems_return, 
			&bytes_after_return, 
			(unsigned char **)&result_atom,
			&x_error_code);

  if (x_error_code
      || actual_type_return != XA_ATOM
      || actual_format_return != 32
      || nitems_return != 1
      || result_atom == NULL)
    {
      MBWM_DBG("### Warning net type prop failed ###");
    }
  else
    win->net_type = result_atom[0];

  if (result_atom) 
    XFree(result_atom);
  
  result_atom = NULL;

  xwin_attr = mb_wm_xwin_get_attributes_reply (wm,
					       cookies[COOKIE_WIN_ATTR],
					       &x_error_code);

  if (!xwin_attr || x_error_code)
    {
      MBWM_DBG("### Warning Get Attr Failed ( %i ) ###", x_error_code);
      goto abort;
    }

  if (!mb_wm_xwin_get_geometry_reply (wm,
				      cookies[COOKIE_WIN_GEOM],
				      &win->geometry,
				      &foo,
				      &win->depth,
				      &x_error_code))
    {
      MBWM_DBG("### Warning Get Geometry Failed ( %i ) ###", x_error_code);
      MBWM_DBG("###   Cookie ID was %li                ###", cookies[COOKIE_WIN_GEOM]);
      goto abort;
    }

  MBWM_DBG("@@@ New Window Obj @@@");
  MBWM_DBG("Win:  %lx", win->xwindow);
  MBWM_DBG("Type: %lx",win->net_type);
  MBWM_DBG("Geom: +%i+%i,%ix%i", win->geometry.x, win->geometry.y,
	   win->geometry.width, win->geometry.height);

  return win;

 abort:
  
  if (xwin_attr) 
    XFree(xwin_attr);

  free(win);

  return NULL;
}

#endif

/* base methods */

MBWindowManagerClient* 	/* FIXME: rename to mb_wm_client_base/class_new ? */
mb_wm_client_new (MBWindowManager *wm, MBWMWindow *win)
{
  MBWindowManagerClient *client = NULL;

  client = mb_wm_util_malloc0(sizeof(MBWindowManagerClient));

  if (!client)
    return NULL; 		/* FIXME: Handle out of memory */

  return client;
}
		   

void
mb_wm_client_base_init (MBWindowManager             *wm, 
			MBWindowManagerClient       *client,
			MBWMWindow *win)
{
  client->wmref  = wm;   
  client->window = win; 
  client->priv   = mb_wm_util_malloc0(sizeof(MBWindowManagerClientPriv));

  client->new      = NULL;	             /* NOT NEEDED ? */
  client->init     = mb_wm_client_base_init;  /* NOT NEEDED ? */
  client->realize  = mb_wm_client_base_realize;
  client->destroy  = mb_wm_client_base_destroy;
  client->geometry = mb_wm_client_base_request_geometry;
  client->stack    = mb_wm_client_base_stack;
  client->show     = mb_wm_client_base_show;
  client->hide     = mb_wm_client_base_hide;
  client->sync     = mb_wm_client_base_display_sync;

  /* Need to set these values for initial reparenting */

  client->frame_geometry.x      = client->window->geometry.x;
  client->frame_geometry.y      = client->window->geometry.y;
  client->frame_geometry.width  = client->window->geometry.width;
  client->frame_geometry.height = client->window->geometry.height;
}

void
mb_wm_client_init (MBWindowManager             *wm, 
		   MBWindowManagerClient       *client,
		   MBWMWindow                  *win)
{
  if (client->init)
    client->init(wm, client, win);
  else
    mb_wm_client_base_init (wm, client, win);      
}

void
mb_wm_client_realize (MBWindowManagerClient *client)
{
  MBWM_ASSERT (client->realize != NULL);

  client->realize(client);

  client->priv->realized = True;
}

Bool
mb_wm_client_is_realized (MBWindowManagerClient *client)
{
  return client->priv->realized;
}

static void
mb_wm_client_base_realize (MBWindowManagerClient *client)
{
  MBWindowManager *wm = client->wmref;

  XSetWindowAttributes attr;

  MBWM_ASSERT(client->window != NULL);

  /* create the frame window */

  attr.override_redirect = True;
  attr.background_pixel  = BlackPixel(wm->xdpy, wm->xscreen);
  attr.event_mask = MBWMChildMask|MBWMButtonMask|ExposureMask;
  
  client->xwin_frame 
    = XCreateWindow(wm->xdpy, wm->xwin_root, 
		    client->frame_geometry.x,
		    client->frame_geometry.y,
		    client->frame_geometry.width,
		    client->frame_geometry.height,
		    0,
		    CopyFromParent, 
		    CopyFromParent, 
		    CopyFromParent,
		    CWOverrideRedirect|CWEventMask|CWBackPixel,
		    &attr);

  XClearWindow(wm->xdpy, client->xwin_frame);

  /* FIXME: below definetly belongs elsewhere ...  */
  XMoveResizeWindow(wm->xdpy, 
		    MBWM_CLIENT_XWIN(client),
		    client->window->geometry.x,
		    client->window->geometry.y,
		    client->window->geometry.width,
		    client->window->geometry.height);

  XReparentWindow(wm->xdpy, 
		  MBWM_CLIENT_XWIN(client), 
		  client->xwin_frame, 
		  client->window->geometry.x - client->frame_geometry.x,
		  client->window->geometry.y  -client->frame_geometry.y);

  XSelectInput(wm->xdpy, 
	       MBWM_CLIENT_XWIN(client),
	       PropertyChangeMask);
}

/* ## Stacking ## */

static void
mb_wm_client_base_stack (MBWindowManagerClient *client,
			 int                    flags)
{
  /* Stack to highest/lowest possible possition in stack */

  /* then mark stacking dirty */

  /* maybe we need to call a stack manager ? */
}

void
mb_wm_client_stack (MBWindowManagerClient *client,
		    int                    flags)
{
  MBWM_ASSERT (client->stack != NULL);

  client->stack(client, flags);

  mb_wm_client_stacking_mark_dirty (client);
}

Bool
mb_wm_client_needs_stack_sync (MBWindowManagerClient *client)
{
  return (client->priv->sync_state & StackingNeedsSync);
}


static void
mb_wm_client_base_show (MBWindowManagerClient *client)
{
  client->priv->mapped = True;
  /* mark dirty somehow */
}

void
mb_wm_client_show (MBWindowManagerClient *client)
{
  MBWM_ASSERT (client->show != NULL);

  client->show(client);

  mb_wm_client_visibility_mark_dirty (client);
}

static void
mb_wm_client_base_hide (MBWindowManagerClient *client)
{
  client->priv->mapped = False;
  /* mark dirty somehow */

}

void
mb_wm_client_hide (MBWindowManagerClient *client)
{
  MBWM_ASSERT (client->hide != NULL);

  client->hide(client);

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

static void
mb_wm_client_base_display_sync (MBWindowManagerClient *client)
{
  MBWindowManager *wm = client->wmref;

  MBWM_MARK();

  mb_wm_util_trap_x_errors();  

  if (mb_wm_client_needs_geometry_sync (client))
    {
      if (client->xwin_frame)
	XMoveResizeWindow(wm->xdpy, 
			  client->xwin_frame,
			  client->frame_geometry.x,
			  client->frame_geometry.y,
			  client->frame_geometry.width,
			  client->frame_geometry.height);
      
      XMoveResizeWindow(wm->xdpy, 
			MBWM_CLIENT_XWIN(client),
			client->window->geometry.x - client->frame_geometry.x,
			client->window->geometry.y - client->frame_geometry.y,
			client->window->geometry.width,
			client->window->geometry.height);
      
      /* FIXME: need flags to handle other stuff like confugre events etc */

    }

  if (mb_wm_client_needs_visibility_sync (client))
    {

      if (mb_wm_client_is_mapped (client))
	{
	  if (client->xwin_frame)
	    {
	      XMapWindow(wm->xdpy, client->xwin_frame); 
	      XMapSubwindows(wm->xdpy, client->xwin_frame);
	    }
	  else
	    XMapWindow(wm->xdpy, MBWM_CLIENT_XWIN(client)); 
	}
      else
	{
	  if (client->xwin_frame)
	    XUnmapWindow(wm->xdpy, client->xwin_frame); 
	  else
	    XMapWindow(wm->xdpy, MBWM_CLIENT_XWIN(client)); 
	}
    }

  XSync(wm->xdpy, False);
  mb_wm_util_untrap_x_errors();  
}

void
mb_wm_client_display_sync (MBWindowManagerClient *client)
{
  MBWM_ASSERT (client->sync != NULL);

  client->sync(client);

  client->priv->sync_state = 0;
}

void
mb_wm_client_destroy (MBWindowManagerClient *client)
{
  /* free everything up */
  MBWM_ASSERT (client->destroy != NULL);

  client->destroy(client);

  mb_wm_display_sync_queue (client->wmref);
}

static void
mb_wm_client_base_destroy (MBWindowManagerClient *client)
{
  MBWindowManager *wm = client->wmref;

  MBWM_MARK();

  mb_wm_util_trap_x_errors();  

  XDestroyWindow(wm->xdpy, client->xwin_frame);

  XSync(wm->xdpy, False);
  mb_wm_util_untrap_x_errors();  
}

static Bool
mb_wm_client_base_request_geometry (MBWindowManagerClient *client,
				    MBGeometry            *new_geometry,
				    MBWMClientReqGeomType  flags)
{
  /* 
   *  flags are
   *
   *  MBReqGeomDontCommit
   *  MBReqGeomIsViaConfigureReq
   *  MBReqGeomIsViaUserAction
   *  MBReqGeomIsViaLayoutManager
   *  MBReqGeomForced
   *
  */

  /* Dont actually change anything - mb_wm_core_sync() should do that 
   * but mark dirty and 'queue any extra info like configure req'.   
  */

  if (flags & MBWMClientReqGeomIsViaLayoutManager)
    {
      MBWM_DBG("called with 'MBWMClientReqGeomIsViaLayoutManager' %ix%i+%i+%i",
	       new_geometry->width,
	       new_geometry->height,
	       new_geometry->x,
	       new_geometry->y);

      client->frame_geometry.x = new_geometry->x;
      client->frame_geometry.y = new_geometry->y;
      client->frame_geometry.width = new_geometry->width;
      client->frame_geometry.height = new_geometry->height;

      client->window->geometry.x = client->frame_geometry.x + 4;
      client->window->geometry.y = client->frame_geometry.y + 4;
      client->window->geometry.width  = client->frame_geometry.width - 8;
      client->window->geometry.height = client->frame_geometry.height - 8;

      mb_wm_client_geometry_mark_dirty (client);

      return True; /* Geometry accepted */
    }

  return TRUE;
}

Bool
mb_wm_client_request_geometry (MBWindowManagerClient *client,
			       MBGeometry            *new_geometry,
			       MBWMClientReqGeomType  flags)
{
  MBWM_ASSERT (client->geometry != NULL);

  return client->geometry(client, new_geometry, flags); 
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


