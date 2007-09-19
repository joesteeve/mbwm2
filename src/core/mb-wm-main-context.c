#include "mb-wm-main-context.h"

#ifdef MBWM_WANT_DEBUG

static const char *MBWMDEBUGEvents[] = {
    "error",
    "reply",
    "KeyPress",
    "KeyRelease",
    "ButtonPress",
    "ButtonRelease",
    "MotionNotify",
    "EnterNotify",
    "LeaveNotify",
    "FocusIn",
    "FocusOut",
    "KeymapNotify",
    "Expose",
    "GraphicsExpose",
    "NoExpose",
    "VisibilityNotify",
    "CreateNotify",
    "DestroyNotify",
    "UnmapNotify",
    "MapNotify",
    "MapRequest",
    "ReparentNotify",
    "ConfigureNotify",
    "ConfigureRequest",
    "GravityNotify",
    "ResizeRequest",
    "CirculateNotify",
    "CirculateRequest",
    "PropertyNotify",
    "SelectionClear",
    "SelectionRequest",
    "SelectionNotify",
    "ColormapNotify",
    "ClientMessage",
    "MappingNotify",
};

#endif


static void
mb_wm_main_context_class_init (MBWMObjectClass *klass)
{
}

static void
mb_wm_main_context_destroy (MBWMObject *this)
{
}

static void
mb_wm_main_context_init (MBWMObject *this, va_list vap)
{
  MBWMMainContext  *ctx = MB_WM_MAIN_CONTEXT (this);
  MBWindowManager  *wm = NULL;
  MBWMObjectProp    prop;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      if (prop == MBWMObjectPropWm)
	{
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	}
      else
	MBWMO_PROP_EAT (vap, prop);

      prop = va_arg (vap, MBWMObjectProp);
    }

  ctx->wm = wm;
}

int
mb_wm_main_context_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMMainContextClass),
	sizeof (MBWMMainContext),
	mb_wm_main_context_init,
	mb_wm_main_context_destroy,
	mb_wm_main_context_class_init
      };
      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

MBWMMainContext*
mb_wm_main_context_new (MBWindowManager *wm)
{
  MBWMMainContext *ctx;

  ctx = MB_WM_MAIN_CONTEXT (mb_wm_object_new (MB_WM_TYPE_MAIN_CONTEXT,
					      MBWMObjectPropWm, wm,
					      NULL));

  return ctx;
}

