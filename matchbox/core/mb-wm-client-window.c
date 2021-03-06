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

/* Via Xatomtype.h */
#define NumPropWMHintsElements 9 /* number of elements in this structure */
#define NumPropWMSizeHintsElements 18

enum {
  COOKIE_WIN_TYPE = 0,
  COOKIE_WIN_ATTR,
  COOKIE_WIN_GEOM,
  COOKIE_WIN_NAME,
  COOKIE_WIN_NAME_UTF8,
  COOKIE_WIN_WM_HINTS,
  COOKIE_WIN_WM_NORMAL_HINTS,
  COOKIE_WIN_TRANSIENCY,
  COOKIE_WIN_PROTOS,
  COOKIE_WIN_MACHINE,
  COOKIE_WIN_PID,
  COOKIE_WIN_ICON,
  COOKIE_WIN_ACTIONS,
  COOKIE_WIN_USER_TIME,
  COOKIE_WIN_CM_TRANSLUCENCY,
  COOKIE_WIN_NET_STATE,
  COOKIE_WIN_MWM_HINTS,

  N_COOKIES
};

#define PROP_MOTIF_WM_HINTS_ELEMENTS    5
#define MWM_HINTS_DECORATIONS   (1L << 1)

#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)

static void
mb_wm_client_window_class_init (MBWMObjectClass *klass)
{
  MBWMClientWindowClass *rw_class;

  MBWM_MARK();

  rw_class = (MBWMClientWindowClass *)klass;

#if MBWM_WANT_DEBUG
  klass->klass_name = "MBWMClientWindow";
#endif
}

static void
mb_wm_client_window_destroy (MBWMObject *this)
{
  MBWMClientWindow * win = MB_WM_CLIENT_WINDOW (this);
  MBWMList         * l   = win->icons;

  if (win->name)
    XFree (win->name);

  if (win->machine)
    XFree (win->machine);

  while (l)
    {
      mb_wm_rgba_icon_free ((MBWMRgbaIcon *)l->data);
      l = l->next;
    }
}

static int
mb_wm_client_window_init (MBWMObject *this, va_list vap)
{
  MBWMClientWindow *win = MB_WM_CLIENT_WINDOW (this);
  MBWMObjectProp    prop;
  MBWindowManager  *wm = NULL;
  Window            xwin = None;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropWm:
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	case MBWMObjectPropXwindow:
	  xwin = va_arg(vap, Window);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  if (!wm || !xwin)
    {
      fprintf (stderr, "--- error, no wm or xwin ---\n");
      return 0;
    }

  win->xwindow = xwin;
  win->wm = wm;

  mb_wm_client_window_sync_properties (win, MBWM_WINDOW_PROP_ALL);

  return 1;
}

int
mb_wm_client_window_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMClientWindowClass),
	sizeof (MBWMClientWindow),
	mb_wm_client_window_init,
	mb_wm_client_window_destroy,
	mb_wm_client_window_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

MBWMClientWindow*
mb_wm_client_window_new (MBWindowManager *wm, Window xwin)
{
  MBWMClientWindow *win;

  win = MB_WM_CLIENT_WINDOW (mb_wm_object_new (MB_WM_TYPE_CLIENT_WINDOW,
					       MBWMObjectPropWm,      wm,
					       MBWMObjectPropXwindow, xwin,
					       NULL));
  return win;
}

/*
 * Creates MBWMIcon from raw _NET_WM_ICON property data, returning
 * pointer to where the next icon might be in the data
 */
static unsigned long *
icon_from_net_wm_icon (unsigned long * data, MBWMRgbaIcon ** mb_icon)
{
  MBWMRgbaIcon * icon = mb_wm_rgba_icon_new ();
  size_t byte_len;

  *mb_icon = icon;

  if (!icon)
    return data;

  icon->width  = *data++;
  icon->height = *data++;

  byte_len = sizeof (unsigned long) * icon->width * icon->height;

  icon->pixels = malloc (byte_len);

  if (!icon->pixels)
    {
      mb_wm_rgba_icon_free (icon);
      *mb_icon = NULL;
      return (data - 2);
    }

  memcpy (icon->pixels, data, byte_len);

  MBWM_DBG("@@@ Icon %d x %d @@@", icon->width, icon->height);

  return (data + icon->width * icon->height);
}

