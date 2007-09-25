#include "mb-wm-client-input.h"

#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_EDGE_SIZE 3

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

  client->client_type = MBWMClientTypeInput;
  client->geometry = mb_wm_client_input_request_geometry;
  client->stack = mb_wm_client_input_stack;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMClientInput";
#endif
}

static void
mb_wm_client_input_destroy (MBWMObject *this)
{
  MBWMClientInput * app = MB_WM_CLIENT_INPUT (this);

}

static void
mb_wm_client_input_init (MBWMObject *this, va_list vap)
{
  MBWMClientInput          *client_input = MB_WM_CLIENT_INPUT (this);
  MBWindowManagerClient    *client       = MB_WM_CLIENT (this);
  MBWMDecor                *decor;
  MBWMDecorButton          *button;
  MBWindowManager          *wm = NULL;
  MBWMClientInputClass     *inp_class;

  inp_class = MB_WM_CLIENT_INPUT_CLASS (MB_WM_OBJECT_GET_CLASS (this));

#if 0
  /*
   * Property parsing not needed for now, as there are no ClientInput specific
   * properties
   */
  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      if (prop == MBWMObjectPropWm)
	{
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	}
      else
	MBWMO_PROP_EAT (vap, prop);

      prop = va_arg (vap, MBWMObjectProp);
    }
#endif

  wm = client->wmref;

  if (!wm)
    return;

  client->stacking_layer = MBWMStackLayerMid;

  mb_wm_client_set_layout_hints (client,
				 LayoutPrefReserveSouth|LayoutPrefVisible);
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

