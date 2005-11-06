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

#ifndef _HAVE_MB_WM_H
#define _HAVE_MB_WM_H

#define _GNU_SOURCE 		/* For vasprintf */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>          /* for XA_ATOM etc */
#include <X11/keysym.h>         /* key mask defines */

#include "xas.h"    		/* async stuff not needed for xlib on xcb */

#define MBWM_WANT_DEBUG 1

#if (MBWM_WANT_DEBUG)
#define MBWM_DBG(x, a...) \
 fprintf(stderr, __FILE__ ":%d,%s() " x "\n", __LINE__, __func__, ##a)
#else
#define MBWM_DBG(x, a...) do {} while (0)
#endif
#define MBWM_MARK() MBWM_DBG("--mark--");

#define MBWM_WANT_ASSERT 0

#if (MBWM_WANT_ASSERT)
#include <assert.h>
#define MBWM_ASSERT(x) assert(x)
#else
#define MBWM_ASSERT(x) do {} while (0)
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#include "mb-wm-types.h"
#include "mb-wm-atoms.h"
#include "mb-wm-props.h"
#include "mb-wm-util.h"
#include "mb-wm-core.h"
#include "mb-wm-keys.h"
#include "mb-wm-client.h"
#include "mb-wm-layout.h"
#include "mb-wm-stack.h"


#endif
