#ifndef _HAVE_MB_WM_THEME_PRIVATE_H
#define _HAVE_MB_WM_THEME_PRIVATE_H

#include "mb-wm.h"

/*
 * Helper structs for xml theme
 */
typedef struct Button
{
  MBWMDecorButtonType type;
  MBWMDecorButtonPack packing;

  MBWMColor clr_fg;
  MBWMColor clr_bg;

  int x;
  int y;
  int width;
  int height;

  /* Needed by png themes */
  int active_x;
  int active_y;
} MBWMXmlButton;

typedef struct Decor
{
  MBWMDecorType type;

  MBWMColor clr_fg;
  MBWMColor clr_bg;
  MBWMColor clr_bg2;
  MBWMColor clr_frame;

  int x;
  int y;
  int width;
  int height;
  int font_size;

  char * font_family;

  MBWMList * buttons;
}MBWMXmlDecor;

typedef struct Client
{
  MBWMClientType  type;

  int x;
  int y;
  int width;
  int height;

  MBWMList       *decors;
}MBWMXmlClient;

MBWMXmlButton *
mb_wm_xml_button_new ();

void
mb_wm_xml_button_free (MBWMXmlButton * b);

MBWMXmlDecor *
mb_wm_xml_decor_new ();

void
mb_wm_xml_decor_free (MBWMXmlDecor * d);

MBWMXmlClient *
mb_wm_xml_client_new ();

void
mb_wm_xml_client_free (MBWMXmlClient * c);

MBWMXmlClient *
mb_wm_xml_client_find_by_type (MBWMList *l, MBWMClientType type);

MBWMXmlDecor *
mb_wm_xml_decor_find_by_type (MBWMList *l, MBWMDecorType type);

MBWMXmlButton *
mb_wm_xml_button_find_by_type (MBWMList *l, MBWMDecorButtonType type);

void
mb_wm_xml_clr_from_string (MBWMColor * clr, const char *s);

#endif
