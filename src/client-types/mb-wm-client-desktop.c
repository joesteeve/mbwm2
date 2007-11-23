#include "mb-wm-client-desktop.h"

#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_EDGE_SIZE 3

static Bool
mb_wm_client_desktop_request_geometry (MBWindowManagerClient *client,
				       MBGeometry            *new_geometry,
				       MBWMClientReqGeomType  flags);

static void
mb_wm_client_desktop_stack (MBWindowManagerClient *client, int flags);

static void
mb_wm_client_desktop_class_init (MBWMObjectClass *klass)
{
  MBWindowManagerClientClass *client;
  MBWMClientDesktopClass * client_desktop;

  MBWM_MARK();

  client     = (MBWindowManagerClientClass *)klass;
  client_desktop = (MBWMClientDesktopClass *)klass;

  client->client_type = MBWMClientTypeDesktop;
  client->geometry = mb_wm_client_desktop_request_geometry;
  client->stack = mb_wm_client_desktop_stack;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMClientDesktop";
#endif
}

static void
mb_wm_client_desktop_destroy (MBWMObject *this)
{
  MBWMClientDesktop * app = MB_WM_CLIENT_DESKTOP (this);

}

static int
mb_wm_client_desktop_init (MBWMObject *this, va_list vap)
{
  MBWMClientDesktop        *client_desktop = MB_WM_CLIENT_DESKTOP (this);
  MBWindowManagerClient    *client         = MB_WM_CLIENT (this);
  MBWMDecor                *decor;
  MBWMDecorButton          *button;
  MBWindowManager          *wm = NULL;
  MBWMClientDesktopClass   *inp_class;

  inp_class = MB_WM_CLIENT_DESKTOP_CLASS (MB_WM_OBJECT_GET_CLASS (this));

#if 0
  /*
   * Property parsing not needed for now, as there are no ClientDesktop specific
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
    return 0;

  client->stacking_layer = MBWMStackLayerBottom;

  mb_wm_client_set_layout_hints (client,
				 LayoutPrefFullscreen|LayoutPrefVisible);

  return 1;
}

int
mb_wm_client_desktop_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMClientDesktopClass),
	sizeof (MBWMClientDesktop),
	mb_wm_client_desktop_init,
	mb_wm_client_desktop_destroy,
	mb_wm_client_desktop_class_init
      };
      type = mb_wm_object_register_class (&info, MB_WM_TYPE_CLIENT_BASE, 0);
    }

  return type;
}

static Bool
mb_wm_client_desktop_request_geometry (MBWindowManagerClient *client,
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
mb_wm_client_desktop_stack (MBWindowManagerClient *client, int flags)
{
  MBWM_MARK();
  mb_wm_stack_move_top(client);
}

MBWindowManagerClient*
mb_wm_client_desktop_new (MBWindowManager *wm, MBWMClientWindow *win)
{
  MBWindowManagerClient *client;

  client = MB_WM_CLIENT(mb_wm_object_new (MB_WM_TYPE_CLIENT_DESKTOP,
					  MBWMObjectPropWm,           wm,
					  MBWMObjectPropClientWindow, win,
					  NULL));

  return client;
}

