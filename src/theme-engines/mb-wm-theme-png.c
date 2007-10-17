#include "mb-wm-theme-png.h"
#include "mb-wm-theme-xml.h"

#include <math.h>
#include <png.h>

#include <X11/Xft/Xft.h>

static int
mb_wm_theme_png_ximg (MBWMThemePng * theme, const char * img);

static unsigned char*
mb_wm_theme_png_load_file (const char *file, int *width, int *height);

static void
mb_wm_theme_png_paint_decor (MBWMTheme *theme, MBWMDecor *decor);

static void
mb_wm_theme_png_paint_button (MBWMTheme *theme, MBWMDecorButton *button);

static void
mb_wm_theme_png_get_decor_dimensions (MBWMTheme *, MBWindowManagerClient *,
				      int*, int*, int*, int*);

static MBWMDecor *
mb_wm_theme_png_create_decor (MBWMTheme*, MBWindowManagerClient *,
			      MBWMDecorType);

static void
mb_wm_theme_png_get_button_size (MBWMTheme *, MBWMDecor *, MBWMDecorButtonType,
				 int *, int *);

static void
mb_wm_theme_png_get_button_position (MBWMTheme *, MBWMDecor *,
				     MBWMDecorButtonType,
				     int *, int *);

static void
mb_wm_theme_png_class_init (MBWMObjectClass *klass)
{
  MBWMThemeClass *t_class = MB_WM_THEME_CLASS (klass);

  t_class->paint_decor      = mb_wm_theme_png_paint_decor;
  t_class->paint_button     = mb_wm_theme_png_paint_button;
  t_class->decor_dimensions = mb_wm_theme_png_get_decor_dimensions;
  t_class->button_size      = mb_wm_theme_png_get_button_size;
  t_class->button_position  = mb_wm_theme_png_get_button_position;
  t_class->create_decor     = mb_wm_theme_png_create_decor;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMThemePng";
#endif
}

static void
mb_wm_theme_png_destroy (MBWMObject *obj)
{
  MBWMThemePng * theme = MB_WM_THEME_PNG (obj);
  Display * dpy = MB_WM_THEME (obj)->wm->xdpy;

  XRenderFreePicture (dpy, theme->xpic);
  XFreePixmap (dpy, theme->xdraw);
}

static int
mb_wm_theme_png_init (MBWMObject *obj, va_list vap)
{
  MBWMThemePng     *theme = MB_WM_THEME_PNG (obj);
  MBWMObjectProp    prop;
  char             *img = NULL;

  prop = va_arg(vap, MBWMObjectProp);
  while (prop)
    {
      switch (prop)
	{
	case MBWMObjectPropThemeImg:
	  img = va_arg(vap, char *);
	  break;
	default:
	  MBWMO_PROP_EAT (vap, prop);
	}

      prop = va_arg(vap, MBWMObjectProp);
    }

  if (!img || !mb_wm_theme_png_ximg (theme, img))
    return 0;

  return 1;
}

int
mb_wm_theme_png_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (MBWMThemePngClass),
	sizeof (MBWMThemePng),
	mb_wm_theme_png_init,
	mb_wm_theme_png_destroy,
	mb_wm_theme_png_class_init
      };

      type = mb_wm_object_register_class (&info, MB_WM_TYPE_THEME, 0);
    }

  return type;
}

struct DecorData
{
  Pixmap    xpix;
  XftDraw  *xftdraw;
  XftColor  clr;
  XftFont  *font;
};

static void
decordata_free (MBWMDecor * decor, void *data)
{
  struct DecorData * dd = data;
  Display * xdpy = decor->parent_client->wmref->xdpy;

  XFreePixmap (xdpy, dd->xpix);
  XftDrawDestroy (dd->xftdraw);

  if (dd->font)
    XftFontClose (xdpy, dd->font);

  free (dd);
}

struct ButtonData
{
  Pixmap    xpix_i;
  XftDraw  *xftdraw_i;
  Pixmap    xpix_a;
  XftDraw  *xftdraw_a;
};

static void
buttondata_free (MBWMDecorButton * button, void *data)
{
  struct ButtonData * bd = data;
  Display * xdpy = button->decor->parent_client->wmref->xdpy;

  XFreePixmap (xdpy, bd->xpix_i);
  XftDrawDestroy (bd->xftdraw_i);
  XFreePixmap (xdpy, bd->xpix_a);
  XftDrawDestroy (bd->xftdraw_a);

  free (bd);
}

