#include "mb-wm-theme-simple.h"

static void
mb_wm_theme_simple_paint_decor (MBWMTheme *theme, MBWMDecor *decor);
static void
mb_wm_theme_simple_paint_button (MBWMTheme *theme, MBWMDecorButton *button);

static void
mb_wm_theme_simple_class_init (MBWMObjectClass *klass)
{
  MBWMThemeClass *t_class = MB_WM_THEME_CLASS (klass);

  t_class->paint_decor  = mb_wm_theme_simple_paint_decor;
  t_class->paint_button = mb_wm_theme_simple_paint_button;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMThemeSimple";
#endif
}

static void
mb_wm_theme_simple_destroy (MBWMObject *obj)
{
}

static int
mb_wm_theme_simple_init (MBWMObject *obj, va_list vap)
{

  return 1;
}

int
mb_wm_theme_simple_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMThemeSimpleClass),
	sizeof (MBWMThemeSimple),
	mb_wm_theme_simple_init,
	mb_wm_theme_simple_destroy,
	mb_wm_theme_simple_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_THEME, 0);
    }

  return type;
}

static void
mb_wm_theme_simple_paint_decor (MBWMTheme *theme, MBWMDecor *decor)
{
  MBWMDecorType          type;
  MBGeometry            *geom;
  MBWindowManagerClient *client;
  Window                 xwin;
  MBWindowManager       *wm = theme->wm;

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

static void
mb_wm_theme_simple_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
}
