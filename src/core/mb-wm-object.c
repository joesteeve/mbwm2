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

#include "mb-wm.h"

static MBWMObjectClassInfo **ObjectClassesInfo  = NULL;
static MBWMObjectClass     **ObjectClasses  = NULL;
static int                   NObjectClasses = 0;

void
mb_wm_object_init(void)
{
#define N_CLASSES_PREALLOC 10

  ObjectClasses = mb_wm_util_malloc0(sizeof(MBWMObjectClass *) * N_CLASSES_PREALLOC);
  ObjectClassesInfo = mb_wm_util_malloc0(sizeof(MBWMObjectClassInfo *) * N_CLASSES_PREALLOC);
}

int
mb_wm_object_register_class (MBWMObjectClassInfo *info, 
			     int                  parent_type,
			     int                  flags)
{
  MBWMObjectClass *klass;

  /* FIXME realloc() */

  printf("%s() called\n", __func__);

  ObjectClassesInfo[NObjectClasses] = info; 
  
  klass             = mb_wm_util_malloc0(info->klass_size);
  klass->init       = info->instance_init;
  klass->destroy    = info->instance_destroy;
  klass->class_init = info->class_init;  
  klass->type       = NObjectClasses + 1;

  if (parent_type != 0)
    klass->parent = ObjectClasses[parent_type-1];

  ObjectClasses[NObjectClasses] = klass;

  return 1 + NObjectClasses++;
}

void
mb_wm_object_ref (MBWMObject *this) 
{
  this->refcnt++;
}

void
mb_wm_object_unref (MBWMObject *this) 
{
  this->refcnt--;

  if (this->refcnt == 0)
    {
      if (this->klass->destroy)
	this->klass->destroy(this);
    }
}

static void
mb_wm_object_class_init_recurse (MBWMObjectClass *klass, 
				 MBWMObjectClass *parent)
{
  if (parent->parent)
    mb_wm_object_class_init_recurse (klass, parent->parent);

  if (parent->class_init)
    parent->class_init (klass);
}

static void
mb_wm_object_class_init (MBWMObjectClass *klass)
{
  if (klass->parent)
    mb_wm_object_class_init_recurse (klass, klass->parent);
  
  if (klass->class_init)
    klass->class_init (klass);
}

static void
mb_wm_object_init_recurse (MBWMObject *obj, MBWMObjectClass *parent)
{
  if (parent->parent)
    mb_wm_object_init_recurse (obj, parent->parent);

  if (parent->init)
    parent->init (obj);
}


MBWMObject*
mb_wm_object_new (int type) 
{
  MBWMObjectClassInfo *info;
  MBWMObject          *obj; 

  info = ObjectClassesInfo[type-1];

  obj = mb_wm_util_malloc0 (info->instance_size);

  obj->klass = MB_WM_OBJECT_CLASS(ObjectClasses[type-1]);

  mb_wm_object_class_init (obj->klass);

  if (obj->klass->parent)
    mb_wm_object_init_recurse (obj, obj->klass->parent);

  if (obj->klass->init)
    obj->klass->init(obj);

  mb_wm_object_ref (obj);

  return obj;
}

const MBWMObjectClass*
mb_wm_object_get_class (MBWMObject *this) 
{
  return this->klass;
}

#if 0

/* ----- Test code -------- */

typedef struct Foo
{
  MBWMObject parent;

  int        hello;
}
Foo;

typedef struct FooClass
{
  MBWMObjectClass parent;

}
FooClass;

void
mb_wm_foo_init (MBWMObject *obj)
{
  printf("%s() called\n", __func__);

}

void
mb_wm_foo_destroy (MBWMObject *obj)
{
  printf("%s() called\n", __func__);
}

int
mb_wm_foo_get_class_type ()
{
  static int type = 0;

  if (UNLIKELY(type == 0))
    {
      static MBWMObjectClassInfo info = {
	sizeof (FooClass),      
	sizeof (Foo),  		/* Instance */
	mb_wm_foo_init,
	mb_wm_foo_destroy,
	NULL
      };

      type = mb_wm_object_register_class (&info);

      printf("type: %i\n", type);
    }

  return type;
}

Foo*
mb_wm_foo_new (int val)
{
  Foo *foo;
  
  foo = (Foo*)mb_wm_object_new (mb_wm_foo_get_class_type ());

  /* call init */
  
  foo->hello = val;

  return foo;
}


int
main (int argc, char **argv)
{
  Foo *foo, *foo2;

  mb_wm_object_init();

  printf("%s() called init, about to call new\n", __func__);

  foo = mb_wm_foo_new (10);
  foo2 = mb_wm_foo_new (10);

  printf("%s() foo->hello is %i\n", __func__, foo->hello);

  mb_wm_object_unref (MB_WM_OBJECT(foo));
}

#endif