static Bool
mb_wm_main_context_handle_x_event (XEvent          *xev,
				   MBWMMainContext *ctx)
{
  MBWindowManager *wm = ctx->wm;
  MBWMEventFuncs  *xev_funcs = &ctx->event_funcs;
  MBWMList        *iter;

#if (MBWM_WANT_DEBUG)
 {
   MBWindowManagerClient *ev_client;

   ev_client = mb_wm_managed_client_from_xwindow(wm, xev->xany.window);

   MBWM_DBG("@ XEvent: '%s:%i' for %lx %s%s",
	    MBWMDEBUGEvents[xev->type],
	    xev->type,
	    xev->xany.window,
	    xev->xany.window == wm->root_win->xwindow ? "(root)" : "",
	    ev_client ? ev_client->name : ""
	    );
 }
#endif

#define XE_ITER_GET_FUNC(i) (((MBWMXEventFuncInfo *)((i)->data))->func)
#define XE_ITER_GET_DATA(i) ((MBWMXEventFuncInfo *)((i)->data))->userdata

  switch (xev->type)
    {
    case Expose:
      break;
    case MapRequest:
      iter = ctx->event_funcs.map_request;

      while (iter &&
	     ((MBWindowManagerMapRequestFunc)XE_ITER_GET_FUNC(iter))
	     (wm, (XMapRequestEvent*)&xev->xmaprequest, XE_ITER_GET_DATA(iter)))
	iter = iter->next;

      break;
    case MapNotify:
      iter = ctx->event_funcs.map_notify;

      while (iter &&
	     ((MBWindowManagerMapNotifyFunc)XE_ITER_GET_FUNC(iter))
	     (wm, (XMapEvent*)&xev->xmap, XE_ITER_GET_DATA(iter)))
	iter = iter->next;

      break;
    case UnmapNotify:
      iter = ctx->event_funcs.unmap_notify;

      while (iter &&
	     ((MBWindowManagerUnmapNotifyFunc)XE_ITER_GET_FUNC(iter))
	     (wm, (XUnmapEvent*)&xev->xunmap, XE_ITER_GET_DATA(iter)))
	iter = iter->next;

      break;
    case DestroyNotify:
      iter = ctx->event_funcs.destroy_notify;

      while (iter &&
	     ((MBWindowManagerDestroyNotifyFunc)XE_ITER_GET_FUNC(iter))
	     (wm, (XDestroyWindowEvent*)&xev->xdestroywindow, XE_ITER_GET_DATA(iter)))
	iter = iter->next;
      break;
    case ConfigureNotify:
      iter = ctx->event_funcs.configure_notify;

      while (iter &&
	  ((MBWindowManagerConfigureNotifyFunc)XE_ITER_GET_FUNC(iter))
	     (wm, (XConfigureEvent*)&xev->xconfigure, XE_ITER_GET_DATA(iter)))
	iter = iter->next;

      break;
    case ConfigureRequest:
      iter = ctx->event_funcs.configure_request;

      while (iter &&
	     ((MBWindowManagerConfigureRequestFunc)XE_ITER_GET_FUNC(iter))
	     (wm, (XConfigureRequestEvent*)&xev->xconfigurerequest, XE_ITER_GET_DATA(iter)))
	iter = iter->next;

      break;
    case KeyPress:
      iter = ctx->event_funcs.key_press;

      while (iter &&
	     ((MBWindowManagerKeyPressFunc)XE_ITER_GET_FUNC(iter))
	     (wm, (XKeyEvent*)&xev->xkey, XE_ITER_GET_DATA(iter)))
	iter = iter->next;

      break;
    case PropertyNotify:
      iter = ctx->event_funcs.property_notify;

      while (iter &&
	     ((MBWindowManagerPropertyNotifyFunc)XE_ITER_GET_FUNC(iter))
	     (wm, (XPropertyEvent*)&xev->xproperty, XE_ITER_GET_DATA(iter)))
	iter = iter->next;

      break;
    case ButtonPress:
      iter = ctx->event_funcs.button_press;

      while (iter &&
	     ((MBWindowManagerButtonPressFunc)XE_ITER_GET_FUNC(iter))
	     (wm, (XButtonEvent*)&xev->xbutton, XE_ITER_GET_DATA(iter)))
	iter = iter->next;
      break;
    case ButtonRelease:
      iter = ctx->event_funcs.button_release;

      while (iter &&
	     ((MBWindowManagerButtonReleaseFunc)XE_ITER_GET_FUNC(iter))
	     (wm, (XButtonEvent*)&xev->xbutton, XE_ITER_GET_DATA(iter)))
	iter = iter->next;
      break;

    }

  if (wm->need_display_sync)
    mb_wm_sync (wm);

  return False;
}

Bool
mb_wm_main_context_spin (MBWMMainContext *ctx)
{
  MBWindowManager * wm = ctx->wm;
  XEvent xev;

  if (!XEventsQueued (wm->xdpy, QueuedAfterReading))
    return False;

  XNextEvent(wm->xdpy, &xev);

  mb_wm_main_context_handle_x_event (&xev, ctx);

  return (XEventsQueued (wm->xdpy, QueuedAfterReading) != 0);
}

void
mb_wm_main_context_loop(MBWMMainContext *ctx)
{
  MBWindowManager * wm = ctx->wm;
  XEvent xev;

  while (True)
    {
      XNextEvent(wm->xdpy, &xev);

      mb_wm_main_context_handle_x_event (&xev, ctx);
    }
}

