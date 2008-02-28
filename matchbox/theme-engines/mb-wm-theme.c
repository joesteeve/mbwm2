/*
 *  Matchbox Window Manager II - A lightweight window manager not for the
 *                               desktop.
 *
 *  Authored By Tomas Frydrych <tf@o-hand.com>
 *
 *  Copyright (c) 2005 OpenedHand Ltd - http://o-hand.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include "mb-wm-theme.h"
#include "mb-wm-theme-xml.h"

#if USE_CAIRO
#include "mb-wm-theme-cairo.h"
#else
#include "mb-wm-theme-simple.h"
#endif

#include <sys/stat.h>
#include <expat.h>

static void
xml_element_start_cb (void *data, const char *tag, const char **expat_attr);

static void
xml_element_end_cb (void *data, const char *tag);

static void
xml_stack_free (MBWMList *stack);

static void
mb_wm_theme_destroy (MBWMObject *obj)
{
  MBWMTheme *theme = MB_WM_THEME (obj);

  if (theme->path)
    free (theme->path);

  MBWMList *l = theme->xml_clients;

  while (l)
    {
      MBWMXmlClient * c = l->data;
      MBWMList * n = l->next;
      mb_wm_xml_client_free (c);
      free (l);

      l = n;
    }
}

static int
mb_wm_theme_init (MBWMObject *obj, va_list vap)
{
  MBWMTheme        *theme = MB_WM_THEME (obj);
  MBWindowManager  *wm = NULL;
  MBWMObjectProp    prop;
  MBWMList         *xml_clients = NULL;
  char             *path = NULL;
  MBWMColor        *clr_lowlight = NULL;
  MBWMColor        *clr_shadow = NULL;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropWm:
	  wm = va_arg(vap, MBWindowManager *);
	  break;
	case MBWMObjectPropThemePath:
	  path = va_arg(vap, char *);
	  break;
	case MBWMObjectPropThemeXmlClients:
	  xml_clients = va_arg(vap, MBWMList *);
	  break;
	case MBWMObjectPropThemeColorLowlight:
	  clr_lowlight = va_arg(vap, MBWMColor *);
	  break;
	case MBWMObjectPropThemeColorShadow:
	  clr_shadow = va_arg(vap, MBWMColor *);
	  break;
	case MBWMObjectPropThemeShadowType:
	  theme->shadow_type = va_arg(vap, int);
	  break;
	case MBWMObjectPropThemeCompositing:
	  theme->compositing = va_arg(vap, int);
	  break;
	case MBWMObjectPropThemeShaped:
	  theme->shaped = va_arg(vap, int);
	  break;

	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  theme->wm = wm;
  theme->xml_clients = xml_clients;

  if (path)
    theme->path = strdup (path);

  if (clr_shadow && clr_shadow->set)
    {
      theme->color_shadow.r = clr_shadow->r;
      theme->color_shadow.g = clr_shadow->g;
      theme->color_shadow.b = clr_shadow->b;
      theme->color_shadow.a = clr_shadow->a;
    }
  else
    {
      theme->color_shadow.r = 0.0;
      theme->color_shadow.g = 0.0;
      theme->color_shadow.b = 0.0;
      theme->color_shadow.a = 0.95;
    }

  if (clr_lowlight && clr_lowlight->set)
    {
      theme->color_lowlight.r = clr_lowlight->r;
      theme->color_lowlight.g = clr_lowlight->g;
      theme->color_lowlight.b = clr_lowlight->b;
      theme->color_lowlight.a = clr_lowlight->a;
    }
  else
    {
      theme->color_lowlight.r = 0.0;
      theme->color_lowlight.g = 0.0;
      theme->color_lowlight.b = 0.0;
      theme->color_lowlight.a = 0.55;
    }

  return 1;
}

int
mb_wm_theme_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMThemeClass),
	sizeof (MBWMTheme),
	mb_wm_theme_init,
	mb_wm_theme_destroy,
	NULL
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_OBJECT, 0);
    }

  return type;
}

Bool
mb_wm_theme_is_button_press_activated (MBWMTheme              *theme,
				       MBWMDecor              *decor,
				       MBWMDecorButtonType    type)
{
  MBWindowManagerClient * client;
  MBWMXmlClient         * c;
  MBWMXmlDecor          * d;
  MBWMXmlButton         * b;
  MBWMClientType          c_type;

  if (!theme || !theme->xml_clients || !decor || !decor->parent_client)
    return False;

  client = decor->parent_client;
  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) &&
      (d = mb_wm_xml_decor_find_by_type (c->decors, decor->type)) &&
      (b = mb_wm_xml_button_find_by_type (d->buttons, type)))
    {
      return b->press_activated;
    }

  return False;
}

void
mb_wm_theme_get_button_size (MBWMTheme             *theme,
			     MBWMDecor             *decor,
			     MBWMDecorButtonType    type,
			     int                   *width,
			     int                   *height)
{
  MBWMThemeClass *klass;

  MBWM_ASSERT (decor && decor->parent_client);

  if (!theme || !decor || !decor->parent_client)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->button_size)
    klass->button_size (theme, decor, type, width, height);
}

/*
 * If the parent decor uses absolute postioning, the returned values
 * are absolute. If the decor does packing, these values are added to
 * calculated button position.
 */
