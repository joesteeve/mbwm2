#include "mb-wm.h"

enum {
  COOKIE_WIN_TYPE = 0,
  COOKIE_WIN_ATTR,
  COOKIE_WIN_GEOM,
  COOKIE_WIN_NAME,
  COOKIE_WIN_NAME_UTF8,
  COOKIE_WIN_SIZE_HINTS,
  COOKIE_WIN_WMHINTS,
  
  N_COOKIES
};

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

  /*
  if (props_req & MBWM_WINDOW_PROP_NAME)
    {
      cookies[COOKIE_WIN_NAME] 
	= mb_wm_property_req (wm,      
			      xwin, 
			      prop,
			      (wm)->atoms[MBWM_ATOM_WM_NAME],
			      1024L,
			      False,
			      XA_WM_NAME);

      cookies[COOKIE_WIN_NAME_UTF8] 
	mb_wm_property_utf8_req(wm, xwin, 
				wm->atoms[MBWM_ATOM_NET_WM_NAME]);
    }
  if (props_req & MBWM_WINDOW_PROP_WMHINTS)
    {
      cookies[COOKIE_WIN_WMHINTS] 
	= mb_wm_property_req (wm,      
			      xwin, 
			      prop,
			      (wm)->atoms[MBWM_ATOM_WM_HINTS],
			      1024L,
			      False,
			      XA_WM_HINTS);
    }
  */

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
	  || result_atom == NULL)
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

 abort:
  
  if (xwin_attr) 
    XFree(xwin_attr);

  return True;
}

