#include "mb-wm.h"

/* Via Xatomtype.h */
#define NumPropWMHintsElements 9 /* number of elements in this structure */

enum {
  COOKIE_WIN_TYPE = 0,
  COOKIE_WIN_ATTR,
  COOKIE_WIN_GEOM,
  COOKIE_WIN_NAME,
  COOKIE_WIN_NAME_UTF8,
  COOKIE_WIN_SIZE_HINTS,
  COOKIE_WIN_WM_HINTS,
  COOKIE_WIN_TRANSIENCY,
  
  N_COOKIES
};

static Bool
validate_reply(void)
{
  ;
}	  

MBWMWindow*
mb_wm_client_window_new (MBWindowManager *wm, Window xwin)
{
  MBWMWindow *win = NULL;

  win = mb_wm_util_malloc0(sizeof(MBWMWindow));

  if (!win)
    return NULL; 		/* FIXME: Handle out of memory */

  win->xwindow = xwin;

  mb_wm_window_sync_properties (wm, win, MBWM_WINDOW_PROP_ALL);

  return win;
}

void
mb_wm_window_change_property (MBWindowManager *wm,
			      MBWMWindow      *win,
			      Atom             prop,
			      Atom             type,
			      int              format,
			      void            *data,
			      int              n_elements)
{
  /*
  XChangeProperty (wm->xdpy, 
		   win->xwindow, 
		   prop, 
		   type, 
		   format, 
		   PropModeReplace,
		   (unsigned char *)data,
		   n_elements);
  */
}
			    

