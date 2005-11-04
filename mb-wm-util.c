#include "mb-wm.h"

static int TrappedErrorCode = 0;
static int (*old_error_handler) (Display *, XErrorEvent *);

static int
error_handler(Display     *xdpy,
	      XErrorEvent *error)
{
  TrappedErrorCode = error->error_code;
  return 0;
}

void
mb_wm_util_trap_x_errors(void)
{
  MBWM_DBG("### X Errors Trapped ###");

  TrappedErrorCode  = 0;
  old_error_handler = XSetErrorHandler(error_handler);
}

int
mb_wm_util_untrap_x_errors(void)
{
  MBWM_DBG("### X Errors Untrapped (%i) ###", TrappedErrorCode);

  XSetErrorHandler(old_error_handler);
  return TrappedErrorCode;
}


void*
mb_wm_util_malloc0(int size)
{
  void *p = NULL;

  p = malloc(size);

  if (p == NULL)
    {
      /* hook into some kind of out of memory */
    }

  memset(p, 0, size);

  return p;
}

void
mb_wm_util_fatal_error(char *msg)
{
  fprintf(stderr, "matchbox-window-manager: *Error*  %s\n", msg);
  exit(1);
}

MBList*
mb_wm_util_list_alloc_item(void)
{
  return mb_wm_util_malloc0(sizeof(MBList));
}

int
mb_wm_util_list_length(MBList *list)
{
  int result = 1;

  list = mb_wm_util_list_get_first(list);

  while ((list = mb_wm_util_list_next(list)) != NULL)
    result++;

  return result;
}

MBList*
mb_wm_util_list_get_last(MBList *list)
{
  if (list == NULL) 
    return NULL;

  while (list->next) 
    list = mb_wm_util_list_next(list);
  return list;
}

MBList*
mb_wm_util_list_get_first(MBList *list)
{
  if (list == NULL) 
    return NULL;

  while (list->prev) 
    list = mb_wm_util_list_prev(list);
  return list;
}

void*
mb_wm_util_list_get_nth_data(MBList *list, int n)
{
  if (list == NULL) 
    return NULL;

  list = mb_wm_util_list_get_first(list);

  while (list->next && n)
    {
      list = mb_wm_util_list_next(list);
      n--;
    }

  if (n) return NULL;

  return (void *)list->data;
}



MBList*
mb_wm_util_list_append(MBList *list, void *data)
{
  if (list == NULL)
    {
      list = mb_wm_util_list_alloc_item();
      list->data = data;
    }
  else
    {
      list = mb_wm_util_list_get_last(list);

      list->next = mb_wm_util_list_alloc_item();
      list->next->prev = list;
      list->next->data = data;
    }

  return list;
}

void
mb_wm_util_list_foreach(MBList *list, MBListForEachCB func, void *userdata)
{
  while (list)
    {
      func(list->data, userdata);
      list = mb_wm_util_list_next(list);
    }
}
