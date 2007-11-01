#include "mb-wm.h"
#include "mb-wm-client-app.h"
#include "mb-wm-client-panel.h"
#include "mb-wm-client-dialog.h"
#include <signal.h>

enum {
  KEY_ACTION_PAGE_NEXT,
  KEY_ACTION_PAGE_PREV,
  KEY_ACTION_TOGGLE_FULLSCREEN,
  KEY_ACTION_TOGGLE_DESKTOP,
};

static MBWindowManager *wm = NULL;

#ifdef MBWM_WANT_DEBUG
/*
 * The Idea behind this is quite simple: when all managed windows are closed
 * and the WM exits, there should have been an unref call for each ref call. To
 * test do something like
 *
 * export DISPLAY=:whatever;
 * export MB_DEBUG=obj-ref,obj-unref
 * matchbox-window-manager-2-simple &
 * gedit
 * kill -TERM $(pidof gedit)
 * kill -TERM $(pidof matchbox-window-manager-2-simple)
 *
 * If you see '=== object count at exit x ===' then we either have a leak
 * (x > 0) or are unrefing a dangling pointer (x < 0).
 */
static void
signal_handler (int sig)
{
  if (sig == SIGTERM)
    {
      int count;

      mb_wm_object_unref (MB_WM_OBJECT (wm));
      mb_wm_object_dump ();

      exit (sig);
    }
}
#endif

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
      mb_wm_cycle_apps (wm, False);
      break;
    case KEY_ACTION_PAGE_PREV:
      mb_wm_cycle_apps (wm, True);
      break;
    case KEY_ACTION_TOGGLE_FULLSCREEN:
      printf(" ### KEY_ACTION_TOGGLE_FULLSCREEN ### \n");
      break;
    case KEY_ACTION_TOGGLE_DESKTOP:
      printf(" ### KEY_ACTION_TOGGLE_DESKTOP ### \n");
      mb_wm_toggle_desktop (wm);
      break;
    }
}

int
main(int argc, char **argv)
{
  MBWMLayout * layout;

#ifdef MBWM_WANT_DEBUG
  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_handler = signal_handler;
  sigaction(SIGTERM, &sa, NULL);
#endif

  mb_wm_object_init();

  wm = mb_wm_new(argc, argv);

  if (wm == NULL)
    mb_wm_util_fatal_error("OOM?");

  layout = mb_wm_layout_new (wm);

  if (layout == NULL)
    mb_wm_util_fatal_error("OOM?");

  mb_wm_set_layout (wm, layout);

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

  mb_wm_object_unref (MB_WM_OBJECT (wm));

#ifdef MBWM_WANT_DEBUG
  mb_wm_object_dump ();
#endif

  return 1;
}
