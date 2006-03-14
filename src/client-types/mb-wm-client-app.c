#include "mb-wm-client-app.h"

#include "mb-wm-theme.h"

static Bool
mb_wm_client_app_request_geometry (MBWindowManagerClient *client,
				   MBGeometry            *new_geometry,
				   MBWMClientReqGeomType  flags);

void
mb_wm_client_app_class_init (MBWMObjectClass *klass) 
{
  MBWindowManagerClientClass *client;

  MBWM_MARK();

  mb_wm_client_base_class_init (klass); 

  client = (MBWindowManagerClientClass *)klass;

  client->geometry = mb_wm_client_app_request_geometry; 
}

void
mb_wm_client_app_destroy (MBWMObject *this)
{
  MBWM_MARK();
  mb_wm_client_base_destroy(this);
}

void
mb_wm_client_app_init (MBWMObject *this)
{
  MBWM_MARK();

  mb_wm_client_base_init (this);
}

int
mb_wm_client_app_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMClientAppClass),      
	sizeof (MBWMClientApp), 
	mb_wm_client_app_init,
	mb_wm_client_app_destroy,
	mb_wm_client_app_class_init
      };

      type = mb_wm_object_register_class (&info);
    }

  return type;
}

static Bool
mb_wm_client_app_request_geometry (MBWindowManagerClient *client,
				   MBGeometry            *new_geometry,
				   MBWMClientReqGeomType  flags)
{
  if (flags & MBWMClientReqGeomIsViaLayoutManager)
    {
      client->frame_geometry.x      = new_geometry->x;
      client->frame_geometry.y      = new_geometry->y;
      client->frame_geometry.width  = new_geometry->width;
      client->frame_geometry.height = new_geometry->height;
      
      client->window->geometry.x = client->frame_geometry.x + 3;
      client->window->geometry.y = client->frame_geometry.y + 20;
      client->window->geometry.width  = client->frame_geometry.width - 6;
      client->window->geometry.height = client->frame_geometry.height - 23;
      
      mb_wm_client_geometry_mark_dirty (client);

      return True; /* Geometry accepted */
    }
}

static void
decor_resize (MBWindowManager   *wm,
	      MBWMDecor         *decor,
	      void              *userdata)
{
  MBWM_DBG("mark");
}

static void
decor_repaint (MBWindowManager   *wm,
	       MBWMDecor         *decor,
	       void              *userdata)
{
  /*
  if (mb_wm_decor_get_type(decor) != MBWMDecorTypeNorth)
    return;
  */

  mb_wm_theme_paint_decor (wm, decor); 

  /*
  XSetWindowBackground(wm->xdpy, 
		       mb_wm_decor_get_x_window (decor),
		       WhitePixel(wm->xdpy, wm->xscreen));
  */
}

MBWindowManagerClient*
mb_wm_client_app_new (MBWindowManager *wm, MBWMWindow *win)
{
  MBWindowManagerClient    *client;
  MBWMDecor                *decor;

  client = MB_WM_CLIENT(mb_wm_object_new (MB_WM_TYPE_CLIENT_APP));

  if (!client)
    return NULL; 		/* FIXME: Handle out of memory */

  client->window = win; 	
  client->wmref  = wm;

  mb_wm_client_set_layout_hints (client,
				 LayoutPrefGrowToFreeSpace|LayoutPrefVisible);

  decor = mb_wm_decor_create (wm, MBWMDecorTypeNorth, 
			      decor_resize, decor_repaint, NULL);
  mb_wm_decor_attach (decor, client);

  decor = mb_wm_decor_create (wm, MBWMDecorTypeSouth, 
			      decor_resize, decor_repaint, NULL);
  mb_wm_decor_attach (decor, client);

  decor = mb_wm_decor_create (wm, MBWMDecorTypeEast, 
			      decor_resize, decor_repaint, NULL);
  mb_wm_decor_attach (decor, client);

  decor = mb_wm_decor_create (wm, MBWMDecorTypeWest, 
			      decor_resize, decor_repaint, NULL);
  mb_wm_decor_attach (decor, client);

  return client;
}

