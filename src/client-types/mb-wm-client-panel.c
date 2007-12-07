#include "mb-wm-client-panel.h"

static void
mb_wm_client_panel_realize (MBWindowManagerClient *client);

static Bool
mb_wm_client_panel_request_geometry (MBWindowManagerClient *client,
				     MBGeometry            *new_geometry,
				     MBWMClientReqGeomType  flags);

static MBWMStackLayerType
mb_wm_client_panel_stacking_layer (MBWindowManagerClient *client);

static void
mb_wm_client_panel_class_init (MBWMObjectClass *klass)
{
  MBWindowManagerClientClass *client;

  client = (MBWindowManagerClientClass *)klass;

  client->client_type = MBWMClientTypePanel;
  client->realize  = mb_wm_client_panel_realize;
  client->geometry = mb_wm_client_panel_request_geometry;
  client->stacking_layer = mb_wm_client_panel_stacking_layer;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMClientPanel";
#endif
}

static int
mb_wm_client_panel_init (MBWMObject *this, va_list vap)
{
  MBWindowManagerClient * client  = MB_WM_CLIENT (this);
  MBWMClientLayoutHints   hints   = LayoutPrefVisible;
  MBGeometry            * win_geo = &client->window->geometry;

  client->stacking_layer = MBWMStackLayerBottomMid;

  if (win_geo->width > win_geo->height)
    if (win_geo->y < (client->wmref->xdpy_height/2))
      hints |= LayoutPrefReserveEdgeNorth;
    else
      hints |= LayoutPrefReserveEdgeSouth;
  else
    if (win_geo->x < (client->wmref->xdpy_width/2))
      hints |= LayoutPrefReserveEdgeWest;
    else
      hints |= LayoutPrefReserveEdgeEast;

  mb_wm_client_set_layout_hints (client, hints);

  return 1;
}

static void
mb_wm_client_panel_destroy (MBWMObject *this)
{
}

int
mb_wm_client_panel_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMClientPanelClass),
	sizeof (MBWMClientPanel),
	mb_wm_client_panel_init,
	mb_wm_client_panel_destroy,
	mb_wm_client_panel_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_CLIENT_BASE, 0);
    }

  return type;
}

static void
mb_wm_client_panel_realize (MBWindowManagerClient *client)
{
  /* Just skip creating frame... */
  return;
}

static Bool
mb_wm_client_panel_request_geometry (MBWindowManagerClient *client,
				     MBGeometry            *new_geometry,
				     MBWMClientReqGeomType  flags)
{
  if (client->window->geometry.x != new_geometry->x
      || client->window->geometry.y != new_geometry->y
      || client->window->geometry.width  != new_geometry->width
      || client->window->geometry.height != new_geometry->height)
    {
      client->window->geometry.x = new_geometry->x;
      client->window->geometry.y = new_geometry->y;
      client->window->geometry.width  = new_geometry->width;
      client->window->geometry.height = new_geometry->height;

      mb_wm_client_geometry_mark_dirty (client);
    }

  return True;
}

static MBWMStackLayerType
mb_wm_client_panel_stacking_layer (MBWindowManagerClient *client)
{
  /*
   * If we are showing desktop, ensure that we stack above it.
   */
  if (client->wmref->flags & MBWindowManagerFlagDesktop)
    return MBWMStackLayerTopMid;

  return client->stacking_layer;
}

MBWindowManagerClient*
mb_wm_client_panel_new(MBWindowManager *wm, MBWMClientWindow *win)
{
  MBWindowManagerClient *client = NULL;

  client = MB_WM_CLIENT(mb_wm_object_new (MB_WM_TYPE_CLIENT_PANEL,
					  MBWMObjectPropWm,           wm,
					  MBWMObjectPropClientWindow, win,
					  NULL));

  return client;
}