void
mb_wm_theme_get_button_position (MBWMTheme             *theme,
				 MBWMDecor             *decor,
				 MBWMDecorButtonType    type,
				 int                   *x,
				 int                   *y)
{
  MBWMThemeClass *klass;

  MBWM_ASSERT (decor && decor->parent_client);

  if (!theme || !decor || !decor->parent_client)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->button_position)
    klass->button_position (theme, decor, type, x, y);
  else
    {
      if (x)
	*x = 2;

      if (y)
	*y = 2;
    }
}

void
mb_wm_theme_get_decor_dimensions (MBWMTheme             *theme,
				  MBWindowManagerClient *client,
				  int                   *north,
				  int                   *south,
				  int                   *west,
				  int                   *east)
{
  MBWMThemeClass *klass;

  MBWM_ASSERT (client);

  if (!theme || !client)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->decor_dimensions)
    klass->decor_dimensions (theme, client, north, south, west, east);
}

void
mb_wm_theme_paint_decor (MBWMTheme *theme, MBWMDecor *decor)
{
  MBWMThemeClass *klass;

  if (!theme)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->paint_decor)
    klass->paint_decor (theme, decor);
}

void
mb_wm_theme_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
  MBWMThemeClass *klass;

  if (!theme)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->paint_button)
    klass->paint_button (theme, button);
}

Bool
mb_wm_theme_supports (MBWMTheme *theme, MBWMThemeCaps capability)
{
  if (theme)
    return False;

  return ((capability & theme->caps != False));
}

typedef enum
{
  XML_CTX_UNKNOWN = 0,
  XML_CTX_THEME,
  XML_CTX_CLIENT,
  XML_CTX_DECOR,
  XML_CTX_BUTTON,
  XML_CTX_IMG,
  XML_CTX_EFFECT,
  XML_CTX_TRANSITION,
} XmlCtx;

struct stack_data
{
  XmlCtx  ctx;
  void   *data;
};

struct expat_data
{
  XML_Parser   par;
  int          theme_type;
  int          version;
  MBWMList     *xml_clients;
  char         *img;
  MBWMList     *stack;
  MBWMColor     color_lowlight;
  MBWMColor     color_shadow;
  MBWMCompMgrShadowType shadow_type;
  Bool          compositing;
  Bool          shaped;
};

