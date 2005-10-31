#ifndef _MB_HAVE_UTIL_H
#define _MB_HAVE_UTIL_H

#include "mb-wm.h"

/* See http://rlove.org/log/2005102601 */
#if __GNUC__ >= 3
# define inlineinline __attribute__ ((always_inline))
# define __pure__attribute__ ((pure))
# define __const__attribute__ ((const))
# define __noreturn__attribute__ ((noreturn))
# define __malloc__attribute__ ((malloc))
# define __must_check__attribute__ ((warn_unused_result))
# define __deprecated__attribute__ ((deprecated))
# define __used__attribute__ ((used))
# define __unused__attribute__ ((unused))
# define __packed__attribute__ ((packed))
# define likely(x)__builtin_expect (!!(x), 1)
# define unlikely(x)__builtin_expect (!!(x), 0)
#else
# define inline/* no inline */
# define __pure/* no pure */
# define __const/* no const */
# define __noreturn/* no noreturn */
# define __malloc/* no malloc */
# define __must_check/* no warn_unused_result */
# define __deprecated/* no deprecated */
# define __used/* no used */
# define __unused/* no unused */
# define __packed/* no packed */
# define likely(x)(x)
# define unlikely(x)(x)
#endif

#define streq(a,b)      (strcmp(a,b) == 0)
#define strcaseeq(a,b)  (strcasecmp(a,b) == 0)
#define unless(x)       if (!(x))

void*
mb_wm_util_malloc0(int size);

void
mb_wm_util_fatal_error(char *msg);

/* XErrors */

void
mb_wm_util_trap_x_errors(void);

int
mb_wm_util_untrap_x_errors(void);

/* List */

typedef struct MBList MBList;

typedef void (*MBListForEachCB) (void *data, void *userdata);

struct MBList 
{
  MBList *next, *prev;
  void *data;
};

#define mb_wm_util_list_next(list) (list)->next
#define mb_wm_util_list_prev(list) (list)->prev
#define mb_wm_util_list_data(data) (list)->data

MBList *
mb_wm_util_list_alloc_item(void);

int
mb_wm_util_list_length(MBList *list);

MBList*
mb_wm_util_list_get_last(MBList *list);

MBList*
mb_wm_util_list_get_first(MBList *list);

void*
mb_wm_util_list_get_nth_data(MBList *list, int n);

MBList*
mb_wm_util_list_append(MBList *list, void *data);

void
mb_wm_util_list_foreach(MBList *list, MBListForEachCB func, void *userdata);

#endif
