static XftFont *
xft_load_font(MBWMDecor * decor, MBWMXmlDecor *d)
{
  char desc[512];
  XftFont *font;
  Display * xdpy = decor->parent_client->wmref->xdpy;
  int       xscreen = decor->parent_client->wmref->xscreen;

  snprintf (desc, sizeof (desc), "%s-%i",
	    d->font_family ? d->font_family : "Sans",
	    d->font_size ? d->font_size : 18);

  font = XftFontOpenName (xdpy, xscreen, desc);

  return font;
}

static void
mb_wm_theme_png_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
  MBWMDecor              * decor;
  MBWMThemePng           * p_theme = MB_WM_THEME_PNG (theme);
  MBWindowManagerClient  * client;
  MBWMClientType           c_type;;
  MBWMXmlClient          * c;
  MBWMXmlDecor           * d;
  MBWMXmlButton          * b;

  /*
   * We do not paint inactive buttons, as they get painted with the decor
   */
  decor  = button->decor;
  client = mb_wm_decor_get_parent (decor);
  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);

  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) &&
      (d = mb_wm_xml_decor_find_by_type (c->decors, decor->type))      &&
      (b = mb_wm_xml_button_find_by_type (d->buttons, button->type)))
    {
      Display * xdpy    = theme->wm->xdpy;
      int       xscreen = theme->wm->xscreen;
      struct DecorData  * ddata = mb_wm_decor_get_theme_data (decor);
      struct ButtonData * bdata;
      int x, y;

      if (!ddata)
	return;

      bdata = mb_wm_decor_button_get_theme_data (button);

      if (!bdata)
	{
	  XRenderColor rclr;

	  bdata = malloc (sizeof (struct ButtonData));

	  bdata->xpix_a = XCreatePixmap(xdpy, decor->xwin,
				      button->geom.width, button->geom.height,
				      DefaultDepth(xdpy, xscreen));

	  bdata->xftdraw_a = XftDrawCreate (xdpy, bdata->xpix_a,
					    DefaultVisual (xdpy, xscreen),
					    DefaultColormap (xdpy, xscreen));

	  rclr.red   = 0x7fff;
	  rclr.green = 0x7fff;
	  rclr.blue  = 0x7fff;
	  rclr.alpha = 0x9fff;

	  if (b->clr_fg.set)
	    {
	      rclr.red   = (int)(d->clr_fg.r * (double)0xffff);
	      rclr.green = (int)(d->clr_fg.g * (double)0xffff);
	      rclr.blue  = (int)(d->clr_fg.b * (double)0xffff);
	    }

	  XRenderComposite (xdpy, PictOpSrc,
			    p_theme->xpic,
			    None,
			    XftDrawPicture (bdata->xftdraw_a),
			    b->x, b->y, 0, 0, 0, 0, b->width, b->height);

	  XRenderFillRectangle (xdpy, PictOpOver,
				XftDrawPicture (bdata->xftdraw_a),
				&rclr,
				0, 0, b->width, b->height);

	  bdata->xpix_i = XCreatePixmap(xdpy, decor->xwin,
				      button->geom.width, button->geom.height,
				      DefaultDepth(xdpy, xscreen));

	  bdata->xftdraw_i = XftDrawCreate (xdpy, bdata->xpix_i,
					    DefaultVisual (xdpy, xscreen),
					    DefaultColormap (xdpy, xscreen));

	  XRenderComposite (xdpy, PictOpSrc,
			    p_theme->xpic,
			    None,
			    XftDrawPicture (bdata->xftdraw_i),
			    b->x, b->y, 0, 0, 0, 0, b->width, b->height);

	  mb_wm_decor_button_set_theme_data (button, bdata, buttondata_free);
	}

      x = b->x - d->x;
      y = b->y - d->y;

      XRenderComposite (xdpy, PictOpSrc,
			button->state == MBWMDecorButtonStatePressed ?
			XftDrawPicture (bdata->xftdraw_a) :
			XftDrawPicture (bdata->xftdraw_i),
			None,
			XftDrawPicture (ddata->xftdraw),
			0, 0, 0, 0, x, y, b->width, b->height);

      XClearWindow (xdpy, decor->xwin);
    }
}

