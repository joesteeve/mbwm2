#ifndef _HAVE_MB_WM_THEME_PRIVATE_H
#define _HAVE_MB_WM_THEME_PRIVATE_H

#include "mb-wm.h"

/*
 * Helper structs for xml theme
 */
struct Clr
{
  double r;
  double g;
  double b;

  Bool set;
};

struct Button
{
  MBWMDecorButtonType type;
  MBWMDecorButtonPack packing;

  struct Clr clr_fg;
  struct Clr clr_bg;

  int x;
  int y;
  int width;
  int height;
};

struct Decor
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
};

struct Client
{
  MBWMClientType  type;
  MBWMList       *decors;
};

struct Button *
button_new ();

void
button_free (struct Button * b);

struct Decor *
decor_new ();

void
decor_free (struct Decor * d);

struct Client *
client_new ();

void
client_free (struct Client * c);

struct Client *
client_find_by_type (MBWMList *l, MBWMClientType type);

struct Decor *
decor_find_by_type (MBWMList *l, MBWMDecorType type);

struct Button *
button_find_by_type (MBWMList *l, MBWMDecorButtonType type);

void
clr_from_string (struct Clr * clr, const char *s);

#endif
