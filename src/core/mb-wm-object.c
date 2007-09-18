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
static int                   ObjectClassesAllocated = 0;
static int                   NObjectClasses = 0;

#ifdef MBWM_WANT_DEBUG
/*
 * Increased for each ref call and decreased for each unref call
 */
static int object_count = 0;
int
mb_wm_object_get_object_count (void)
{
  return object_count;
}
#endif

#define N_CLASSES_PREALLOC 10
#define N_CLASSES_REALLOC_STEP  5

void
mb_wm_object_init(void)
{
  ObjectClasses     = mb_wm_util_malloc0 (sizeof(void*) * N_CLASSES_PREALLOC);
  ObjectClassesInfo = mb_wm_util_malloc0 (sizeof(void*) * N_CLASSES_PREALLOC);

  if (ObjectClasses && ObjectClassesInfo)
    ObjectClassesAllocated = N_CLASSES_PREALLOC;
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

int
mb_wm_object_register_class (MBWMObjectClassInfo *info,
			     int                  parent_type,
			     int                  flags)
{
  MBWMObjectClass *klass;

  if (NObjectClasses >= ObjectClassesAllocated)
    {
      int byte_len;
      int new_offset;
      int new_byte_len;

      new_offset = ObjectClassesAllocated;
      ObjectClassesAllocated += N_CLASSES_REALLOC_STEP;

      byte_len     = sizeof(void *) * (ObjectClassesAllocated);
      new_byte_len = sizeof(void *) * (ObjectClassesAllocated - new_offset);

      ObjectClasses     = realloc (ObjectClasses,     byte_len);
      ObjectClassesInfo = realloc (ObjectClassesInfo, byte_len);

      if (!ObjectClasses || !ObjectClassesInfo)
	return 0;

      memset (ObjectClasses + new_offset    , 0, new_byte_len);
      memset (ObjectClassesInfo + new_offset, 0, new_byte_len);
    }

  ObjectClassesInfo[NObjectClasses] = info;

  klass             = mb_wm_util_malloc0(info->klass_size);
  klass->init       = info->instance_init;
  klass->destroy    = info->instance_destroy;
  klass->class_init = info->class_init;
  klass->type       = NObjectClasses + 1;

  if (parent_type != 0)
    klass->parent = ObjectClasses[parent_type-1];

  ObjectClasses[NObjectClasses] = klass;

  mb_wm_object_class_init (klass);

  return 1 + NObjectClasses++;
}

void *
mb_wm_object_ref (MBWMObject *this)
{
  if (!this)
    {
      MBWM_DBG("### Warning: called with NULL ###");
      return this;
    }

#ifdef MBWM_WANT_DEBUG
  object_count++;
#endif

  this->refcnt++;

  MBWM_TRACE_MSG (OBJ_REF, "### REF ###");

  return this;
}

static void
mb_wm_object_destroy_recursive (const MBWMObjectClass * klass,
				MBWMObject *this)
{
  /* Destruction needs to happen top to bottom */
  MBWMObjectClass *parent_klass = klass->parent;

  if (klass->destroy)
    klass->destroy (this);

  if (parent_klass)
    mb_wm_object_destroy_recursive (parent_klass, this);
}

void
mb_wm_object_unref (MBWMObject *this)
{
  if (!this)
    {
      MBWM_DBG("### Warning: called with NULL ###");
      return;
    }

  MBWM_TRACE_MSG (OBJ_UNREF, "### UNREF ###");

#ifdef MBWM_WANT_DEBUG
  object_count--;
  if (object_count < 0)
    {
      /* Note that the trace is not necessarily pointing to the code that is
       * at fault, but at least it gives us a terminal point before which the
       * bad unref happened.
       */
      MBWM_TRACE_MSG (OBJ_UNREF, "### Unbalanced unref ###");
    }
#endif

  this->refcnt--;

  if (this->refcnt == 0)
    {
      MBWM_NOTE (OBJ_UNREF, "=== DESTROYING OBJECT type %d ===",
		 this->klass->type);

      mb_wm_object_destroy_recursive (MB_WM_OBJECT_GET_CLASS (this),
				      this);
    }
}

static void
mb_wm_object_init_recurse (MBWMObject *obj, MBWMObjectClass *parent,
			   va_list vap)
{
  va_list vap2;

  va_copy (vap2, vap);

  if (parent->parent)
    mb_wm_object_init_recurse (obj, parent->parent, vap2);

  if (parent->init)
    parent->init (obj, vap);

  va_end (vap2);
}

static void
mb_wm_object_init_object (MBWMObject *obj, va_list vap)
{
  va_list vap2;

  va_copy(vap2, vap);

  if (obj->klass->parent)
    mb_wm_object_init_recurse (obj, obj->klass->parent, vap2);

  if (obj->klass->init)
    obj->klass->init(obj, vap);

  va_end(vap2);
}


MBWMObject*
mb_wm_object_new (int type, ...)
{
  MBWMObjectClassInfo *info;
  MBWMObject          *obj;
  va_list              vap;

  va_start(vap, type);

  info = ObjectClassesInfo[type-1];

  obj = mb_wm_util_malloc0 (info->instance_size);

  obj->klass = MB_WM_OBJECT_CLASS(ObjectClasses[type-1]);

  mb_wm_object_init_object (obj, vap);

  mb_wm_object_ref (obj);

  va_end(vap);

  return obj;
}

const MBWMObjectClass*
mb_wm_object_get_class (MBWMObject *this)
{
  return this->klass;
}

unsigned long
mb_wm_object_signal_connect (MBWMObject             *obj,
			     unsigned long           signal,
			     MBWMObjectCallbackFunc  func,
			     void                   *userdata)
{
  static unsigned long id_counter = 0;
  MBWMFuncInfo *func_info;

  MBWM_ASSERT(func != NULL);

  func_info           = mb_wm_util_malloc0(sizeof(MBWMFuncInfo));
  func_info->func     = (void*)func;
  func_info->userdata = userdata;
  func_info->data     = mb_wm_object_ref (obj);
  func_info->signal   = signal;
  func_info->id       = id_counter++;

  obj->callbacks =
    mb_wm_util_list_append (obj->callbacks, func_info);

  return func_info->id;
}

void
mb_wm_object_signal_disconnect (MBWMObject    *obj,
				unsigned long  id)
{
  MBWMList  *item = obj->callbacks;

  while (item)
    {
      MBWMFuncInfo* info = item->data;

      if (info->id == id)
	{
	  MBWMList * prev = item->prev;
	  MBWMList * next = item->next;

	  if (prev)
	    prev->next = next;
	  else
	    obj->callbacks = next;

	  if (next)
	    next->prev = prev;

	  mb_wm_object_unref (MB_WM_OBJECT (info->data));

	  free (info);
	  free (item);

	  return;
	}

      item = item->next;
    }

  MBWM_DBG ("### Warning: did not find signal handler %d ###", id);
}

void
mb_wm_object_signal_emit (MBWMObject    *obj,
			  unsigned long  signal)
{
    MBWMList  *item = obj->callbacks;

    while (item)
      {
	MBWMFuncInfo* info = item->data;

	if (info->signal & signal)
	  {
	    if (((MBWMObjectCallbackFunc)info->func) (obj,
						      signal,
						      info->userdata))
	      {
		break;
	      }
	  }

	item = item->next;
      }
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
