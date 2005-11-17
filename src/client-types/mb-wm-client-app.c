#include "mb-wm-client-app.h"

struct MBWindowManagerClientApp
{
  MBWindowManagerClient base_client;
};

static int MBWMClientAppType = 0;

void
mb_wm_client_app_register_type (MBWindowManager *wm)
{
  if (!MBWMClientAppType)
    MBWMClientAppType = mb_wm_register_client_type(wm);
}

int
mb_wm_client_app_get_type ()
{
  return MBWMClientAppType;
}


MBWindowManagerClient*
mb_wm_client_app_new (MBWindowManager *wm, MBWMWindow *win)
{
  MBWindowManagerClientApp *tc = NULL;

  if (!MBWMClientAppType)
    return NULL;

  tc = mb_wm_util_malloc0(sizeof(MBWindowManagerClientApp));

  mb_wm_client_init (wm, MBWM_CLIENT(tc), win);

  tc->base_client.type = MBWMClientAppType;

  mb_wm_client_set_layout_hints (MBWM_CLIENT(tc),
				 LayoutPrefGrowToFreeSpace|LayoutPrefVisible);

  return MBWM_CLIENT(tc);
}

