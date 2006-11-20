#include "mb-wm-client-app.h"

#include "mb-wm-theme.h"

#define FRAME_TITLEBAR_HEIGHT 20
#define FRAME_EDGE_SIZE 3

static Bool
mb_wm_client_app_request_geometry (MBWindowManagerClient *client,
				   MBGeometry            *new_geometry,
				   MBWMClientReqGeomType  flags);

void
mb_wm_client_app_class_init (MBWMObjectClass *klass) 
{
  MBWindowManagerClientClass *client;

  MBWM_MARK();

  client = (MBWindowManagerClientClass *)klass;

  MBWM_DBG("client->stack is %p", client->stack);

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
      type = mb_wm_object_register_class (&info, MB_WM_TYPE_CLIENT_BASE, 0);
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
      
      client->window->geometry.x 
	= client->frame_geometry.x + FRAME_EDGE_SIZE;
      client->window->geometry.y 
	= client->frame_geometry.y + FRAME_TITLEBAR_HEIGHT;
      client->window->geometry.width  
	= client->frame_geometry.width - (2*FRAME_EDGE_SIZE);
      client->window->geometry.height 
	= client->frame_geometry.height 
            - FRAME_EDGE_SIZE - FRAME_TITLEBAR_HEIGHT;
      
      mb_wm_client_geometry_mark_dirty (client);

      return True; /* Geometry accepted */
    }
}

static void
decor_resize (MBWindowManager   *wm,
	      MBWMDecor         *decor,
	      void              *userdata)
{
  const MBGeometry *geom;
  MBWMDecorButton  *button;
  MBWMClientApp    *client_app;
  
  client_app = (MBWMClientApp *)userdata; 

  geom = mb_wm_decor_get_geometry (decor);

  button = (MBWMDecorButton  *)decor->buttons->data;

  mb_wm_decor_button_move_to (client_app->button_close, 
			      geom->width - FRAME_TITLEBAR_HEIGHT -2 , 2);
}

static void
decor_repaint (MBWindowManager   *wm,
	       MBWMDecor         *decor,
	       void              *userdata)
{
  mb_wm_theme_paint_decor (wm, decor); 
}

static void
close_button_pressed (MBWindowManager   *wm,
		      MBWMDecorButton   *button,
		      void              *userdata)
{
  MBWindowManagerClient *client = (MBWindowManagerClient*)userdata;

  XDestroyWindow(wm->xdpy, MB_WM_CLIENT_XWIN(client));
}

MBWindowManagerClient*
mb_wm_client_app_new (MBWindowManager *wm, MBWMWindow *win)
{
  MBWindowManagerClient    *client;
  MBWMClientApp            *client_app;
  MBWMDecor                *decor;
  MBWMDecorButton          *button;

  client_app = MB_WM_CLIENT_APP(mb_wm_object_new (MB_WM_TYPE_CLIENT_APP));

  if (!client_app)
    return NULL; 		/* FIXME: Handle out of memory */

  client = MB_WM_CLIENT(client_app);

  client->window        = win; 	
  client->wmref         = wm;
  client->stacking_layer = MBWMStackLayerMid;

  mb_wm_client_set_layout_hints (client,
				 LayoutPrefGrowToFreeSpace|LayoutPrefVisible);

  /* Titlebar */

  decor = mb_wm_decor_create (wm, MBWMDecorTypeNorth, 
			      decor_resize, decor_repaint, client_app);

  client_app->button_close 
    = mb_wm_decor_button_create (wm,
				 decor,
				 FRAME_TITLEBAR_HEIGHT-2, 
				 FRAME_TITLEBAR_HEIGHT-2,
				 NULL,
				 close_button_pressed,
				 NULL,
				 0,
				 client);

  mb_wm_decor_button_show (client_app->button_close);

  /* Borders */

  mb_wm_decor_attach (decor, client);

  decor = mb_wm_decor_create (wm, MBWMDecorTypeSouth, 
			      NULL, decor_repaint, NULL);
  mb_wm_decor_attach (decor, client);

  decor = mb_wm_decor_create (wm, MBWMDecorTypeEast, 
			      NULL, decor_repaint, NULL);
  mb_wm_decor_attach (decor, client);

  decor = mb_wm_decor_create (wm, MBWMDecorTypeWest, 
			      NULL, decor_repaint, NULL);
  mb_wm_decor_attach (decor, client);

  return client;
}

