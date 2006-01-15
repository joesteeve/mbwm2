#include "mb-wm-client-app.h"

struct MBWindowManagerClientApp
{
  MBWindowManagerClient base_client;
};

int
mb_wm_client_app_get_type ()
{
  static int type = 0;

  printf("type: %i\n", type);

  if (type == 0)
    type = mb_wm_register_client_type();

  return type;
}

MBWindowManagerClient*
mb_wm_client_app_new (MBWindowManager *wm, MBWMWindow *win)
{
  MBWindowManagerClientApp *tc = NULL;

  tc = mb_wm_util_malloc0(sizeof(MBWindowManagerClientApp));

  mb_wm_client_init (wm, MBWM_CLIENT(tc), win);

  tc->base_client.type = mb_wm_client_app_get_type ();

  fprintf(stderr, "XXX tc->base_client.type:%i\n", tc->base_client.type);

  mb_wm_client_set_layout_hints (MBWM_CLIENT(tc),
				 LayoutPrefGrowToFreeSpace|LayoutPrefVisible);

  return MBWM_CLIENT(tc);
}