static void
mb_wm_theme_png_paint_decor (MBWMTheme *theme, MBWMDecor *decor)
{
  MBWMThemePng           * p_theme = MB_WM_THEME_PNG (theme);
  MBWindowManagerClient  * client = decor->parent_client;
  MBWMClientType           c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMXmlClient          * c;
  MBWMXmlDecor           * d;

  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) &&
      (d = mb_wm_xml_decor_find_by_type (c->decors, decor->type)))
    {
      Display * xdpy    = theme->wm->xdpy;
      int       xscreen = theme->wm->xscreen;
      struct DecorData * data = mb_wm_decor_get_theme_data (decor);
      const char * title;
      int x, y;

      if (!data)
	{
	  XRenderColor rclr;

	  data = malloc (sizeof (struct DecorData));
	  data->xpix = XCreatePixmap(xdpy, decor->xwin,
				     decor->geom.width, decor->geom.height,
				     DefaultDepth(xdpy, xscreen));

	  data->xftdraw = XftDrawCreate (xdpy, data->xpix,
					 DefaultVisual (xdpy, xscreen),
					 DefaultColormap (xdpy, xscreen));

	  rclr.red = 0;
	  rclr.green = 0;
	  rclr.blue  = 0;
	  rclr.alpha = 0xffff;

	  if (d->clr_fg.set)
	    {
	      rclr.red   = (int)(d->clr_fg.r * (double)0xffff);
	      rclr.green = (int)(d->clr_fg.g * (double)0xffff);
	      rclr.blue  = (int)(d->clr_fg.b * (double)0xffff);
	    }

	  XftColorAllocValue (xdpy, DefaultVisual (xdpy, xscreen),
			      DefaultColormap (xdpy, xscreen),
			      &rclr, &data->clr);

	  data->font = xft_load_font (decor, d);

	  XSetWindowBackgroundPixmap(xdpy, decor->xwin, data->xpix);

	  mb_wm_decor_set_theme_data (decor, data, decordata_free);
	}

      for (x = 0; x < decor->geom.width; x += d->width)
	for (y = 0; y < decor->geom.height; y += d->height)
	  {
	    XRenderComposite(xdpy, PictOpSrc,
			     p_theme->xpic,
			     None,
			     XftDrawPicture (data->xftdraw),
			     d->x, d->y, 0, 0, x, y, d->width, d->height);
	  }

      title = mb_wm_client_get_name (client);

      if (title && data->font)
	{
	  XRectangle rec;

	  int pack_start_x = mb_wm_decor_get_pack_start_x (decor);
	  int pack_end_x = mb_wm_decor_get_pack_end_x (decor);
	  int west_width = mb_wm_client_frame_west_width (client);
	  int y = (decor->geom.height -
		   (data->font->ascent + data->font->descent)) / 2
	    + data->font->ascent;

	  rec.x = 0;
	  rec.y = 0;
	  rec.width = pack_end_x - 2;
	  rec.height = d->height;

	  XftDrawSetClipRectangles (data->xftdraw, 0, 0, &rec, 1);

	  XftDrawStringUtf8(data->xftdraw,
			    &data->clr,
			    data->font,
			    west_width + pack_start_x, y,
			    title, strlen (title));
	}

      XClearWindow (xdpy, decor->xwin);
    }
}

static void
construct_buttons (MBWMThemePng * theme, MBWMDecor * decor, MBWMXmlDecor *d)
{
  MBWindowManagerClient *client = decor->parent_client;
  MBWindowManager       *wm     = client->wmref;
  MBWMDecorButton       *button;

  if (d)
    {
      MBWMList * l = d->buttons;
      while (l)
	{
	  MBWMXmlButton * b = l->data;

	  button = mb_wm_decor_button_stock_new (wm,
						 b->type,
						 b->packing,
						 decor,
						 0);

	  mb_wm_decor_button_show (button);
	  mb_wm_object_unref (MB_WM_OBJECT (button));

	  l = l->next;
	}
    }
}

