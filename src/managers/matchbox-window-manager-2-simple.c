#include "mb-wm.h"
#include "mb-wm-client-app.h"
#include "mb-wm-client-panel.h"
#include "mb-wm-client-dialog.h"

enum {
  KEY_ACTION_PAGE_NEXT,
  KEY_ACTION_PAGE_PREV,
  KEY_ACTION_TOGGLE_FULLSCREEN,
  KEY_ACTION_TOGGLE_DESKTOP,
};

void
key_binding_func (MBWindowManager   *wm,
		  MBWMKeyBinding    *binding,
		  void              *userdata)
{
  printf(" ### got key press ### \n");
  int action;

  action = (int)(userdata);

  switch (action)
    {
    case KEY_ACTION_PAGE_NEXT:
      {
	mb_wm_stack_cycle_by_type(wm, MB_WM_TYPE_CLIENT_APP );
	mb_wm_display_sync_queue (wm);

      }
      break;
    case KEY_ACTION_PAGE_PREV:
      printf(" ### KEY_ACTION_PAGE_PREV ### \n");
      break;
    case KEY_ACTION_TOGGLE_FULLSCREEN:
      printf(" ### KEY_ACTION_TOGGLE_FULLSCREEN ### \n");
      break;
    case KEY_ACTION_TOGGLE_DESKTOP:
      printf(" ### KEY_ACTION_TOGGLE_DESKTOP ### \n");
      break;
    }
}

int
main(int argc, char **argv)
{
  MBWindowManager     *wm;
  char * test[2] = 
    {
      "test1", "test2"
    };
  
  mb_wm_object_init();

  wm = mb_wm_new(2, (char**)&test[0]);

  if (wm == NULL)
    mb_wm_util_fatal_error("OOM?");

  mb_wm_theme_init (wm);

  mb_wm_manage_preexistsing_wins (wm);

  mb_wm_keys_binding_add_with_spec (wm,
				    "<alt>d",
				    key_binding_func,
				    NULL,
				    (void*)KEY_ACTION_TOGGLE_DESKTOP);

  mb_wm_keys_binding_add_with_spec (wm,
				    "<alt>n",
				    key_binding_func,
				    NULL,
				    (void*)KEY_ACTION_PAGE_NEXT);

  mb_wm_keys_binding_add_with_spec (wm,
				    "<alt>p",
				    key_binding_func,
				    NULL,
				    (void*)KEY_ACTION_PAGE_PREV);

  mb_wm_main_loop(wm);

  return 1;
}
