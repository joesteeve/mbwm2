/* 
 *  Matchbox Window Manager II - A lightweight window manager not for the
 *                               desktop.
 *
 *  Authored By Matthew Allum <mallum@o-hand.com>
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

#ifndef _HAVE_MB_WM_DECOR_H
#define _HAVE_MB_WM_DECOR_H

#define MB_WM_DECOR(c) ((MBWMDecor*)(c)) 
#define MB_WM_DECOR_CLASS(c) ((MBWMDecorClass*)(c)) 
#define MB_WM_TYPE_DECOR (mb_wm_decor_class_type ())

typedef struct MBWMDecor       MBWMDecor;
typedef struct MBWMDecorClass  MBWMDecorClass;


#define MB_WM_DECOR_BUTTON(c) ((MBWMDecorButton*)(c)) 
#define MB_WM_DECOR_BUTTON_CLASS(c) ((MBWMDecorButtonClass*)(c)) 
#define MB_WM_TYPE_DECOR_BUTTON (mb_wm_decor_button_class_type ())


typedef struct MBWMDecorButton MBWMDecorButton;
typedef struct MBWMDecorButtonClass MBWMDecorButtonClass;

typedef void (*MBWMDecorResizedFunc) (MBWindowManager   *wm,
				      MBWMDecor         *decor,
				      void              *userdata);

typedef void (*MBWMDecorRepaintFunc) (MBWindowManager   *wm,
				      MBWMDecor         *decor,
				      void              *userdata);

typedef void (*MBWMDecorButtonRepaintFunc) (MBWindowManager   *wm,
					    MBWMDecorButton   *button,
					    void              *userdata);

typedef void (*MBWMDecorButtonPressedFunc) (MBWindowManager   *wm,
					    MBWMDecorButton   *button,
					    void              *userdata);

typedef void (*MBWMDecorButtonReleasedFunc) (MBWindowManager   *wm,
					     MBWMDecorButton   *button,
					     void              *userdata);


struct MBWMDecor
{
  MBWMObject             parent;
  MBWMDecorType          type;
  Window                 xwin;
  MBWindowManagerClient *parent_client;
  MBGeometry             geom;
  Bool                   dirty;
  MBWMDecorResizedFunc   resize;
  MBWMDecorRepaintFunc   repaint;
  void                  *userdata;
  MBWMList              *buttons;
};

struct MBWMDecorClass
{
  MBWMObjectClass        parent;
};


MBWMDecor*
mb_wm_decor_new (MBWindowManager     *wm,
		 MBWMDecorType        type,
		 MBWMDecorResizedFunc resize,
		 MBWMDecorRepaintFunc repaint,
		 void                *userdata);

static Bool
mb_wm_decor_reparent (MBWMDecor *decor);

static void
mb_wm_decor_calc_geometry (MBWMDecor *decor);

void
mb_wm_decor_handle_repaint (MBWMDecor *decor);

void
mb_wm_decor_handle_resize (MBWMDecor *decor);

Window
mb_wm_decor_get_x_window (MBWMDecor *decor);

MBWMDecorType
mb_wm_decor_get_type (MBWMDecor *decor);

const MBGeometry*
mb_wm_decor_get_geometry (MBWMDecor *decor);

MBWindowManagerClient*
mb_wm_decor_get_parent (MBWMDecor *decor);

void
mb_wm_decor_mark_dirty (MBWMDecor *decor);

void
mb_wm_decor_attach (MBWMDecor             *decor,
		    MBWindowManagerClient *client);

void
mb_wm_decor_detach (MBWMDecor *decor);

typedef enum MBWMDecorButtonState 
{
  MBWMDecorButtonStateInactive = 0,
  MBWMDecorButtonStatePressed

} MBWMDecorButtonState ;

struct MBWMDecorButton
{
  MBWMObject                  parent;
  MBWMDecor                  *decor;
  Window                      xwin;

  MBGeometry                  geom;

  /* in priv ? */
  Bool                        visible;
  Bool                        needs_sync;
  MBWMDecorButtonState        state;

  MBWMDecorButtonRepaintFunc  repaint;
  MBWMDecorButtonPressedFunc  press;
  MBWMDecorButtonReleasedFunc release;
  void                       *userdata;
};

struct MBWMDecorButtonClass
{
  MBWMObjectClass   parent;

  /* 
     show;
     hide;
     realize; ??
  */
};

void
mb_wm_decor_button_show (MBWMDecorButton *button);

void
mb_wm_decor_button_hide (MBWMDecorButton *button);

void
mb_wm_decor_button_move_to (MBWMDecorButton *button, int x, int y);

MBWMDecorButton*
mb_wm_decor_button_new (MBWindowManager            *wm,
			MBWMDecor                  *decor,
			int                         width,
			int                         height,
			MBWMDecorButtonPressedFunc  press,
			MBWMDecorButtonReleasedFunc release,
			MBWMDecorButtonRepaintFunc  paint,
			MBWMDecorButtonFlags        flags,
			void                       *userdata);


#endif
