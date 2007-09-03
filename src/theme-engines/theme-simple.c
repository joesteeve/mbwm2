#include "mb-wm-theme.h"

typedef struct ThemeSimple
{
  


} 
ThemeSimple;


void
theme_simple_paint_decor (MBWindowManager   *wm,
			 MBWMDecor         *decor)
{
  MBWMDecorType          type;
  MBGeometry            *geom;
  MBWindowManagerClient *client;
  Window                 xwin;

  client = mb_wm_decor_get_parent (decor);
  xwin = mb_wm_decor_get_x_window (decor);

  if (client == NULL || xwin == None) return;

  type = mb_wm_decor_get_type (decor);
  geom = mb_wm_decor_get_geometry (decor);

  switch (type)
    {
    case MBWMDecorTypeNorth:
      break;
    case MBWMDecorTypeSouth:
    case MBWMDecorTypeWest:
    case MBWMDecorTypeEast:
      break;
    default:
      break;
    }
}

void
theme_simple_paint_button (MBWindowManager   *wm,
			   MBWMDecorButton   *button)
{

}

Bool
theme_simple_switch (MBWindowManager   *wm,
		     const char        *detail)
{

}

void
theme_simple_init (MBWindowManager   *wm)
{

}
