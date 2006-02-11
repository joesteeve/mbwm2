#include "mb-wm-client-app.h"

#include "mb-wm-theme.h"

struct MBWindowManagerClientApp
{
  MBWindowManagerClient base_client;
};

int
mb_wm_client_app_get_type ()
{
  static int type = 0;

  printf("type: %i\n", type);

  if (type == 0)
    type = mb_wm_register_client_type();

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
      
      client->window->geometry.x = client->frame_geometry.x + 4;
      client->window->geometry.y = client->frame_geometry.y + 20;
      client->window->geometry.width  = client->frame_geometry.width - 8;
      client->window->geometry.height = client->frame_geometry.height - 24;
      
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
  MBWindowManagerClientApp *tc = NULL;
  MBWMDecor                *decor;

  tc = mb_wm_util_malloc0(sizeof(MBWindowManagerClientApp));

  mb_wm_client_init (wm, MBWM_CLIENT(tc), win);

  tc->base_client.type = mb_wm_client_app_get_type ();

  fprintf(stderr, "XXX tc->base_client.type:%i\n", tc->base_client.type);

  mb_wm_client_set_layout_hints (MBWM_CLIENT(tc),
				 LayoutPrefGrowToFreeSpace|LayoutPrefVisible);

  decor = mb_wm_decor_create (wm, MBWMDecorTypeNorth, 
			      decor_resize, decor_repaint, NULL);
  mb_wm_decor_attach (decor, MBWM_CLIENT(tc));

  decor = mb_wm_decor_create (wm, MBWMDecorTypeSouth, 
			      decor_resize, decor_repaint, NULL);
  mb_wm_decor_attach (decor, MBWM_CLIENT(tc));

  decor = mb_wm_decor_create (wm, MBWMDecorTypeEast, 
			      decor_resize, decor_repaint, NULL);
  mb_wm_decor_attach (decor, MBWM_CLIENT(tc));

  decor = mb_wm_decor_create (wm, MBWMDecorTypeWest, 
			      decor_resize, decor_repaint, NULL);
  mb_wm_decor_attach (decor, MBWM_CLIENT(tc));
  
  tc->base_client.geometry = mb_wm_client_app_request_geometry; 

  return MBWM_CLIENT(tc);
}

