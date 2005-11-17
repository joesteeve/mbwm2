#include "mb-wm-client-panel.h"

struct MBWindowManagerClientPanel
{
  MBWindowManagerClient base_client;
};

static int MBWMClientPanelType = 0;

void
mb_wm_client_panel_register_type (MBWindowManager *wm)
{
  if (!MBWMClientPanelType)
    MBWMClientPanelType = mb_wm_register_client_type(wm);
}

int
mb_wm_client_panel_get_type ()
{
  return MBWMClientPanelType;
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


MBWindowManagerClient*
mb_wm_client_panel_new(MBWindowManager *wm, MBWMWindow *win)
{
  MBWindowManagerClientPanel *pc = NULL;

  if (!TestPanelClientType)
    return NULL;

  pc = mb_wm_util_malloc0(sizeof(TestPanelClient));

  mb_wm_client_init (wm, MBWM_CLIENT(pc), win);

  pc->base_client.type = TestPanelClientType;

  /* overide realize method */
  pc->base_client.realize  = mb_wm_client_panel_realize;
  pc->base_client.geometry = mb_wm_client_panel_request_geometry; 

  mb_wm_client_set_layout_hints (MBWM_CLIENT(pc),
				 LayoutPrefReserveEdgeSouth|LayoutPrefVisible);

  return MBWM_CLIENT(pc);
}