MBWMTheme *
mb_wm_theme_new (MBWindowManager * wm, const char * theme_path)
{
  MBWMTheme     *theme = NULL;
  int            theme_type = 0;
  char          *path = NULL;
  char           buf[256];
  XML_Parser     par = NULL;
  FILE          *file = NULL;
  MBWMList      *xml_clients = NULL;
  char          *img = NULL;
  MBWMColor      clr_lowlight;
  MBWMColor      clr_shadow;
  MBWMCompMgrShadowType shadow_type;
  Bool           compositing;
  Bool           shaped;
  struct stat    st;

  /*
   * If no theme specified, we try to load the default one, if that fails,
   * we automatically fallback on the built-in defaults.
   */
  if (!theme_path)
    theme_path = "Default";

  /* Attempt to parse the xml theme, if any, retrieving the theme type
   *
   * NB: We cannot do this in the _init function, since we need to know the
   *     type *before* we can create the underlying object on which the
   *     init method operates.
   */

  if (*theme_path == '/')
    {
      if (!stat (theme_path, &st))
	path = (char *) theme_path;
    }
  else
    {
      const char  *home = getenv("HOME");
      int          size;

      if (home)
	{
	  const char  *fmt = "%s/.themes/%s/matchbox2/theme.xml";

	  size = strlen (theme_path) + strlen (fmt) + strlen (home);
	  path = alloca (size);
	  snprintf (path, size, fmt, home, theme_path);

	  if (stat (path, &st))
	    path = NULL;
	}

      if (!path)
	{
	  const char * fmt = "%s/themes/%s/matchbox2/theme.xml";

	  size = strlen (theme_path) + strlen (fmt) + strlen (DATADIR);
	  path = alloca (size);
	  snprintf (path, size, fmt, DATADIR, theme_path);

	  if (stat (path, &st))
	    path = NULL;
	}
    }

  if (path)
    {
      struct expat_data  udata;

      if (!(file = fopen (path, "r")) ||
	  !(par = XML_ParserCreate(NULL)))
	{
	  goto default_theme;
	}

      memset (&udata, 0, sizeof (struct expat_data));
      udata.compositing = True;
      udata.par         = par;

      XML_SetElementHandler (par,
			     xml_element_start_cb,
			     xml_element_end_cb);

      XML_SetUserData(par, (void *)&udata);

      while (fgets (buf, sizeof (buf), file) &&
	     XML_Parse(par, buf, strlen(buf), 0));

      XML_Parse(par, NULL, 0, 1);

      if (udata.version == 2)
	{
	  theme_type  = udata.theme_type;
	  xml_clients = udata.xml_clients;

	  if (udata.img)
	    {
	      if (*udata.img == '/')
		img = udata.img;
	      else
		{
		  int len = strlen (path) + strlen (udata.img);
		  char * s;
		  char * p = malloc (len + 1);
		  strncpy (p, path, len);

		  s = strrchr (p, '/');

		  if (s)
		    {
		      *(s+1) = 0;
		      strcat (p, udata.img);
		    }
		  else
		    {
		      strncpy (p, udata.img, len);
		    }

		  img = p;
		  free (udata.img);
		}
	    }
	}

      clr_lowlight.r   = udata.color_lowlight.r;
      clr_lowlight.g   = udata.color_lowlight.g;
      clr_lowlight.b   = udata.color_lowlight.b;
      clr_lowlight.a   = udata.color_lowlight.a;
      clr_lowlight.set = udata.color_lowlight.set;

      clr_shadow.r   = udata.color_shadow.r;
      clr_shadow.g   = udata.color_shadow.g;
      clr_shadow.b   = udata.color_shadow.b;
      clr_shadow.a   = udata.color_shadow.a;
      clr_shadow.set = udata.color_shadow.set;

      shadow_type = udata.shadow_type;
      compositing = udata.compositing;
      shaped      = udata.shaped;

      xml_stack_free (udata.stack);
    }

  if (theme_type)
    {
      theme =
	MB_WM_THEME (mb_wm_object_new (theme_type,
			MBWMObjectPropWm,                  wm,
			MBWMObjectPropThemePath,           path,
			MBWMObjectPropThemeImg,            img,
			MBWMObjectPropThemeXmlClients,     xml_clients,
			MBWMObjectPropThemeColorLowlight, &clr_lowlight,
			MBWMObjectPropThemeColorShadow,   &clr_shadow,
			MBWMObjectPropThemeShadowType,     shadow_type,
			MBWMObjectPropThemeCompositing,    compositing,
			MBWMObjectPropThemeShaped,         shaped,
			NULL));
    }

 default_theme:

  if (!theme)
    {
      theme = MB_WM_THEME (mb_wm_object_new (
#if USE_CAIRO
			MB_WM_TYPE_THEME_CAIRO,
#else
			MB_WM_TYPE_THEME_SIMPLE,
#endif
	                MBWMObjectPropWm,                  wm,
			MBWMObjectPropThemeXmlClients,     xml_clients,
			MBWMObjectPropThemeColorLowlight, &clr_lowlight,
			MBWMObjectPropThemeColorShadow,   &clr_shadow,
			MBWMObjectPropThemeShadowType,     shadow_type,
			MBWMObjectPropThemeCompositing,    compositing,
			MBWMObjectPropThemeShaped,         shaped,
			NULL));
    }

  if (par)
    XML_ParserFree (par);

  if (file)
    fclose (file);

  if (img)
    free (img);

  return theme;
}

MBWMDecor *
mb_wm_theme_create_decor (MBWMTheme             *theme,
			  MBWindowManagerClient *client,
			  MBWMDecorType          type)
{
  MBWMThemeClass *klass;

  MBWM_ASSERT (client);

  if (!theme || !client)
    return NULL;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->create_decor)
    return klass->create_decor (theme, client, type);
}

void
mb_wm_theme_resize_decor (MBWMTheme *theme, MBWMDecor *decor)
{
  MBWMThemeClass *klass;

  MBWM_ASSERT (decor);

  if (!theme || !decor)
    return;

  klass = MB_WM_THEME_CLASS(MB_WM_OBJECT_GET_CLASS (theme));

  if (klass->resize_decor)
    klass->resize_decor (theme, decor);
}

MBWMClientLayoutHints
mb_wm_theme_get_client_layout_hints (MBWMTheme             * theme,
				     MBWindowManagerClient * client)
{
  MBWMXmlClient * c;
  MBWMClientType  c_type;

  if (!client || !theme)
    return 0;

  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if (!theme->xml_clients ||
      !(c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)))
    {
      return 0;
    }

  return c->layout_hints;
}

#if ENABLE_COMPOSITE
const MBWMList *
mb_wm_theme_get_client_effects (MBWMTheme              * theme,
				MBWindowManagerClient  * client)
{
  MBWMXmlClient * c;
  MBWMClientType  c_type;

  if (!client || !theme)
    return NULL;

  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if (!theme->xml_clients ||
      !(c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)))
    {
      return NULL;
    }

  return c->effects;
}

MBWMThemeTransition *
mb_wm_theme_get_client_transition (MBWMTheme             * theme,
				   MBWindowManagerClient * client)
{
  MBWMXmlClient * c;
  MBWMClientType  c_type;

  if (!client || !theme)
    return NULL;

  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if (!theme->xml_clients ||
      !(c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)))
    {
      return NULL;
    }

  return c->transition;
}
#endif

/*
 * Returns True if the theme prescribes at least one value for the geometry
 */
Bool
mb_wm_theme_get_client_geometry (MBWMTheme             * theme,
				 MBWindowManagerClient * client,
				 MBGeometry            * geom)
{
  MBWMXmlClient * c;
  MBWMClientType  c_type;

  if (!geom || !client || !theme)
    return False;

  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if (!theme || !theme->xml_clients ||
      !(c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) ||
      (c->x < 0 && c->y < 0 && c->width < 0 && c->height < 0))
    {
      return False;
    }

  geom->x      = c->x;
  geom->y      = c->y;
  geom->width  = c->width;
  geom->height = c->height;

  return True;
}

