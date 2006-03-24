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

void
mb_wm_client_dialog_class_init (MBWMObjectClass *klass) 
{
  MBWindowManagerClientClass *client;

  MBWM_MARK();

  mb_wm_client_base_class_init (klass); 

  client = (MBWindowManagerClientClass *)klass;

  client->geometry = mb_wm_client_dialog_request_geometry; 
  client->stack = mb_wm_client_dialog_stack;
}

 void
mb_wm_client_dialog_stack (MBWindowManagerClient *client,
			   int                    flags)
{
  MBWM_MARK();
  mb_wm_stack_move_top(client);
}

void
mb_wm_client_dialog_destroy (MBWMObject *this)
{
  mb_wm_client_base_destroy(this);
}

void
mb_wm_client_dialog_init (MBWMObject *this)
{
  MBWM_MARK();

  mb_wm_client_base_init (this);
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

      type = mb_wm_object_register_class (&info);
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
      client->frame_geometry.x      = new_geometry->x;
      client->frame_geometry.y      = new_geometry->y;
      client->frame_geometry.width  = new_geometry->width;
      client->frame_geometry.height = new_geometry->height;
      client->window->geometry.x = new_geometry->x;
      client->window->geometry.y = new_geometry->y;
      client->window->geometry.width  = new_geometry->width;
      client->window->geometry.height = new_geometry->height;

      mb_wm_client_geometry_mark_dirty (client);
    }

  return True; /* Geometry accepted */
}


MBWindowManagerClient*
mb_wm_client_dialog_new (MBWindowManager *wm, MBWMWindow *win)
{
  MBWindowManagerClient    *client;
  MBWMClientDialog         *client_dialog;
  MBWMDecor                *decor;
  MBWMDecorButton          *button;

  client_dialog = MB_WM_CLIENT_DIALOG(mb_wm_object_new (MB_WM_TYPE_CLIENT_DIALOG));

  if (!client_dialog)
    return NULL; 		/* FIXME: Handle out of memory */

  client = MB_WM_CLIENT(client_dialog);

  client->window = win; 	
  client->wmref  = wm;

  mb_wm_client_set_layout_hints (client, 
				 LayoutPrefPositionFree|LayoutPrefVisible);

  if (win->xwin_transient_for
      && win->xwin_transient_for != win->xwindow
      && win->xwin_transient_for != wm->xwin_root)
    {
      mb_wm_client_add_transient (mb_wm_core_managed_client_from_xwindow (wm, 
									  win->xwin_transient_for),
				  
				  client);
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

  return client;
}

