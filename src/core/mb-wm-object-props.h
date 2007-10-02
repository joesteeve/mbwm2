
#ifndef _HAVE_MB_OBJECT_PROPS_H
#define _HAVE_MB_OBJECT_PROPS_H

#include "mb-wm-types.h"

/*
 * MBWMObject construction properties
 *
 * NB: the properties are only used at construction time, being passed to
 * mb_wm_object_new (); they cannot be set or queried subsequently.
 *
 * Property ids are numerical; this allows us (a) to avoid excessive use of
 * strcmp during object creation (passing 6 properties to the constructor
 * would result in each _init function doing 21 strcmp() call to retrieve it's
 * values), and (b) handle unknown properties safely.
 *
 * Property arguments can only be of the following types:
 *
 *     int,
 *     long,
 *     long long,
 *     void*,
 *
 * plus their unsigned variants.
 *
 * The property id is a 32-bit value, constructed as follows:
 *
 * Bits 31-4  : numerical id that uniquely identifies this property. Bits 31-24
 *              are reserved for private properties of any objects built out of
 *              tree to avoid clashing with default properties; with default
 *              properties bits 31-24 are always 0 (_MBWMObjectPropLastGlobal
 *              represent the highest numerical id of a default property).
 *
 * Bits 3-0   : Size of the property argument, as returned by sizeof().
 *
 * Since properties always come in id-value pairs, when an object _init()
 * function encounters a property it does not know, it needs to eat the
 * argument, the MBWMO_PROP_EAT() macro is provided for this purpose.
 */
#define _MKOPROP(n, type) (((1<<4)+(n<<4))|sizeof(type))

#define MBWMO_PROP_EAT(_ovap, prop)       \
do                                        \
{                                         \
  int size = (prop & 0x0000000f);         \
                                          \
  if (size == sizeof (int))               \
    va_arg (_ovap, int);                  \
  else if (size == sizeof (void *))       \
    va_arg (_ovap, void *);               \
  else if (size == sizeof (long))         \
    va_arg (_ovap, long);                 \
  else if (size == sizeof (long long))    \
    va_arg (_ovap, long long);            \
}while (0)

typedef enum MBWMObjectProp
  {
    MBWMObjectPropWidth                   = _MKOPROP(0,  int),
    MBWMObjectPropHeight                  = _MKOPROP(1,  int),
    MBWMObjectPropXwindow                 = _MKOPROP(2,  Window),
    MBWMObjectPropArgc                    = _MKOPROP(3,  int),
    MBWMObjectPropArgv                    = _MKOPROP(4,  void*),
    MBWMObjectPropWm                      = _MKOPROP(5,  void*),
    MBWMObjectPropClientWindow            = _MKOPROP(6,  void*),

    MBWMObjectPropDecor                   = _MKOPROP(7,  void*),
    MBWMObjectPropDecorType               = _MKOPROP(8,  MBWMDecorType),
    MBWMObjectPropDecorUserData           = _MKOPROP(9,  void*),

    MBWMObjectPropDecorButtonRepaintFunc  = _MKOPROP(10, void*),
    MBWMObjectPropDecorButtonPressedFunc  = _MKOPROP(11, void*),
    MBWMObjectPropDecorButtonReleasedFunc = _MKOPROP(12, void*),
    MBWMObjectPropDecorButtonFlags        = _MKOPROP(13, MBWMDecorButtonFlags),
    MBWMObjectPropDecorButtonUserData     = _MKOPROP(14, void*),
    MBWMObjectPropDecorButtonType         = _MKOPROP(15, int),
    MBWMObjectPropDecorButtonPack         = _MKOPROP(16, int),

    MBWMObjectPropThemePath               = _MKOPROP(17, void*),

    _MBWMObjectPropLastGlobal = 0x00fffff0,
  }
MBWMObjectProp;

#undef _MKOPROP

#endif