Bool
mb_wm_theme_is_client_shaped (MBWMTheme             * theme,
			      MBWindowManagerClient * client)
{
#ifdef HAVE_XEXT
  MBWMXmlClient * c;
  MBWMClientType  c_type;

  if (!client || !theme || !theme->shaped || client->is_argb32)
    return False;

  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if (theme->xml_clients &&
      (c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)))
    {
      return c->shaped;
    }

  return False;
#else
  return False;
#endif
}

/*
 * Retrieves color to be used for lowlighting (16-bit rgba)
 */
void
mb_wm_theme_get_lowlight_color (MBWMTheme             * theme,
				unsigned int          * red,
				unsigned int          * green,
				unsigned int          * blue,
				unsigned int          * alpha)
{
  if (theme)
    {
      if (red)
	*red = (unsigned int)(theme->color_lowlight.r * (double)0xffff);

      if (green)
	*green = (unsigned int)(theme->color_lowlight.g * (double)0xffff);

      if (blue)
	*blue = (unsigned int)(theme->color_lowlight.b * (double)0xffff);

      if (alpha)
	*alpha = (unsigned int)(theme->color_lowlight.a * (double)0xffff);

      return;
    }

  if (red)
    *red = 0;

  if (green)
    *green = 0;

  if (blue)
    *blue = 0;

  if (*alpha)
    *alpha = 0x8d8d;
}

/*
 * Retrieves color to be used for lowlighting (16-bit rgba)
 */
void
mb_wm_theme_get_shadow_color (MBWMTheme             * theme,
			      unsigned int          * red,
			      unsigned int          * green,
			      unsigned int          * blue,
			      unsigned int          * alpha)
{
  if (theme)
    {
      if (red)
	*red = (unsigned int)(theme->color_shadow.r * (double)0xffff);

      if (green)
	*green = (unsigned int)(theme->color_shadow.g * (double)0xffff);

      if (blue)
	*blue = (unsigned int)(theme->color_shadow.b * (double)0xffff);

      if (alpha)
	*alpha = (unsigned int)(theme->color_shadow.a * (double)0xffff);

      return;
    }

  if (red)
    *red = 0;

  if (green)
    *green = 0;

  if (blue)
    *blue = 0;

  if (*alpha)
    *alpha = 0xff00;
}

MBWMCompMgrShadowType
mb_wm_theme_get_shadow_type (MBWMTheme * theme)
{
  if (!theme)
    return MBWM_COMP_MGR_SHADOW_NONE;

  return theme->shadow_type;
}

Bool
mb_wm_theme_use_compositing_mgr (MBWMTheme * theme)
{
  if (!theme)
    return False;

  return theme->compositing;
}

/*
 * Expat callback stuff
 */

static void
xml_stack_push (MBWMList ** stack, XmlCtx ctx)
{
  struct stack_data * s = malloc (sizeof (struct stack_data));

  s->ctx = ctx;
  s->data = NULL;

  *stack = mb_wm_util_list_prepend (*stack, s);
}

static XmlCtx
xml_stack_top_ctx (MBWMList *stack)
{
  struct stack_data * s = stack->data;

  return s->ctx;
}

static void *
xml_stack_top_data (MBWMList *stack)
{
  struct stack_data * s = stack->data;

  return s->data;
}

static void
xml_stack_top_set_data (MBWMList *stack, void * data)
{
  struct stack_data * s = stack->data;

  s->data = data;
}

static void
xml_stack_pop (MBWMList ** stack)
{
  MBWMList * top = *stack;
  struct stack_data * s = top->data;

  *stack = top->next;
  free (s);
  free (top);
}

static void
xml_stack_free (MBWMList *stack)
{
  MBWMList * l = stack;
  while (l)
    {
      MBWMList * n = l->next;
      free (l->data);
      free (l);

      l = n;
    }
}