Bool
mb_wm_client_window_sync_properties ( MBWMClientWindow *win,
				     unsigned long     props_req)
{
  MBWMCookie       cookies[N_COOKIES];
  MBWindowManager *wm = win->wm;
  Atom             actual_type_return, *result_atom = NULL;
  int              actual_format_return;
  unsigned long    nitems_return;
  unsigned long    bytes_after_return;
  unsigned int     foo;
  int              x_error_code;
  Window           xwin;
  int              changes = 0;

  MBWMClientWindowAttributes *xwin_attr = NULL;

  xwin = win->xwindow;

  if (props_req & MBWM_WINDOW_PROP_WIN_TYPE)
    cookies[COOKIE_WIN_TYPE]
      = mb_wm_property_atom_req(wm, xwin,
				wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE]);

  if (props_req & MBWM_WINDOW_PROP_NET_STATE)
    cookies[COOKIE_WIN_NET_STATE]
      = mb_wm_property_atom_req(wm, xwin,
				wm->atoms[MBWM_ATOM_NET_WM_STATE]);

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

  if (props_req & MBWM_WINDOW_PROP_WM_NORMAL_HINTS)
    {
      cookies[COOKIE_WIN_WM_NORMAL_HINTS]
	= mb_wm_property_req (wm,
			      xwin,
			      wm->atoms[MBWM_ATOM_WM_NORMAL_HINTS],
			      0,
			      1024L,
			      False,
			      XA_WM_SIZE_HINTS);
    }

  if (props_req & MBWM_WINDOW_PROP_MWM_HINTS)
    {
      cookies[COOKIE_WIN_MWM_HINTS]
	= mb_wm_property_req (wm,
			      xwin,
			      wm->atoms[MBWM_ATOM_MOTIF_WM_HINTS],
			      0,
			      PROP_MOTIF_WM_HINTS_ELEMENTS,
			      False,
			      wm->atoms[MBWM_ATOM_MOTIF_WM_HINTS]);
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

  if (props_req & MBWM_WINDOW_PROP_PROTOS)
    {
      cookies[COOKIE_WIN_PROTOS]
	= mb_wm_property_atom_req(wm, xwin,
				  wm->atoms[MBWM_ATOM_WM_PROTOCOLS]);
    }

  if (props_req & MBWM_WINDOW_PROP_CLIENT_MACHINE)
    {
      cookies[COOKIE_WIN_MACHINE]
	= mb_wm_property_req (wm,
			      xwin,
			      wm->atoms[MBWM_ATOM_WM_CLIENT_MACHINE],
			      0,
			      2048L,
			      False,
			      XA_STRING);
    }

  if (props_req & MBWM_WINDOW_PROP_NET_PID)
    {
      cookies[COOKIE_WIN_PID]
	= mb_wm_property_cardinal_req (wm,
				       xwin,
				       wm->atoms[MBWM_ATOM_NET_WM_PID]);
    }

  if (props_req & MBWM_WINDOW_PROP_NET_ICON)
    {
      cookies[COOKIE_WIN_ICON]
	= mb_wm_property_cardinal_req (wm,
				       xwin,
				       wm->atoms[MBWM_ATOM_NET_WM_ICON]);
    }

  if (props_req & MBWM_WINDOW_PROP_NET_USER_TIME)
    {
      cookies[COOKIE_WIN_USER_TIME]
	= mb_wm_property_cardinal_req (wm,
				       xwin,
				       wm->atoms[MBWM_ATOM_NET_WM_USER_TIME]);
    }

  if (props_req & MBWM_WINDOW_PROP_CM_TRANSLUCENCY)
    {
      cookies[COOKIE_WIN_CM_TRANSLUCENCY]
	= mb_wm_property_cardinal_req (wm,
				       xwin,
				       wm->atoms[MBWM_ATOM_CM_TRANSLUCENCY]);
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
	{
	  if (win->net_type != result_atom[0])
	    changes |= MBWM_WINDOW_PROP_WIN_TYPE;

	  win->net_type = result_atom[0];
	}

      if (result_atom)
	XFree(result_atom);

      result_atom = NULL;
    }

  if (props_req & MBWM_WINDOW_PROP_NET_STATE)
    {
      mb_wm_property_reply (wm,
			    cookies[COOKIE_WIN_NET_STATE],
			    &actual_type_return,
			    &actual_format_return,
			    &nitems_return,
			    &bytes_after_return,
			    (unsigned char **)&result_atom,
			    &x_error_code);

      if (x_error_code
	  || actual_type_return != XA_ATOM
	  || actual_format_return != 32
	  || nitems_return <= 0
	  || result_atom == NULL
	  )
	{
	  MBWM_DBG("### Warning net type state failed ###");
	}
      else
	{
	  int i;
	  win->ewmh_state = 0;

	  for (i = 0; i < nitems_return; ++i)
	    {
	      if (result_atom[i] == wm->atoms[MBWM_ATOM_NET_WM_STATE_MODAL])
		win->ewmh_state |= MBWMClientWindowEWMHStateModal;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_STICKY])
		win->ewmh_state |= MBWMClientWindowEWMHStateSticky;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_MAXIMIZED_VERT])
		win->ewmh_state |= MBWMClientWindowEWMHStateMaximisedVert;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_MAXIMIZED_HORZ])
		win->ewmh_state |= MBWMClientWindowEWMHStateMaximisedHorz;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_SHADED])
		win->ewmh_state |= MBWMClientWindowEWMHStateShaded;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_SKIP_TASKBAR])
		win->ewmh_state |= MBWMClientWindowEWMHStateSkipTaskbar;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_SKIP_PAGER])
		win->ewmh_state |= MBWMClientWindowEWMHStateSkipPager;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_HIDDEN])
		win->ewmh_state |= MBWMClientWindowEWMHStateHidden;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_FULLSCREEN])
		win->ewmh_state |= MBWMClientWindowEWMHStateFullscreen;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_ABOVE])
		win->ewmh_state |= MBWMClientWindowEWMHStateAbove;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_BELOW])
		win->ewmh_state |= MBWMClientWindowEWMHStateBelow;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_STATE_DEMANDS_ATTENTION])
		win->ewmh_state |= MBWMClientWindowEWMHStateDemandsAttention;
	    }
	}

      changes |= MBWM_WINDOW_PROP_NET_STATE;

      if (result_atom)
	XFree(result_atom);

      result_atom = NULL;
    }

  if (props_req & MBWM_WINDOW_PROP_GEOMETRY)
    {
      if (!mb_wm_xwin_get_geometry_reply (wm,
					  cookies[COOKIE_WIN_GEOM],
					  &win->x_geometry,
					  &foo,
					  &win->depth,
					  &x_error_code))
	{
	  MBWM_DBG("### Warning Get Geometry Failed ( %i ) ###",
		   x_error_code);
	  MBWM_DBG("###   Cookie ID was %li                ###",
		   cookies[COOKIE_WIN_GEOM]);
	  goto abort;
	}

      MBWM_DBG("@@@ New Window Obj @@@");
      MBWM_DBG("Win:  %lx", win->xwindow);
      MBWM_DBG("Type: %lx",win->net_type);
      MBWM_DBG("Geom: +%i+%i,%ix%i",
	       win->x_geometry.x,
	       win->x_geometry.y,
	       win->x_geometry.width,
	       win->x_geometry.height);

      /* FIXME is it right that we directly/immeditaly update
       * win->geometry here, perhaps that should be left as a
       * responsability for a signal handler? */
      win->geometry = win->x_geometry;
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

      win->visual            = xwin_attr->visual;
      win->colormap          = xwin_attr->colormap;

      /* Window gravity should not be set from the window attribute
       * but using the WM_NORMAL_HINTS atom
       */

      /* win->gravity           = xwin_attr->win_gravity;*/
      win->override_redirect = xwin_attr->override_redirect;
      win->window_class      = xwin_attr->class;
    }

  if (props_req & MBWM_WINDOW_PROP_NAME)
    {
      if (win->name)
	XFree(win->name);

      /* Prefer UTF8 Naming... */
      win->name
	= mb_wm_property_get_reply_and_validate (wm,
						 cookies[COOKIE_WIN_NAME_UTF8],
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
						     cookies[COOKIE_WIN_NAME],
						     XA_STRING,
						     8,
						     0,
						     NULL,
						     &x_error_code);
	}

      if (win->name == NULL)
	win->name = strdup("unknown");

      MBWM_DBG("@@@ New Window Name: '%s' @@@", win->name);

      changes |= MBWM_WINDOW_PROP_NAME;
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

	  /* FIXME: should track better if thus has changed or not */
	  changes |= MBWM_WINDOW_PROP_WM_HINTS;
	}
    }
  if (props_req & MBWM_WINDOW_PROP_WM_NORMAL_HINTS)
    {
      XSizeHints *sizehints = NULL;

      sizehints = mb_wm_property_get_reply_and_validate (wm,
						       cookies[COOKIE_WIN_WM_NORMAL_HINTS],
						       XA_WM_SIZE_HINTS,
						       32,
						       NumPropWMSizeHintsElements,
						       NULL,
						       &x_error_code);
      if (sizehints)
        {
	  MBWM_DBG("@@@ New Window WM Normal Hints @@@");

          if (sizehints->flags & PWinGravity)
	    {
	      win->gravity = sizehints->win_gravity;
	      MBWM_DBG("Window Gravity : %i", sizehints->win_gravity);
	    }

	  win->user_pos = (sizehints->flags & USPosition) ? True : False;
	  MBWM_DBG ("User positioning : %s", win->user_pos ? "yes" : "no");

	  /* May be used to recover other information */

          XFree (sizehints);

	  changes |= MBWM_WINDOW_PROP_WM_NORMAL_HINTS;
        }

    }

  if (props_req & MBWM_WINDOW_PROP_MWM_HINTS)
    {
      /*
       * Handle the Motif decoration hint (Gtk uses it).
       *
       * We currently only differentiate between decorated and undecorated
       * windows, but do not allow finer customisation of the decor.
       */
      typedef struct
      {
	unsigned long       flags;
	unsigned long       functions;
	unsigned long       decorations;
	long                inputMode;
	unsigned long       status;
      } MotifWmHints;

      MotifWmHints *mwmhints = NULL;

      mwmhints =
	mb_wm_property_get_reply_and_validate (wm,
					       cookies[COOKIE_WIN_MWM_HINTS],
					       wm->atoms[MBWM_ATOM_MOTIF_WM_HINTS],
					       32,
					       PROP_MOTIF_WM_HINTS_ELEMENTS,
					       NULL,
					       &x_error_code);

      if (mwmhints)
	{
	  MBWM_DBG("@@@ New Window Motif WM Hints @@@");

	  if (mwmhints->flags & MWM_HINTS_DECORATIONS)
	    {
	      if (mwmhints->decorations == 0)
		{
		  win->undecorated = TRUE;
		}
	    }

	  XFree(mwmhints);

	  /* FIXME: should track better if thus has changed or not */
	  changes |= MBWM_WINDOW_PROP_MWM_HINTS;
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

	  if (*trans_win != win->xwin_transient_for)
	    changes |= MBWM_WINDOW_PROP_TRANSIENCY;

	  win->xwin_transient_for = *trans_win;
	  XFree(trans_win);
	}
      else MBWM_DBG("@@@ Window transient for nothing @@@");

      changes |= MBWM_WINDOW_PROP_TRANSIENCY;
    }

  if (props_req & MBWM_WINDOW_PROP_PROTOS)
    {
      mb_wm_property_reply (wm,
			    cookies[COOKIE_WIN_PROTOS],
			    &actual_type_return,
			    &actual_format_return,
			    &nitems_return,
			    &bytes_after_return,
			    (unsigned char **)&result_atom,
			    &x_error_code);

      if (x_error_code
	  || actual_type_return != XA_ATOM
	  || actual_format_return != 32
	  || result_atom == NULL
	  )
	{
	  MBWM_DBG("### Warning wm protocols prop failed ###");
	}
      else
	{
	  int i;

	  for (i = 0; i < nitems_return; ++i)
	    {
	      if (result_atom[i] == wm->atoms[MBWM_ATOM_WM_TAKE_FOCUS])
		win->protos |= MBWMClientWindowProtosFocus;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_WM_DELETE_WINDOW])
		win->protos |= MBWMClientWindowProtosDelete;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_HELP])
		win->protos |= MBWMClientWindowProtosContextHelp;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_ACCEPT])
		win->protos |= MBWMClientWindowProtosContextAccept;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_CONTEXT_CUSTOM])
		win->protos |= MBWMClientWindowProtosContextCustom;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_PING])
		win->protos |= MBWMClientWindowProtosPing;
	      else if (result_atom[i] ==
		       wm->atoms[MBWM_ATOM_NET_WM_SYNC_REQUEST])
		win->protos |= MBWMClientWindowProtosSyncRequest;
	      else
		MBWM_DBG("Unhandled WM Protocol %d", result_atom[i]);
	    }
	}

      if (result_atom)
	XFree(result_atom);

      changes |= MBWM_WINDOW_PROP_PROTOS;

      result_atom = NULL;
    }

  if (props_req & MBWM_WINDOW_PROP_CLIENT_MACHINE)
    {
      if (win->machine)
	XFree(win->machine);

      win->machine
	= mb_wm_property_get_reply_and_validate (wm,
						 cookies[COOKIE_WIN_MACHINE],
						 XA_STRING,
						 8,
						 0,
						 NULL,
						 &x_error_code);

      if (!win->machine)
	{
	  MBWM_DBG("### Warning wm client machine prop failed ###");
	}
      else
	{
	  MBWM_DBG("@@@ Client machine %s @@@", win->machine);
	}
    }

  if (props_req & MBWM_WINDOW_PROP_NET_PID)
    {
      unsigned int *pid = NULL;

      mb_wm_property_reply (wm,
			    cookies[COOKIE_WIN_PID],
			    &actual_type_return,
			    &actual_format_return,
			    &nitems_return,
			    &bytes_after_return,
			    (unsigned char **)&pid,
			    &x_error_code);

      if (x_error_code
	  || actual_type_return != XA_CARDINAL
	  || actual_format_return != 32
	  || pid == NULL
	  )
	{
	  MBWM_DBG("### Warning net pid prop failed ###");
	}
      else
	{
	  win->pid = (pid_t) *pid;
	  MBWM_DBG("@@@ pid %d @@@", win->pid);
	}

      if (pid)
	XFree(pid);
    }

  if (props_req & MBWM_WINDOW_PROP_CM_TRANSLUCENCY)
    {
      int *translucency = NULL;

      mb_wm_property_reply (wm,
			    cookies[COOKIE_WIN_CM_TRANSLUCENCY],
			    &actual_type_return,
			    &actual_format_return,
			    &nitems_return,
			    &bytes_after_return,
			    (unsigned char **)&translucency,
			    &x_error_code);

      if (x_error_code
	  || actual_type_return != XA_CARDINAL
	  || actual_format_return != 32
	  || translucency == NULL
	  )
	{
	  MBWM_DBG("### no CM_TRANSLUCENCY prop ###");
	  win->translucency = -1;
	}
      else
	{
	  win->translucency = *translucency;
	  MBWM_DBG("@@@ translucency %d @@@", win->translucency);
	}

      changes |= MBWM_WINDOW_PROP_CM_TRANSLUCENCY;

      if (translucency)
	XFree (translucency);
    }

  if (props_req & MBWM_WINDOW_PROP_NET_ICON)
    {
      unsigned long *icons = NULL;

      mb_wm_property_reply (wm,
			    cookies[COOKIE_WIN_ICON],
			    &actual_type_return,
			    &actual_format_return,
			    &nitems_return,
			    &bytes_after_return,
			    (unsigned char **)&icons,
			    &x_error_code);

      if (x_error_code
	  || actual_type_return != XA_CARDINAL
	  || actual_format_return != 32
	  || icons == NULL
	  )
	{
	  MBWM_DBG("### Warning net icon prop failed ###");
	}
      else
	{
	  MBWMList *l = win->icons;
	  MBWMList *list_end = NULL;
	  unsigned long *p = icons;
	  unsigned long *p_end = icons + nitems_return;

	  while (l)
	    {
	      MBWMRgbaIcon * ic = l->data;

	      mb_wm_rgba_icon_free (ic);

	      l = l->next;
	    }

	  while (p < p_end)
	    {
	      l = mb_wm_util_malloc0 (sizeof (MBWMList));
	      p = icon_from_net_wm_icon (p, (MBWMRgbaIcon **)&l->data);

	      if (list_end)
		{
		  l->prev = list_end;
		  list_end->next = l;
		}
	      else
		{
		  win->icons = l;
		}

	      list_end = l;
	    }
	}

      if (icons)
	XFree(icons);

      changes |= MBWM_WINDOW_PROP_NET_ICON;

    }

  if (props_req & MBWM_WINDOW_PROP_NET_USER_TIME)
    {
      unsigned long *user_time = NULL;

      mb_wm_property_reply (wm,
			    cookies[COOKIE_WIN_USER_TIME],
			    &actual_type_return,
			    &actual_format_return,
			    &nitems_return,
			    &bytes_after_return,
			    (unsigned char **)&user_time,
			    &x_error_code);

      if (x_error_code
	  || actual_type_return != XA_CARDINAL
	  || actual_format_return != 32
	  || user_time == NULL
	  )
	{
	  MBWM_DBG("### Warning _NET_WM_USER_TIME failed ###");
	}
      else
	{
	  win->user_time = *user_time;
	  MBWM_DBG("@@@ user time %d @@@", win->user_time);
	}

      if (user_time)
	XFree(user_time);
    }


  if (changes)
    mb_wm_object_signal_emit (MB_WM_OBJECT (win), changes);

 abort:

  if (xwin_attr)
    XFree(xwin_attr);

  return True;
}

Bool
mb_wm_client_window_is_state_set (MBWMClientWindow *win,
				  MBWMClientWindowEWMHState state)
{
  return (win->ewmh_state & state) ? True : False;
}

