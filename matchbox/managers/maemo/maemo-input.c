#include "maemo-input.h"

static void
maemo_input_class_init (MBWMObjectClass *klass)
{
  MBWindowManagerClientClass *client;
  MBWMClientInputClass * client_input;

  MBWM_MARK();

  client     = (MBWindowManagerClientClass *)klass;
  client_input = (MBWMClientInputClass *)klass;

  MBWM_DBG("client->stack is %p", client->stack);

  client->client_type = MBWMClientTypeInput;

#if MBWM_WANT_DEBUG
  klass->klass_name = "MaemoInput";
#endif
}

static void
maemo_input_destroy (MBWMObject *this)
{
}

static int
maemo_input_init (MBWMObject *this, va_list vap)
{
  MBWindowManagerClient  *client       = MB_WM_CLIENT (this);
  MBWMClientInput        *client_input = MB_WM_CLIENT_INPUT (this);
  MBWindowManager        *wm           = client->wmref;
  MBWMClientWindow       *win          = client->window;

  /* Maemo input windows must always be transient for something; however, for
   * now, for purposes of testing, we will simulate this by this little hack.
   */
#if 1
  if (!win->xwin_transient_for)
    {
      MBWindowManagerClient * c;
      MBWindowManagerClient * t;

      if ((c = mb_wm_stack_get_highest_by_type (wm, MBWMClientTypeApp)))
	{
	  Window w;
	  MBWMList *trans = mb_wm_client_get_transients (c);

	  if (trans && (t = trans->data))
	    w = t->window->xwindow;
	  else
	    w = c->window->xwindow;

	  win->xwin_transient_for = w;

	  XSetTransientForHint(wm->xdpy, win->xwindow, w);

	  mb_wm_util_list_free (trans);
	}
    }
#endif

  if (win->xwin_transient_for
      && win->xwin_transient_for != win->xwindow
      && win->xwin_transient_for != wm->root_win->xwindow)
    {
      MBWindowManagerClient * t =
	mb_wm_managed_client_from_xwindow (wm,
					   win->xwin_transient_for);

      mb_wm_client_get_coverage (t, & client_input->transient_geom);

      MBWM_DBG ("Adding to '%lx' transient list", win->xwin_transient_for);
      mb_wm_client_add_transient (t, client);

      client->stacking_layer = 0;  /* We stack with whatever transient too */
    }
  else
    {
      MBWM_DBG ("Maemo input windows must have transient attribute !!!");
      return 0;
    }

  mb_wm_client_set_layout_hints (client,
				 LayoutPrefReserveSouth|LayoutPrefVisible);

  return 1;
}

int
maemo_input_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMClientInputClass),
	sizeof (MBWMClientInput),
	maemo_input_init,
	maemo_input_destroy,
	maemo_input_class_init
      };
      type = mb_wm_object_register_class (&info, MB_WM_TYPE_CLIENT_INPUT, 0);
    }

  return type;
}

MBWindowManagerClient*
maemo_input_new (MBWindowManager *wm, MBWMClientWindow *win)
{
  MBWindowManagerClient *client;

  client = MB_WM_CLIENT (mb_wm_object_new (MAEMO_TYPE_INPUT,
					   MBWMObjectPropWm,           wm,
					   MBWMObjectPropClientWindow, win,
					   NULL));

  return client;
}