static void
xml_element_start_cb (void *data, const char *tag, const char **expat_attr)
{
  struct expat_data * exd = data;

  MBWM_DBG ("tag <%s>\n", tag);

  if (!strcmp (tag, "theme"))
    {
      MBWMColor clr;
      const char ** p = expat_attr;

      xml_stack_push (&exd->stack, XML_CTX_THEME);

      while (*p)
	{
	  if (!strcmp (*p, "engine-version"))
	    exd->version = atoi (*(p+1));
	  else if (!strcmp (*p, "engine-type"))
	    {
	      if (!strcmp (*(p+1), "default"))
#if USE_CAIRO
		exd->theme_type = MB_WM_TYPE_THEME_CAIRO;
#else
		exd->theme_type = MB_WM_TYPE_THEME_SIMPLE;
#endif
#if THEME_PNG
	      else if (!strcmp (*(p+1), "png"))
		exd->theme_type = MB_WM_TYPE_THEME_PNG;
#endif
	    }
	  else if (!strcmp (*p, "shaped"))
	    {
	      if (!strcmp (*(p+1), "yes") || !strcmp (*(p+1), "1"))
		exd->shaped = 1;
	    }
	  else if (!strcmp (*p, "clr-shadow"))
	    {
	      mb_wm_xml_clr_from_string (&clr, *(p+1));
	      exd->color_shadow.r = clr.r;
	      exd->color_shadow.g = clr.g;
	      exd->color_shadow.b = clr.b;
	      exd->color_shadow.a = clr.a;
	      exd->color_shadow.set = True;
	    }
	  else if (!strcmp (*p, "clr-lowlight"))
	    {
	      mb_wm_xml_clr_from_string (&clr, *(p+1));
	      exd->color_lowlight.r = clr.r;
	      exd->color_lowlight.g = clr.g;
	      exd->color_lowlight.b = clr.b;
	      exd->color_lowlight.a = clr.a;
	      exd->color_lowlight.set = True;
	    }
	  else if (!strcmp (*p, "shadow-type"))
	    {
	      if (!strcmp (*(p+1), "simple"))
		exd->shadow_type = MBWM_COMP_MGR_SHADOW_SIMPLE;
	      else if (!strcmp (*(p+1), "gaussian"))
		exd->shadow_type = MBWM_COMP_MGR_SHADOW_GAUSSIAN;
	    }
	  else if (!strcmp (*p, "compositing"))
	    {
	      if (!strcmp (*(p+1), "yes") || !strcmp (*(p+1), "1"))
		exd->compositing = True;
	      else
		exd->compositing = False;
	    }

	  p += 2;
	}
    }

  if (!strcmp (tag, "client"))
    {
      MBWMXmlClient * c = mb_wm_xml_client_new ();
      const char **p = expat_attr;

      XmlCtx ctx = xml_stack_top_ctx (exd->stack);

      xml_stack_push (&exd->stack, XML_CTX_CLIENT);

      if (ctx != XML_CTX_THEME)
	{
	  MBWM_DBG ("Expected context theme");
	  return;
	}

      while (*p)
	{
	  if (!strcmp (*p, "type"))
	    {
	      if (!strcmp (*(p+1), "app"))
		c->type = MBWMClientTypeApp;
	      else if (!strcmp (*(p+1), "dialog"))
		c->type = MBWMClientTypeDialog;
	      else if (!strcmp (*(p+1), "panel"))
		c->type = MBWMClientTypePanel;
	      else if (!strcmp (*(p+1), "input"))
		c->type = MBWMClientTypeInput;
	      else if (!strcmp (*(p+1), "desktop"))
		c->type = MBWMClientTypeDesktop;
	      else if (!strcmp (*(p+1), "notification"))
		c->type = MBWMClientTypeNote;
	    }
	  else if (!strcmp (*p, "shaped"))
	    {
	      if (!strcmp (*(p+1), "yes") || !strcmp (*(p+1), "1"))
		c->shaped = 1;
	    }
	  else if (!strcmp (*p, "width"))
	    {
	      c->width = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "height"))
	    {
	      c->height = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "x"))
	    {
	      c->x = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "y"))
	    {
	      c->y = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "layout-hints") && *(p+1))
	    {
	      /* comma-separate list of hints */
	      char * duph = strdup (*(p+1));
	      char * comma;
	      char * h = duph;

	      while (h)
		{
		  comma = strchr (h, ',');

		  if (comma)
		    *comma = 0;

		  if (!strcmp (h, "reserve-edge-north"))
		    {
		      c->layout_hints |= LayoutPrefReserveEdgeNorth;
		    }
		  else if (!strcmp (h, "reserve-edge-south"))
		    {
		      c->layout_hints |= LayoutPrefReserveEdgeSouth;
		    }
		  else if (!strcmp (h, "reserve-edge-west"))
		    {
		      c->layout_hints |= LayoutPrefReserveEdgeWest;
		    }
		  else if (!strcmp (h, "reserve-edge-east"))
		    {
		      c->layout_hints |= LayoutPrefReserveEdgeEast;
		    }
		  if (!strcmp (h, "reserve-north"))
		    {
		      c->layout_hints |= LayoutPrefReserveNorth;
		    }
		  else if (!strcmp (h, "reserve-south"))
		    {
		      c->layout_hints |= LayoutPrefReserveSouth;
		    }
		  else if (!strcmp (h, "reserve-west"))
		    {
		      c->layout_hints |= LayoutPrefReserveWest;
		    }
		  else if (!strcmp (h, "reserve-east"))
		    {
		      c->layout_hints |= LayoutPrefReserveEast;
		    }
		  else if (!strcmp (h, "grow"))
		    {
		      c->layout_hints |= LayoutPrefGrowToFreeSpace;
		    }
		  else if (!strcmp (h, "free"))
		    {
		      c->layout_hints |= LayoutPrefPositionFree;
		    }
		  else if (!strcmp (h, "full-screen"))
		    {
		      c->layout_hints |= LayoutPrefFullscreen;
		    }
		  else if (!strcmp (h, "fixed-x"))
		    {
		      c->layout_hints |= LayoutPrefFixedX;
		    }
		  else if (!strcmp (h, "fixed-y"))
		    {
		      c->layout_hints |= LayoutPrefFixedY;
		    }
		  else if (!strcmp (h, "overlaps"))
		    {
		      c->layout_hints |= LayoutPrefOverlaps;
		    }

		  if (comma)
		    h = comma + 1;
		  else
		    break;
		}

	      free (duph);
	    }

	  p += 2;
	}

      if (!c->type)
	mb_wm_xml_client_free (c);
      else
	{
	  exd->xml_clients = mb_wm_util_list_prepend (exd->xml_clients, c);
	  xml_stack_top_set_data (exd->stack, c);
	}


      return;
    }