Bool
mb_wm_window_sync_properties (MBWindowManager *wm,
			      MBWMWindow      *win,
			      unsigned long    props_req)
{
  MBWMCookie       cookies[N_COOKIES];
  Atom             actual_type_return, *result_atom = NULL;
  int              actual_format_return;
  unsigned long    nitems_return;
  unsigned long    bytes_after_return;
  unsigned int     foo;
  int              x_error_code;
  Window           xwin;

  MBWMWindowAttributes *xwin_attr = NULL;
  XWMHints             *hints     = NULL;

  xwin = win->xwindow;

  if (props_req & MBWM_WINDOW_PROP_WIN_TYPE)
    cookies[COOKIE_WIN_TYPE] 
      = mb_wm_property_atom_req(wm, xwin, 
				wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE]);

  if (props_req & MBWM_WINDOW_PROP_ATTR)
    cookies[COOKIE_WIN_ATTR]
      = mb_wm_xwin_get_attributes (wm, xwin);

  if (props_req & MBWM_WINDOW_PROP_GEOMETRY)
    cookies[COOKIE_WIN_GEOM]
      = mb_wm_xwin_get_geometry (wm, (Drawable)xwin);

  if (props_req & MBWM_WINDOW_PROP_NAME)
    {
      cookies[COOKIE_WIN_NAME] 
	= mb_wm_property_req (wm,      
			      xwin, 
			      wm->atoms[MBWM_ATOM_WM_NAME],
			      0,
			      2048L,
			      False,
			      XA_STRING);

      cookies[COOKIE_WIN_NAME_UTF8] =
	mb_wm_property_utf8_req(wm, xwin, 
				wm->atoms[MBWM_ATOM_NET_WM_NAME]);
    }

  if (props_req & MBWM_WINDOW_PROP_WM_HINTS)
    {
      cookies[COOKIE_WIN_WM_HINTS] 
	= mb_wm_property_req (wm,      
			      xwin, 
			      wm->atoms[MBWM_ATOM_WM_HINTS],
			      0,
			      1024L,
			      False,
			      XA_WM_HINTS);
    }

  if (props_req & MBWM_WINDOW_PROP_TRANSIENCY)
    {
      cookies[COOKIE_WIN_TRANSIENCY] 
	= mb_wm_property_req (wm,      
			      xwin, 
			      wm->atoms[MBWM_ATOM_WM_TRANSIENT_FOR],
			      0,
			      1L,
			      False,
			      XA_WINDOW);
    }

  /* bundle all pending requests to server and wait for replys */
  XSync(wm->xdpy, False); 

  if (props_req & MBWM_WINDOW_PROP_WIN_TYPE)
    {
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
	  || result_atom == NULL
	  )
	{
	  MBWM_DBG("### Warning net type prop failed ###");
	}
      else
	win->net_type = result_atom[0];

      if (result_atom) 
	XFree(result_atom);
  
      result_atom = NULL;
    }

  if (props_req & MBWM_WINDOW_PROP_ATTR)
    {
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
      
    }

  if (props_req & MBWM_WINDOW_PROP_NAME)
    {
      if (win->name)
	XFree(win->name);

      /* Prefer UTF8 Naming... */
      win->name 
	= mb_wm_property_get_reply_and_validate (wm,
						 cookies[COOKIE_WIN_NAME],
						 wm->atoms[MBWM_ATOM_UTF8_STRING],
						 8,
						 0,
						 NULL,
						 &x_error_code);

      /* FIXME: Validate the UTF8 */

      if (!win->name)
	{
	  /* FIXME: Should flag up name could be in some wacko encoding ? */
	  win->name 
	    = mb_wm_property_get_reply_and_validate (wm,
						     cookies[COOKIE_WIN_NAME_UTF8],
						     XA_STRING,
						     8,
						     0,
						     NULL,
						     &x_error_code);
	}

      if (win->name == NULL)
	win->name = strdup("unknown");

      MBWM_DBG("@@@ New Window Name: '%s' @@@", win->name);
    }

  if (props_req & MBWM_WINDOW_PROP_WM_HINTS)
    {
      XWMHints *wmhints = NULL;

      /* NOTE: pre-R3 X strips group element so will faill for that */
      wmhints = mb_wm_property_get_reply_and_validate (wm,
						       cookies[COOKIE_WIN_WM_HINTS],
						       XA_WM_HINTS,
						       32,
						       NumPropWMHintsElements,
						       NULL,
						       &x_error_code);
 
      if (wmhints)
	{
	  MBWM_DBG("@@@ New Window WM Hints @@@");

	  if (wmhints->flags & InputHint)
	    {
	      win->want_key_input = wmhints->input;
	      MBWM_DBG("Want key input: %s", wmhints->input ? "yes" : "no" );
	    }

	  if (wmhints->flags & StateHint)
	    {
	      win->initial_state = wmhints->initial_state;
	      MBWM_DBG("Initial State : %i", wmhints->initial_state );
	    }

	  if (wmhints->flags & IconPixmapHint)
	    {
	      win->icon_pixmap = wmhints->icon_pixmap;
	      MBWM_DBG("Icon Pixmap   : %lx", wmhints->icon_pixmap );
	    }

	  if (wmhints->flags & IconMaskHint)
	    {
	      win->icon_pixmap_mask = wmhints->icon_mask;
	      MBWM_DBG("Icon Mask     : %lx", wmhints->icon_mask );
	    }
	  
	  if (wmhints->flags & WindowGroupHint)
	    {
	      win->xwin_group = wmhints->window_group;
	      MBWM_DBG("Window Group  : %lx", wmhints->window_group );
	    }

	  /* NOTE: we ignore icon window stuff */

	  XFree(wmhints);
	}
    }

  if (props_req & MBWM_WINDOW_PROP_TRANSIENCY)
    {
      Window *trans_win = NULL;

      trans_win 
	= mb_wm_property_get_reply_and_validate (wm,
						 cookies[COOKIE_WIN_TRANSIENCY],
						 MBWM_ATOM_WM_TRANSIENT_FOR,
						 32,
						 1,
						 NULL,
						 &x_error_code);
      
      if (trans_win)
	{
	  MBWM_DBG("@@@ Window transient for %lx @@@", *trans_win);
	  win->xwin_transient_for = *trans_win;
	  XFree(trans_win);
	}
      else MBWM_DBG("@@@ Window transient for nothing @@@");
    }

 abort:
  
  if (xwin_attr) 
    XFree(xwin_attr);

  return True;
}

