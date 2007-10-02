#include "mb-wm-theme-png.h"
#include "mb-wm-theme-xml.h"

#include <math.h>
#include <png.h>

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
  t_class->create_decor     = mb_wm_theme_png_create_decor;

#ifdef MBWM_WANT_DEBUG
  klass->klass_name = "MBWMThemePng";
#endif
}

static void
mb_wm_theme_png_destroy (MBWMObject *obj)
{
  MBWMThemePng * theme = MB_WM_THEME_PNG (obj);

  if (theme->ximg)
    XFree (theme->ximg);
}

static int
mb_wm_theme_png_init (MBWMObject *obj, va_list vap)
{
  MBWMThemePng * theme = MB_WM_THEME_PNG (obj);

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

static void
mb_wm_theme_png_paint_decor (MBWMTheme *theme,
			     MBWMDecor *decor)
{
}

static void
mb_wm_theme_png_paint_button (MBWMTheme *theme, MBWMDecorButton *button)
{
}

static MBWMDecor *
mb_wm_theme_png_create_decor (MBWMTheme             *theme,
			      MBWindowManagerClient *client,
			      MBWMDecorType          type)
{
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
  struct Client * c;
  struct Decor  * d;

  /* FIXME -- assumes button on the north decor only */
  if ((c = client_find_by_type (theme->xml_clients, c_type)) &&
      (d = decor_find_by_type (c->decors, decor->type)))
    {
      struct Button * b = button_find_by_type (d->buttons, type);

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
  struct Client * c;
  struct Decor  * d;

  /* FIXME -- assumes button on the north decor only */
  if ((c = client_find_by_type (theme->xml_clients, c_type)) &&
      (d = decor_find_by_type (c->decors, decor->type)))
    {
      struct Button * b = button_find_by_type (d->buttons, type);

      if (b)
	{
	  if (x)
	    *x = b->x;

	  if (y)
	    *y = b->y;

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
  struct Client * c;
  struct Decor  * d;

  /* FIXME -- assumes button on the north decor only */
  if ((c = client_find_by_type (theme->xml_clients, c_type)))
    {
      if (north)
	{
	  d = decor_find_by_type (c->decors, MBWMDecorTypeNorth);

	  if (d)
	    *north = d->height;
	  else
	    *north = 0;
	}

      if (south)
	{
	  d = decor_find_by_type (c->decors, MBWMDecorTypeSouth);

	  if (d)
	    *south = d->height;
	  else
	    *south = 0;
	}

      if (west)
	{
	  d = decor_find_by_type (c->decors, MBWMDecorTypeWest);

	  if (d)
	    *west = d->width;
	  else
	    *west = 0;
	}

      if (east)
	{
	  d = decor_find_by_type (c->decors, MBWMDecorTypeEast);

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