#if ENABLE_COMPOSITE
  if (!strcmp (tag, "effect"))
    {
      MBWMThemeEffects   *eff = mb_wm_util_malloc0 (sizeof (MBWMThemeEffects));
      const char        **p   = expat_attr;
      XmlCtx              ctx = xml_stack_top_ctx (exd->stack);
      MBWMXmlClient      *c   = xml_stack_top_data (exd->stack);

      xml_stack_push (&exd->stack, XML_CTX_EFFECT);

      if (ctx != XML_CTX_CLIENT || !c)
	{
	  MBWM_DBG ("Expected context client");
	  return;
	}

      while (*p)
	{
	  if (!strcmp (*p, "event"))
	    {
	      if (!strcmp (*(p+1), "minimize"))
		eff->event = MBWMCompMgrEffectEventMinimize;
	      else if (!strcmp (*(p+1), "map"))
		eff->event = MBWMCompMgrEffectEventMap;
	      else if (!strcmp (*(p+1), "unmap"))
		eff->event = MBWMCompMgrEffectEventUnmap;
	    }
	  else if (!strcmp (*p, "duration"))
	    eff->duration = atoi (*(p+1));
	  else if (!strcmp (*p, "gravity"))
	    {
	      const char * g = *(p+1);

	      if (!strncmp (g, "none", 2))
		eff->gravity = MBWMGravityNone;
	      else if (!strncmp (g, "nw", 2))
		eff->gravity = MBWMGravityNorthWest;
	      else if (!strncmp (g, "ne", 2))
		eff->gravity = MBWMGravityNorthEast;
	      else if (!strncmp (g, "sw", 2))
		eff->gravity = MBWMGravitySouthWest;
	      else if (!strncmp (g, "se", 2))
		eff->gravity = MBWMGravitySouthEast;
	      else
		{
		  switch (*g)
		    {
		    case 'n':
		      eff->gravity = MBWMGravityNorth; break;
		    case 's':
		      eff->gravity = MBWMGravitySouth; break;
		    case 'w':
		      eff->gravity = MBWMGravityWest;  break;
		    case 'e':
		      eff->gravity = MBWMGravityEast;  break;
		    default:
		      eff->gravity = MBWMGravityNone;
		    }
		}
	    }
	  else if (!strcmp (*p, "type"))
	    {
	      const char *e = *(p+1);

	      while (e && *e)
		{
		  char *bar;

		  if (!strncmp (e, "scale-up", 8))
		    {
		      eff->type |= MBWMCompMgrEffectScaleUp;
		    }
		  else if (!strncmp (e, "scale-down", 12))
		    {
		      eff->type |= MBWMCompMgrEffectScaleDown;
		    }
		  else if (!strncmp (e, "scale-down", 12))
		    {
		      eff->type |= MBWMCompMgrEffectScaleDown;
		    }
		  else if (!strncmp (e, "spin-xcw", 8))
		    {
		      eff->type |= MBWMCompMgrEffectSpinXCW;
		    }
		  else if (!strncmp (e, "spin-xccw", 9))
		    {
		      eff->type |= MBWMCompMgrEffectSpinXCCW;
		    }
		  else if (!strncmp (e, "spin-ycw", 8))
		    {
		      eff->type |= MBWMCompMgrEffectSpinYCW;
		    }
		  else if (!strncmp (e, "spin-yccw", 9))
		    {
		      eff->type |= MBWMCompMgrEffectSpinYCCW;
		    }
		  else if (!strncmp (e, "spin-zcw", 8))
		    {
		      eff->type |= MBWMCompMgrEffectSpinZCW;
		    }
		  else if (!strncmp (e, "spin-zccw", 9))
		    {
		      eff->type |= MBWMCompMgrEffectSpinZCCW;
		    }
		  else if (!strncmp (e, "fade", 4))
		    {
		      eff->type |= MBWMCompMgrEffectFade;
		    }
		  else if (!strncmp (e, "unfade", 4))
		    {
		      eff->type |= MBWMCompMgrEffectUnfade;
		    }
		  else if (!strncmp (e, "slide-in", 8))
		    {
		      eff->type |= MBWMCompMgrEffectSlideIn;
		    }
		  else if (!strncmp (e, "slide-out", 9))
		    {
		      eff->type |= MBWMCompMgrEffectSlideOut;
		    }

		  bar = strchr (e, '|');

		  if (bar)
		    e = bar + 1;
		  else
		    break;
		}
	    }

	  p += 2;
	}

      if (!eff->event || !eff->type)
	{
	  free (eff);
	  return;
	}

      /*
       * Default to sensible value when no duration set
       */
      if (!eff->duration)
	eff->duration = 200;

      c->effects = mb_wm_util_list_prepend (c->effects, eff);

      xml_stack_top_set_data (exd->stack, eff);

      return;
    }

  if (!strcmp (tag, "transition"))
    {
      MBWMThemeTransition *trs;
      XmlCtx               ctx = xml_stack_top_ctx (exd->stack);
      MBWMXmlClient       *c   = xml_stack_top_data (exd->stack);
      const char         **p   = expat_attr;

      trs = mb_wm_util_malloc0 (sizeof (MBWMThemeTransition));

      xml_stack_push (&exd->stack, XML_CTX_TRANSITION);

      if (ctx != XML_CTX_CLIENT || !c)
	{
	  MBWM_DBG ("Expected context client");
	  return;
	}

      while (*p)
	{
	  if (!strcmp (*p, "type"))
	    {
	      const char *e = *(p+1);

	      if (!strncmp (e, "fade", 4))
		{
		  trs->type = MBWMCompMgrTransitionFade;
		}
	      else if (!strncmp (e, "slide", 5))
		{
		  trs->type = MBWMCompMgrTransitionSlide;
		}
	      else if (!strcmp (*p, "gravity"))
		{
		  const char * g = *(p+1);

		  if (!strncmp (g, "none", 2))
		    trs->gravity = MBWMGravityNone;
		  else if (!strncmp (g, "nw", 2))
		    trs->gravity = MBWMGravityNorthWest;
		  else if (!strncmp (g, "ne", 2))
		    trs->gravity = MBWMGravityNorthEast;
		  else if (!strncmp (g, "sw", 2))
		    trs->gravity = MBWMGravitySouthWest;
		  else if (!strncmp (g, "se", 2))
		    trs->gravity = MBWMGravitySouthEast;
		  else
		    {
		      switch (*g)
			{
			case 'n':
			  trs->gravity = MBWMGravityNorth; break;
			case 's':
			  trs->gravity = MBWMGravitySouth; break;
			case 'w':
			  trs->gravity = MBWMGravityWest;  break;
			case 'e':
			  trs->gravity = MBWMGravityEast;  break;
			default:
			  trs->gravity = MBWMGravityNone;
			}
		    }
		}
	    }
	  else if (!strcmp (*p, "duration"))
	    trs->duration = atoi (*(p+1));

	  p += 2;
	}

      if (!trs->type)
	{
	  free (trs);
	  return;
	}

      /*
       * Default to sensible value when no duration set
       */
      if (!trs->duration)
	trs->duration = 200;

      c->transition = trs;

      xml_stack_top_set_data (exd->stack, trs);

      return;
    }