static MBWMDecor *
mb_wm_theme_png_create_decor (MBWMTheme             *theme,
			      MBWindowManagerClient *client,
			      MBWMDecorType          type)
{
  MBWMClientType   c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMDecor       *decor = NULL;
  MBWindowManager *wm = client->wmref;
  MBWMXmlClient   *c;

  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)))
    {
      MBWMXmlDecor *d;

      d = mb_wm_xml_decor_find_by_type (c->decors, type);

      if (d)
	{
	  decor = mb_wm_decor_new (wm, type);
	  decor->absolute_packing = True;
	  mb_wm_decor_attach (decor, client);
	  construct_buttons (MB_WM_THEME_PNG (theme), decor, d);
	}
    }

  return decor;
}

static void
mb_wm_theme_png_get_button_size (MBWMTheme             *theme,
				 MBWMDecor             *decor,
				 MBWMDecorButtonType    type,
				 int                   *width,
				 int                   *height)
{
  MBWindowManagerClient * client = decor->parent_client;
  MBWMClientType  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMThemePng  *p_theme = MB_WM_THEME_PNG (theme);
  MBWMXmlClient * c;
  MBWMXmlDecor  * d;

  /* FIXME -- assumes button on the north decor only */
  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) &&
      (d = mb_wm_xml_decor_find_by_type (c->decors, decor->type)))
    {
      MBWMXmlButton * b = mb_wm_xml_button_find_by_type (d->buttons, type);

      if (b)
	{
	  if (width)
	    *width = b->width;

	  if (height)
	    *height = b->height;

	  return;
	}
    }

  if (width)
    *width = 0;

  if (height)
    *height = 0;
}

static void
mb_wm_theme_png_get_button_position (MBWMTheme             *theme,
				     MBWMDecor             *decor,
				     MBWMDecorButtonType    type,
				     int                   *x,
				     int                   *y)
{
  MBWindowManagerClient * client = decor->parent_client;
  MBWMClientType  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMThemePng  *p_theme = MB_WM_THEME_PNG (theme);
  MBWMXmlClient * c;
  MBWMXmlDecor  * d;

  /* FIXME -- assumes button on the north decor only */
  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)) &&
      (d = mb_wm_xml_decor_find_by_type (c->decors, decor->type)))
    {
      MBWMXmlButton * b = mb_wm_xml_button_find_by_type (d->buttons, type);

      if (b)
	{
	  if (x)
	    *x = b->x - d->x - client->frame_geometry.x;

	  if (y)
	    *y = b->y - d->y - client->frame_geometry.y;

	  return;
	}
    }

  if (x)
    *x = 0;

  if (y)
    *y = 0;
}

static void
mb_wm_theme_png_get_decor_dimensions (MBWMTheme             *theme,
				      MBWindowManagerClient *client,
				      int                   *north,
				      int                   *south,
				      int                   *west,
				      int                   *east)
{
  MBWMClientType  c_type = MB_WM_CLIENT_CLIENT_TYPE (client);
  MBWMThemePng  *p_theme = MB_WM_THEME_PNG (theme);
  MBWMXmlClient * c;
  MBWMXmlDecor  * d;

  /* FIXME -- assumes button on the north decor only */
  if ((c = mb_wm_xml_client_find_by_type (theme->xml_clients, c_type)))
    {
      if (north)
	{
	  d = mb_wm_xml_decor_find_by_type (c->decors, MBWMDecorTypeNorth);

	  if (d)
	    *north = d->height;
	  else
	    *north = 0;
	}

      if (south)
	{
	  d = mb_wm_xml_decor_find_by_type (c->decors, MBWMDecorTypeSouth);

	  if (d)
	    *south = d->height;
	  else
	    *south = 0;
	}

      if (west)
	{
	  d = mb_wm_xml_decor_find_by_type (c->decors, MBWMDecorTypeWest);

	  if (d)
	    *west = d->width;
	  else
	    *west = 0;
	}

      if (east)
	{
	  d = mb_wm_xml_decor_find_by_type (c->decors, MBWMDecorTypeEast);

	  if (d)
	    *east = d->width;
	  else
	    *east = 0;
	}

      return;
    }

  if (north)
    *north = 0;

  if (south)
    *south = 0;

  if (west)
    *west = 0;

  if (east)
    *east = 0;
}

/*
 * From matchbox-keyboard
 */
