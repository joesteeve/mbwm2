#ifndef _HAVE_MB_WM_THEME_PRIVATE_H
#define _HAVE_MB_WM_THEME_PRIVATE_H

#include "mb-wm.h"

/*
 * Helper structs for xml theme
 */
typedef struct Clr
{
  double r;
  double g;
  double b;

  Bool set;
}MBWMXmlClr;

typedef struct Button
{
  MBWMDecorButtonType type;
  MBWMDecorButtonPack packing;

  struct Clr clr_fg;
  struct Clr clr_bg;

  int x;
  int y;
  int width;
  int height;
} MBWMXmlButton;

typedef struct Decor
{
  MBWMDecorType type;

  struct Clr clr_fg;
  struct Clr clr_bg;
  struct Clr clr_bg2;
  struct Clr clr_frame;

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
mb_wm_xml_clr_from_string (MBWMXmlClr * clr, const char *s);

#endif