#endif

  if (!strcmp (tag, "decor"))
    {
      MBWMXmlDecor * d = mb_wm_xml_decor_new ();
      const char **p = expat_attr;
      XmlCtx ctx = xml_stack_top_ctx (exd->stack);
      MBWMXmlClient * c = xml_stack_top_data (exd->stack);

      xml_stack_push (&exd->stack, XML_CTX_DECOR);

      if (ctx != XML_CTX_CLIENT || !c)
	{
	  MBWM_DBG ("Expected context client");
	  return;
	}

      while (*p)
	{
	  if (!strcmp (*p, "clr-fg"))
	    mb_wm_xml_clr_from_string (&d->clr_fg, *(p+1));
	  else if (!strcmp (*p, "clr-bg"))
	    mb_wm_xml_clr_from_string (&d->clr_bg, *(p+1));
	  else if (!strcmp (*p, "clr-bg2"))
	    mb_wm_xml_clr_from_string (&d->clr_bg2, *(p+1));
	  else if (!strcmp (*p, "clr-frame"))
	    mb_wm_xml_clr_from_string (&d->clr_frame, *(p+1));
	  else if (!strcmp (*p, "type"))
	    {
	      if (!strcmp (*(p+1), "north"))
		d->type = MBWMDecorTypeNorth;
	      else if (!strcmp (*(p+1), "south"))
		d->type = MBWMDecorTypeSouth;
	      else if (!strcmp (*(p+1), "east"))
		d->type = MBWMDecorTypeEast;
	      else if (!strcmp (*(p+1), "west"))
		d->type = MBWMDecorTypeWest;
	    }
	  else if (!strcmp (*p, "width"))
	    {
	      d->width = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "height"))
	    {
	      d->height = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "x"))
	    {
	      d->x = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "y"))
	    {
	      d->y = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "font-size"))
	    {
	      d->font_size = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "font-family"))
	    {
	      d->font_family = strdup (*(p+1));
	    }
	  else if (!strcmp (*p, "show-title"))
	    {
	      if (!strcmp (*(p+1), "yes") || !strcmp (*(p+1), "1"))
		d->show_title = 1;
	    }

	  p += 2;
	}

      if (!d->type)
	mb_wm_xml_decor_free (d);
      else
	{
	  c->decors = mb_wm_util_list_prepend (c->decors, d);
	  xml_stack_top_set_data (exd->stack, d);
	}

      return;
    }

  if (!strcmp (tag, "button"))
    {
      MBWMXmlButton * b = mb_wm_xml_button_new ();
      const char **p = expat_attr;
      XmlCtx ctx = xml_stack_top_ctx (exd->stack);
      MBWMXmlDecor * d = xml_stack_top_data (exd->stack);

      xml_stack_push (&exd->stack, XML_CTX_BUTTON);

      if (ctx != XML_CTX_DECOR || !d)
	{
	  MBWM_DBG ("Expected context decor");
	  return;
	}

      while (*p)
	{
	  if (!strcmp (*p, "clr-fg"))
	    mb_wm_xml_clr_from_string (&b->clr_fg, *(p+1));
	  else if (!strcmp (*p, "clr-bg"))
	    mb_wm_xml_clr_from_string (&b->clr_bg, *(p+1));
	  else if (!strcmp (*p, "type"))
	    {
	      if (!strcmp (*(p+1), "minimize"))
		b->type = MBWMDecorButtonMinimize;
	      else if (!strcmp (*(p+1), "close"))
		b->type = MBWMDecorButtonClose;
	      else if (!strcmp (*(p+1), "menu"))
		b->type = MBWMDecorButtonMenu;
	      else if (!strcmp (*(p+1), "accept"))
		b->type = MBWMDecorButtonAccept;
	      else if (!strcmp (*(p+1), "fullscreen"))
		b->type = MBWMDecorButtonFullscreen;
	      else if (!strcmp (*(p+1), "help"))
		b->type = MBWMDecorButtonHelp;
	    }
	  else if (!strcmp (*p, "packing"))
	    {
	      if (!strcmp (*(p+1), "end"))
		b->packing = MBWMDecorButtonPackEnd;
	      else if (!strcmp (*(p+1), "start"))
		b->packing = MBWMDecorButtonPackStart;
	    }
	  else if (!strcmp (*p, "x"))
	    {
	      b->x = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "y"))
	    {
	      b->y = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "width"))
	    {
	      b->width = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "height"))
	    {
	      b->height = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "active-x"))
	    {
	      b->active_x = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "active-y"))
	    {
	      b->active_y = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "inactive-x"))
	    {
	      b->inactive_x = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "inactive-y"))
	    {
	      b->inactive_y = atoi (*(p+1));
	    }
	  else if (!strcmp (*p, "press-activated"))
	    {
	      if (!strcmp (*(p+1), "yes") || !strcmp (*(p+1), "1"))
		b->press_activated = 1;
	    }

	  p += 2;
	}

      if (!b->type)
	{
	  mb_wm_xml_button_free (b);
	  return;
	}

      d->buttons = mb_wm_util_list_append (d->buttons, b);

      xml_stack_top_set_data (exd->stack, b);

      return;
    }

  if (!strcmp (tag, "img"))
    {
      const char **p = expat_attr;
      XmlCtx ctx = xml_stack_top_ctx (exd->stack);

      xml_stack_push (&exd->stack, XML_CTX_IMG);

      if (ctx != XML_CTX_THEME)
	{
	  MBWM_DBG ("Expected context theme");
	  return;
	}

      while (*p)
	{
	  if (!strcmp (*p, "src"))
	    {
	      exd->img = strdup (*(p+1));
	      return;
	    }

	  p += 2;
	}

      return;
    }

}