static unsigned char*
mb_wm_theme_png_load_file (const char *file,
			   int        *width,
			   int        *height)
{
  FILE *fd;
  unsigned char *data;
  unsigned char header[8];
  int  bit_depth, color_type;

  png_uint_32  png_width, png_height, i, rowbytes;
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep *row_pointers;

  if ((fd = fopen (file, "rb")) == NULL)
    return NULL;

  fread (header, 1, 8, fd);
  if (!png_check_sig (header, 8))
    {
      fclose(fd);
      return NULL;
    }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
    {
      fclose(fd);
      return NULL;
    }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
    {
      png_destroy_read_struct (&png_ptr, NULL, NULL);
      fclose(fd);
      return NULL;
    }

  if (setjmp (png_ptr->jmpbuf))
    {
      png_destroy_read_struct( &png_ptr, &info_ptr, NULL);
      fclose(fd);
      return NULL;
    }

  png_init_io (png_ptr, fd);
  png_set_sig_bytes (png_ptr, 8);
  png_read_info (png_ptr, info_ptr);
  png_get_IHDR (png_ptr, info_ptr, &png_width, &png_height, &bit_depth,
		&color_type, NULL, NULL, NULL);

  *width  = (int) png_width;
  *height = (int) png_height;

  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  if (bit_depth < 8)
    png_set_packing(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY  ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  /* Add alpha */
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_RGB)
    png_set_add_alpha (png_ptr, 0xff, PNG_FILLER_AFTER);

  if (color_type == PNG_COLOR_TYPE_PALETTE ||
      png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_expand (png_ptr);

  png_read_update_info (png_ptr, info_ptr);

  /* allocate space for data and row pointers */
  rowbytes = png_get_rowbytes (png_ptr, info_ptr);
  data = (unsigned char*) malloc ((rowbytes * (*height + 1)));
  row_pointers = (png_bytep *) malloc ((*height) * sizeof (png_bytep));

  if (!data || !row_pointers)
    {
      png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

      if (data)
	free (data);

      if (row_pointers)
	free (row_pointers);

      return NULL;
    }

  for (i = 0;  i < *height; i++)
    row_pointers[i] = data + i * rowbytes;

  png_read_image (png_ptr, row_pointers);
  png_read_end (png_ptr, NULL);

  free (row_pointers);
  png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
  fclose(fd);

  return data;
}

static int
mb_wm_theme_png_ximg (MBWMThemePng * theme, const char * img)
{
  MBWindowManager * wm = MB_WM_THEME (theme)->wm;
  Display * dpy = wm->xdpy;
  int       screen = wm->xscreen;

  XImage * ximg;
  GC       gc;
  int x;
  int y;
  int width;
  int height;
  XRenderPictFormat       *ren_fmt;
  XRenderPictureAttributes ren_attr;
  unsigned char * p;
  unsigned char * png_data = mb_wm_theme_png_load_file (img, &width, &height);

  if (!png_data || !width || !height)
    return 0;

  ren_fmt = XRenderFindStandardFormat(dpy, PictStandardARGB32);

  theme->xdraw =
    XCreatePixmap (dpy, RootWindow(dpy,screen), width, height, ren_fmt->depth);

  XSync (dpy, False);

  ren_attr.dither          = True;
  ren_attr.component_alpha = True;
  ren_attr.repeat          = False;

  gc = XCreateGC (dpy, theme->xdraw, 0, NULL);

  ximg = XCreateImage(dpy, DefaultVisual (dpy, screen),
		      ren_fmt->depth, ZPixmap,
		      0, NULL, width, height, 32, 0);

  ximg->data = malloc (ximg->bytes_per_line * ximg->height);

  p = png_data;

  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      {
	unsigned char a, r, g, b;
	r = *p++; g = *p++; b = *p++; a = *p++;
	r = (r * (a + 1)) / 256;
	g = (g * (a + 1)) / 256;
	b = (b * (a + 1)) / 256;
	XPutPixel(ximg, x, y, (a << 24) | (r << 16) | (g << 8) | b);
      }

  XPutImage (dpy, theme->xdraw, gc, ximg, 0, 0, 0, 0, width, height);

  theme->xpic = XRenderCreatePicture (dpy, theme->xdraw, ren_fmt,
				      CPRepeat|CPDither|CPComponentAlpha,
				      &ren_attr);

  free (ximg->data);
  ximg->data = NULL;
  XDestroyImage (ximg);

  XFreeGC (dpy, gc);

  free (png_data);

  return 1;
}
