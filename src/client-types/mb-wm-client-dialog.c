#include "mb-wm-client-dialog.h"

#include "mb-wm-theme.h"

#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_EDGE_SIZE 3

static Bool
mb_wm_client_dialog_request_geometry (MBWindowManagerClient *client,
				      MBGeometry            *new_geometry,
				      MBWMClientReqGeomType  flags);

static void
mb_wm_client_dialog_theme_change (MBWindowManagerClient *client);

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

  client->client_type  = MBWMClientTypeDialog;
  client->geometry     = mb_wm_client_dialog_request_geometry;
  client->stack        = mb_wm_client_dialog_stack;
  client->show         = mb_wm_client_dialog_show;
  client->theme_change = mb_wm_client_dialog_theme_change;

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

static int
mb_wm_client_dialog_init (MBWMObject *this, va_list vap)
{
  MBWindowManagerClient *client = MB_WM_CLIENT (this);
  MBWMClientDialog      *client_dialog = MB_WM_CLIENT_DIALOG (this);
  MBWindowManager       *wm = client->wmref;
  MBWMClientWindow      *win = client->window;

  mb_wm_client_set_layout_hints (client,
				 LayoutPrefPositionFree|LayoutPrefVisible);

  mb_wm_theme_create_decor (wm->theme, client, MBWMDecorTypeNorth);
  mb_wm_theme_create_decor (wm->theme, client, MBWMDecorTypeSouth);
  mb_wm_theme_create_decor (wm->theme, client, MBWMDecorTypeWest);
  mb_wm_theme_create_decor (wm->theme, client, MBWMDecorTypeEast);

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

  return 1;
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
      int north, south, west, east;
      MBWindowManager *wm = client->wmref;

      mb_wm_theme_get_decor_dimensions (wm->theme, client,
					&north, &south, &west, &east);

      client->frame_geometry.x      = new_geometry->x;
      client->frame_geometry.y      = new_geometry->y;
      client->frame_geometry.width  = new_geometry->width;
      client->frame_geometry.height = new_geometry->height;

      client->window->geometry.x
	= client->frame_geometry.x + west;
      client->window->geometry.y
	= client->frame_geometry.y + north;
      client->window->geometry.width
	= client->frame_geometry.width - (west + east);
      client->window->geometry.height
	= client->frame_geometry.height - (south + north);

      mb_wm_client_geometry_mark_dirty (client);

      return True; /* Geometry accepted */
    }

  return True; /* Geometry accepted */
}

static void
mb_wm_client_dialog_theme_change (MBWindowManagerClient *client)
{
  MBWMList * l = client->decor;

  while (l)
    {
      MBWMDecor * d = l->data;
      MBWMList * n = l->next;

      mb_wm_object_unref (MB_WM_OBJECT (d));
      free (l);

      l = n;
    }

  client->decor = NULL;

  mb_wm_theme_create_decor (client->wmref->theme, client, MBWMDecorTypeNorth);
  mb_wm_theme_create_decor (client->wmref->theme, client, MBWMDecorTypeSouth);
  mb_wm_theme_create_decor (client->wmref->theme, client, MBWMDecorTypeWest);
  mb_wm_theme_create_decor (client->wmref->theme, client, MBWMDecorTypeEast);

  mb_wm_client_geometry_mark_dirty (client);
  mb_wm_client_visibility_mark_dirty (client);
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