unsigned long
mb_wm_main_context_x_event_handler_add (MBWMMainContext *ctx,
					int              type,
					MBWMXEventFunc   func,
					void            *userdata)
{
  static unsigned long ids = 0;
  MBWMXEventFuncInfo *func_info;

  ++ids;

  func_info           = mb_wm_util_malloc0(sizeof(MBWMXEventFuncInfo));
  func_info->func     = func;
  func_info->userdata = userdata;
  func_info->id       = ids;

  switch (type)
    {
    case Expose:
      break;
    case MapRequest:
      ctx->event_funcs.map_request =
	mb_wm_util_list_append (ctx->event_funcs.map_request, func_info);
      break;
    case MapNotify:
      ctx->event_funcs.map_notify=
	mb_wm_util_list_append (ctx->event_funcs.map_notify, func_info);
      break;
    case UnmapNotify:
      ctx->event_funcs.unmap_notify=
	mb_wm_util_list_append (ctx->event_funcs.unmap_notify, func_info);
      break;
    case DestroyNotify:
      ctx->event_funcs.destroy_notify =
	mb_wm_util_list_append (ctx->event_funcs.destroy_notify, func_info);
      break;
    case ConfigureNotify:
      ctx->event_funcs.configure_notify =
	mb_wm_util_list_append (ctx->event_funcs.configure_notify, func_info);
      break;
    case ConfigureRequest:
      ctx->event_funcs.configure_request =
	mb_wm_util_list_append (ctx->event_funcs.configure_request, func_info);
      break;
    case KeyPress:
      ctx->event_funcs.key_press =
	mb_wm_util_list_append (ctx->event_funcs.key_press, func_info);
      break;
    case PropertyNotify:
      ctx->event_funcs.property_notify =
	mb_wm_util_list_append (ctx->event_funcs.property_notify, func_info);
      break;
    case ButtonPress:
      ctx->event_funcs.button_press =
	mb_wm_util_list_append (ctx->event_funcs.button_press, func_info);
      break;
    case ButtonRelease:
      ctx->event_funcs.button_release =
	mb_wm_util_list_append (ctx->event_funcs.button_release, func_info);
      break;

    default:
      break;
    }

  return ids;
}

void
mb_wm_main_context_x_event_handler_remove (MBWMMainContext *ctx,
					   int              type,
					   unsigned long    id)
{
  MBWMList *l = NULL;
  MBWMList **l_start;

  switch (type)
    {
    case Expose:
      break;
    case MapRequest:
      l_start = &ctx->event_funcs.map_request;
      break;
    case MapNotify:
      l_start = &ctx->event_funcs.map_notify;
      break;
    case UnmapNotify:
      l_start = &ctx->event_funcs.unmap_notify;
      break;
    case DestroyNotify:
      l_start = &ctx->event_funcs.destroy_notify;
      break;
    case ConfigureNotify:
      l_start = &ctx->event_funcs.configure_notify;
      break;
    case ConfigureRequest:
      l_start = &ctx->event_funcs.configure_request;
      break;
    case KeyPress:
      l_start = &ctx->event_funcs.key_press;
      break;
    case PropertyNotify:
      l_start = &ctx->event_funcs.property_notify;
      break;
    case ButtonPress:
      l_start = &ctx->event_funcs.button_press;
      break;
    case ButtonRelease:
      l_start = &ctx->event_funcs.button_release;
      break;

    default:
      break;
    }

  l = *l_start;

  while (l)
    {
      MBWMXEventFuncInfo * info = l->data;

      if (info->id == id)
	{
	  MBWMList * prev = l->prev;
	  MBWMList * next = l->next;

	  if (prev)
	    prev->next = next;
	  else
	    *l_start = next;

	  if (next)
	    next->prev = prev;

	  free (info);
	  free (l);

	  return;
	}

      l = l->next;
    }
}

unsigned long
mb_wm_context_timeout_handler_add (MBWMMainContext            *ctx,
				   int                         ms,
				   MBWindowManagerTimeOutFunc  func,
				   void                       *userdata)
{
  static unsigned long ids = 0;
  MBWMTimeOutEventInfo * tinfo;

  ++ids;

  tinfo = mb_wm_util_malloc0 (sizeof (MBWMTimeOutEventInfo));
  tinfo->func = func;
  tinfo->id = ids;
  tinfo->ms = ms;
  tinfo->userdata = userdata;

  ctx->event_funcs.timeout =
    mb_wm_util_list_append (ctx->event_funcs.timeout, tinfo);

  return ids;
}

void
mb_wm_main_context_timeout_handler_remove (MBWMMainContext *ctx,
					   unsigned long    id)
{
  MBWMList * l = ctx->event_funcs.timeout;

  while (l)
    {
        MBWMTimeOutEventInfo * info = l->data;

      if (info->id == id)
	{
	  MBWMList * prev = l->prev;
	  MBWMList * next = l->next;

	  if (prev)
	    prev->next = next;
	  else
	    ctx->event_funcs.timeout = next;

	  if (next)
	    next->prev = prev;

	  free (info);
	  free (l);

	  return;
	}

      l = l->next;
    }
}

unsigned long
mb_wm_main_context_fd_watch_add (MBWMMainContext           *ctx,
				 MBWindowManagerFdWatchFunc func)
{
  static unsigned long ids = 0;

  ++ids;

  return ids;
}

void
mb_wm_main_context_fd_watch_remove (MBWMMainContext *ctx,
				    unsigned long    id)
{

}

