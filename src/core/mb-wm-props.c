#include "mb-wm.h"
#include "xas.h"

MBWMCookie
mb_wm_property_req (MBWindowManager *wm,
		    Window           win,
		    Atom             property,
		    long             offset,
		    long             length,
		    Bool             delete,
		    Atom             req_type)
{
  XasCookie cookie;

  cookie = xas_get_property(wm->xas_context, 
			    win,
			    property,
			    offset,
			    length,
			    delete,
			    req_type);

  return (MBWMCookie)cookie;
}


Status
mb_wm_property_reply (MBWindowManager  *wm,
		      MBWMCookie        cookie,
		      Atom             *actual_type_return, 
		      int              *actual_format_return, 
		      unsigned long    *nitems_return, 
		      unsigned long    *bytes_after_return, 
		      unsigned char   **prop_return,
		      int              *x_error_code)
{
  return  xas_get_property_reply(wm->xas_context, 
			         (XasCookie)cookie,
				 actual_type_return, 
				 actual_format_return, 
				 nitems_return, 
				 bytes_after_return, 
				 prop_return,
				 x_error_code);
}

Bool
mb_wm_property_have_reply (MBWindowManager     *wm,
			   MBWMCookie           cookie)
{
  return xas_have_reply(wm->xas_context, (XasCookie)cookie);
}


MBWMCookie
mb_wm_xwin_get_attributes (MBWindowManager   *wm,
			   Window             win)
{
  return xas_get_window_attributes(wm->xas_context, win);
}

MBWMCookie
mb_wm_xwin_get_geometry (MBWindowManager   *wm,
			 Drawable            d)
{
  return xas_get_geometry(wm->xas_context, d);
}

MBWMWindowAttributes*
mb_wm_xwin_get_attributes_reply (MBWindowManager   *wm,
				 MBWMCookie         cookie,
				 int               *x_error_code)
{
  return (MBWMWindowAttributes*)
    xas_get_window_attributes_reply(wm->xas_context,
				    cookie, 
				    x_error_code); 
}

Status
mb_wm_xwin_get_geometry_reply (MBWindowManager   *wm,
			       XasCookie          cookie,
			       MBGeometry        *geom_return,
			       unsigned int      *border_width_return,
			       unsigned int      *depth_return,
			       int               *x_error_code)
{
  return xas_get_geometry_reply (wm->xas_context,
				 cookie,
				 &geom_return->x,
				 &geom_return->y,
				 &geom_return->width,
				 &geom_return->height,
				 border_width_return,
				 depth_return,
				 x_error_code);
}