static void
xml_element_end_cb (void *data, const char *tag)
{
  struct expat_data * exd = data;

  XmlCtx ctx = xml_stack_top_ctx (exd->stack);

  MBWM_DBG ("tag </%s>\n", tag);

  if (!strcmp (tag, "theme"))
    {
      XML_StopParser (exd->par, 0);
    }
  else if (!strcmp (tag, "client"))
    {
      if (ctx == XML_CTX_CLIENT)
	{
	  xml_stack_pop (&exd->stack);
	}
      else
	MBWM_DBG ("Expected client on the top of the stack!");
    }
  else if (!strcmp (tag, "effect"))
    {
      if (ctx == XML_CTX_EFFECT)
	{
	  xml_stack_pop (&exd->stack);
	}
      else
	MBWM_DBG ("Expected effect on the top of the stack!");
    }
  else if (!strcmp (tag, "transition"))
    {
      if (ctx == XML_CTX_TRANSITION)
	{
	  xml_stack_pop (&exd->stack);
	}
      else
	MBWM_DBG ("Expected transition on the top of the stack!");
    }
  else if (!strcmp (tag, "decor"))
    {
      if (ctx == XML_CTX_DECOR)
	{
	  xml_stack_pop (&exd->stack);
	}
      else
	MBWM_DBG ("Expected decor on the top of the stack!");
    }
  else if (!strcmp (tag, "button"))
    {
      if (ctx == XML_CTX_BUTTON)
	{
	  xml_stack_pop (&exd->stack);
	}
      else
	MBWM_DBG ("Expected button on the top of the stack!");
    }
  else if (!strcmp (tag, "img"))
    {
      if (ctx == XML_CTX_IMG)
	{
	  xml_stack_pop (&exd->stack);
	}
      else
	MBWM_DBG ("Expected img on the top of the stack!");
    }
}

