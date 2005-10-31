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


MBWindowManagerClient*
test_client_new(MBWindowManager *wm, MBWindowManagerClientWindow *win)
{
  TestClient *tc = NULL;

  tc = mb_wm_util_malloc0(sizeof(TestClient));

  mb_wm_client_init (wm, MBWM_CLIENT(tc), win);

  tc->base_client.type = TestClientType;
  tc->base_client.layout_hints = LayoutPrefGrowToFreeSpace|LayoutPrefVisible;

  return MBWM_CLIENT(tc);
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

  wm->new_client_from_window_func = test_client_new;

  mb_wm_run(wm);

  return 1;
}
