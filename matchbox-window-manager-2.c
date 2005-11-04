/* 
 * Here we would provide wm policy
 *
 */

#include "mb-wm.h"

typedef struct TestClient TestClient;
typedef struct TestPanelClient TestPanelClient;

int TestClientType, TestPanelClientType;

struct TestClient
{
  MBWindowManagerClient base_client;
};

struct TestPanelClient
{
  MBWindowManagerClient base_client;
};

static void
test_panel_client_realize (MBWindowManagerClient *client)
{
  /* Just skip creating frame... */
  return;
}

Bool
test_panel_client_request_geometry (MBWindowManagerClient *client,
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
test_panel_client_new(MBWindowManager *wm, MBWindowManagerClientWindow *win)
{
  TestPanelClient *pc = NULL;

  pc = mb_wm_util_malloc0(sizeof(TestPanelClient));

  mb_wm_client_init (wm, MBWM_CLIENT(pc), win);

  pc->base_client.type = TestPanelClientType;

  /* overide realize method */
  pc->base_client.realize  = test_panel_client_realize;
  pc->base_client.geometry = test_panel_client_request_geometry; 

  mb_wm_client_set_layout_hints (MBWM_CLIENT(pc),
				 LayoutPrefReserveEdgeSouth|LayoutPrefVisible);

  return MBWM_CLIENT(pc);
}


MBWindowManagerClient*
test_client_new(MBWindowManager *wm, MBWindowManagerClientWindow *win)
{
  TestClient *tc = NULL;

  tc = mb_wm_util_malloc0(sizeof(TestClient));

  mb_wm_client_init (wm, MBWM_CLIENT(tc), win);

  tc->base_client.type = TestClientType;

  mb_wm_client_set_layout_hints (MBWM_CLIENT(tc),
				 LayoutPrefGrowToFreeSpace|LayoutPrefVisible);

  return MBWM_CLIENT(tc);
}

MBWindowManagerClient*
client_new(MBWindowManager *wm, MBWindowManagerClientWindow *win)
{
  if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DOCK])
    return test_panel_client_new(wm, win);
  else
    return test_client_new(wm, win);
}

int 
main(int argc, char **argv)
{
  MBWindowManager     *wm;

  wm = mb_wm_new();

  if (wm == NULL)
    mb_wm_util_fatal_error("OOM?");

  TestClientType      = mb_wm_register_client_type(wm);
  TestPanelClientType = mb_wm_register_client_type(wm);

  mb_wm_init(wm, NULL, NULL);

  wm->new_client_from_window_func = client_new;

  mb_wm_run(wm);

  return 1;
}
