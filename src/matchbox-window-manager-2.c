/* 
 * Here we would provide wm policy
 *
 */

#include "mb-wm.h"
#include "mb-wm-client-app.h"
#include "mb-wm-client-panel.h"

MBWindowManagerClient*
client_new(MBWindowManager *wm, MBWMWindow *win)
{
  if (win->net_type == wm->atoms[MBWM_ATOM_NET_WM_WINDOW_TYPE_DOCK])
    return test_panel_client_new(wm, win);
  else
    return test_client_new(wm, win);
}

void
test_key_func (MBWindowManager   *wm,
	       MBWMKeyBinding    *binding,
	       void              *userdata)
{
  printf(" ### got key press ### \n");
}

int 
main(int argc, char **argv)
{
  MBWindowManager     *wm;

  wm = mb_wm_new();

  if (wm == NULL)
    mb_wm_util_fatal_error("OOM?");

  mb_wm_client_panel_register_type(wm);
  mb_wm_client_app_register_type(wm);

  mb_wm_init(wm, NULL, NULL);

  wm->new_client_from_window_func = client_new;

  mb_wm_keys_binding_add_with_spec (wm,
				    "<alt>d",
				    test_key_func,
				    NULL,
				    NULL);

  mb_wm_run(wm);

  return 1;
}
