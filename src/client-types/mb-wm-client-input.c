#include "mb-wm-client-input.h"

#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_EDGE_SIZE 3

static void
mb_wm_client_input_detransitise (MBWindowManagerClient *client);

static void
mb_wm_client_input_realize (MBWindowManagerClient *client);

static Bool
mb_wm_client_input_request_geometry (MBWindowManagerClient *client,
				     MBGeometry            *new_geometry,
				     MBWMClientReqGeomType  flags);

static void
mb_wm_client_input_stack (MBWindowManagerClient *client, int flags);

static void
mb_wm_client_input_class_init (MBWMObjectClass *klass)
{
  MBWindowManagerClientClass *client;
  MBWMClientInputClass * client_input;

  MBWM_MARK();

  client     = (MBWindowManagerClientClass *)klass;
  client_input = (MBWMClientInputClass *)klass;

  MBWM_DBG("client->stack is %p", client->stack);

  client->client_type  = MBWMClientTypeInput;
  client->geometry     = mb_wm_client_input_request_geometry;
  client->stack        = mb_wm_client_input_stack;
  client->realize      = mb_wm_client_input_realize;
  client->detransitise = mb_wm_client_input_detransitise;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMClientInput";
#endif
}

static void
mb_wm_client_input_destroy (MBWMObject *this)
{
  MBWMClientInput * app = MB_WM_CLIENT_INPUT (this);

}

static int
mb_wm_client_input_init (MBWMObject *this, va_list vap)
{
  MBWMClientInput          *client_input = MB_WM_CLIENT_INPUT (this);
  MBWindowManagerClient    *client       = MB_WM_CLIENT (this);
  MBWindowManager          *wm = client->wmref;
  MBWMClientWindow         *win = client->window;

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
      MBWM_DBG ("Input is transient to root");
      /* Stack with 'always on top' */
      client->stacking_layer = MBWMStackLayerTopMid;
    }

  mb_wm_client_set_layout_hints (client,
				 LayoutPrefReserveSouth|LayoutPrefVisible);

  return 1;
}

int
mb_wm_client_input_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMClientInputClass),
	sizeof (MBWMClientInput),
	mb_wm_client_input_init,
	mb_wm_client_input_destroy,
	mb_wm_client_input_class_init
      };
      type = mb_wm_object_register_class (&info, MB_WM_TYPE_CLIENT_BASE, 0);
    }

  return type;
}

static void
mb_wm_client_input_realize (MBWindowManagerClient *client)
{
  /* Just skip creating frame... */
  return;
}

static Bool
mb_wm_client_input_request_geometry (MBWindowManagerClient *client,
				     MBGeometry            *new_geometry,
				     MBWMClientReqGeomType  flags)
{
  if (flags & MBWMClientReqGeomIsViaLayoutManager)
    {
      client->frame_geometry.x      = new_geometry->x;
      client->frame_geometry.y      = new_geometry->y;
      client->frame_geometry.width  = new_geometry->width;
      client->frame_geometry.height = new_geometry->height;

      client->window->geometry.x      = client->frame_geometry.x;
      client->window->geometry.y      = client->frame_geometry.y;
      client->window->geometry.width  = client->frame_geometry.width;
      client->window->geometry.height = client->frame_geometry.height;

      mb_wm_client_geometry_mark_dirty (client);

      return True; /* Geometry accepted */
    }
}

static void
mb_wm_client_input_stack (MBWindowManagerClient *client, int flags)
{
  MBWM_MARK();

  mb_wm_stack_move_client_above_type (client,
				     MBWMClientTypeApp|MBWMClientTypeDesktop);
}

MBWindowManagerClient*
mb_wm_client_input_new (MBWindowManager *wm, MBWMClientWindow *win)
{
  MBWindowManagerClient *client;

  client = MB_WM_CLIENT(mb_wm_object_new (MB_WM_TYPE_CLIENT_INPUT,
					  MBWMObjectPropWm,           wm,
					  MBWMObjectPropClientWindow, win,
					  NULL));

  return client;
}

static void
mb_wm_client_input_detransitise (MBWindowManagerClient *client)
{
  MBWMClientInput * input = MB_WM_CLIENT_INPUT (client);

  if (!client->transient_for)
    return;

  if (input->transient_geom.width && input->transient_geom.height)
    {
      mb_wm_client_request_geometry (client->transient_for,
				     &input->transient_geom,
				     MBWMClientReqGeomForced);
    }
}

