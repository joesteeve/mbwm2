#include "mb-wm-client-dialog.h"

#include "mb-wm-theme.h"

#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_EDGE_SIZE 3

static Bool
mb_wm_client_dialog_request_geometry (MBWindowManagerClient *client,
				      MBGeometry            *new_geometry,
				      MBWMClientReqGeomType  flags);

static void
mb_wm_client_dialog_stack (MBWindowManagerClient *client,
			   int                    flags);

static void
mb_wm_client_dialog_show (MBWindowManagerClient *client)
{
  MBWindowManagerClientClass  *parent_klass;

  parent_klass =
    MB_WM_CLIENT_CLASS(MB_WM_OBJECT_GET_PARENT_CLASS(client));

  if (client->transient_for != NULL)
    {
      MBWindowManager * wm = client->wmref;

      /*
       * If an attempt has been made to activate a hidden
       * dialog, activate its parent app first.
       *
       * Note this is mainly to work with some task selectors
       * ( eg the gnome one, which activates top dialog ).
       */
      MBWindowManagerClient *parent = client->transient_for;

      while (parent->transient_for != NULL)
	parent = parent->transient_for;

      if (parent != mb_wm_get_visible_main_client(wm))
	mb_wm_client_show (parent);
    }

  if (parent_klass->show)
    parent_klass->show(client);
}

static void
mb_wm_client_dialog_class_init (MBWMObjectClass *klass)
{
  MBWindowManagerClientClass *client;

  MBWM_MARK();

  client = (MBWindowManagerClientClass *)klass;

  client->client_type = MBWMClientTypeDialog;
  client->geometry = mb_wm_client_dialog_request_geometry;
  client->stack = mb_wm_client_dialog_stack;
  client->show = mb_wm_client_dialog_show;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMClientDialog";
#endif
}

static void
mb_wm_client_dialog_stack (MBWindowManagerClient *client,
			   int                    flags)
{
  MBWM_MARK();
  mb_wm_stack_move_top(client);
}

static void
mb_wm_client_dialog_destroy (MBWMObject *this)
{
}

static void
mb_wm_client_dialog_init (MBWMObject *this, va_list vap)
{
  MBWindowManagerClient *client = MB_WM_CLIENT (this);
  MBWMClientDialog      *client_dialog = MB_WM_CLIENT_DIALOG (this);
  MBWMDecor             *decor;
  MBWMDecorButton       *button;
  MBWindowManager       *wm;
  MBWMClientWindow      *win;
  MBWMObjectProp         prop;
  
  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropWm:
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	case MBWMObjectPropClientWindow:
	  win = va_arg(vap, MBWMClientWindow *);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  mb_wm_client_set_layout_hints (client,
				 LayoutPrefPositionFree|LayoutPrefVisible);

  if (win->xwin_transient_for
      && win->xwin_transient_for != win->xwindow
      && win->xwin_transient_for != wm->root_win->xwindow)
    {
      MBWM_DBG ("Adding to '%lx' transient list",
		win->xwin_transient_for);
      mb_wm_client_add_transient
	(mb_wm_managed_client_from_xwindow (wm,
					    win->xwin_transient_for),
	 client);
      client->stacking_layer = 0;  /* We stack with whatever transient too */
    }
  else
    {
      MBWM_DBG ("Dialog is transient to root");
      /* Stack with 'always on top' */
      client->stacking_layer = MBWMStackLayerTopMid;
    }

  /* center if window sets 0,0 */
  if (client->window->geometry.x == 0 && client->window->geometry.y == 0)
    {
        MBGeometry  avail_geom;

	mb_wm_get_display_geometry (wm, &avail_geom);

	client->window->geometry.x
	  = (avail_geom.width - client->window->geometry.width) / 2;
	client->window->geometry.y
	  = (avail_geom.height - client->window->geometry.height) / 2;
    }

  client->frame_geometry.x      = client->window->geometry.x;
  client->frame_geometry.y      = client->window->geometry.y;
  client->frame_geometry.width  = client->window->geometry.width;
  client->frame_geometry.height = client->window->geometry.height;

}

int
mb_wm_client_dialog_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMClientDialogClass),
	sizeof (MBWMClientDialog),
	mb_wm_client_dialog_init,
	mb_wm_client_dialog_destroy,
	mb_wm_client_dialog_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_CLIENT_BASE, 0);
    }

  return type;
}

static Bool
mb_wm_client_dialog_request_geometry (MBWindowManagerClient *client,
				      MBGeometry            *new_geometry,
				      MBWMClientReqGeomType  flags)
{
  if (client->window->geometry.x != new_geometry->x
      || client->window->geometry.y != new_geometry->y
      || client->window->geometry.width  != new_geometry->width
      || client->window->geometry.height != new_geometry->height)
    {
      client->frame_geometry.x        = new_geometry->x;
      client->frame_geometry.y        = new_geometry->y;
      client->frame_geometry.width    = new_geometry->width;
      client->frame_geometry.height   = new_geometry->height;
      client->window->geometry.x      = new_geometry->x;
      client->window->geometry.y      = new_geometry->y;
      client->window->geometry.width  = new_geometry->width;
      client->window->geometry.height = new_geometry->height;

      mb_wm_client_geometry_mark_dirty (client);
    }

  return True; /* Geometry accepted */
}


MBWindowManagerClient*
mb_wm_client_dialog_new (MBWindowManager *wm, MBWMClientWindow *win)
{
  MBWindowManagerClient *client;

  client
    = MB_WM_CLIENT(mb_wm_object_new (MB_WM_TYPE_CLIENT_DIALOG,
				     MBWMObjectPropWm,           wm,
				     MBWMObjectPropClientWindow, win,
				     NULL));

  return client;
}

