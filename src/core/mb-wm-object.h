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

#ifndef _HAVE_MB_OBJECT_H
#define _HAVE_MB_OBJECT_H

typedef struct MBWMObject      MBWMObject;
typedef struct MBWMObjectClass MBWMObjectClass;

typedef void (*MBWMObjFunc)   (MBWMObject* obj);
typedef void (*MBWMClassFunc) (MBWMObjectClass* klass);

#define MB_WM_TYPE_OBJECT 0
#define MB_WM_OBJECT(x) ((MBWMObject*)(x))
#define MB_WM_OBJECT_CLASS(x) ((MBWMObjectClass*)(x))
#define MB_WM_OBJECT_TYPE(x) (((MBWMObject*)(x))->klass->type)

typedef enum  MBWMObjectClassType
{
  MB_WM_OBJECT_TYPE_CLASS     = 0,
  MB_WM_OBJECT_TYPE_ABSTRACT,
  MB_WM_OBJECT_TYPE_SINGLETON
}
MBWMObjectClassType;

typedef struct MBWMObjectClassInfo
{
  size_t              klass_size;
  size_t              instance_size;
  MBWMObjFunc         instance_init;
  MBWMObjFunc         instance_destroy;
  MBWMClassFunc       class_init;
}
MBWMObjectClassInfo;

struct MBWMObjectClass
{
  int              type;
  MBWMObjectClass *parent;
  MBWMObjFunc      init;
  MBWMObjFunc      destroy;
  MBWMClassFunc    class_init;
};

struct MBWMObject
{
  MBWMObjectClass *klass;
  int              refcnt;
};

void
mb_wm_object_init(void);

int
mb_wm_object_register_class (MBWMObjectClassInfo *info,
			     int                  parent_type,
			     int                  flags);

MBWMObject *
mb_wm_object_ref (MBWMObject *this);

void
mb_wm_object_unref (MBWMObject *this);

MBWMObject*
mb_wm_object_new (int type);

const MBWMObjectClass*
mb_wm_object_get_class (MBWMObject *this);


#endif
