/*  $Id: mmap.h 7599 2007-02-09 02:46:39Z eagle $
**
**  Manipulation routines for memory-mapped pages.
**
**  Written by Alex Kiernan (alex.kiernan@thus.net)
*/

#ifndef INN_MMAP_H
#define INN_MMAP_H 1

#include <inn/defines.h>

BEGIN_DECLS

/* msync the page containing a section of memory.  This is the internal
   function, which we wrap with a define below. */
int inn__msync_page(void *, size_t, int flags);

/* Some platforms only support two arguments to msync.  On those platforms,
   make the third argument to msync_page always be zero, getting rid of
   whatever the caller tried to pass.  This avoids undefined symbols for
   MS_ASYNC and friends on platforms with two-argument msync functions. */
#ifdef INN_HAVE_MSYNC_3_ARG
# define inn_msync_page inn__msync_page
#else
# define inn_msync_page(p, l, f) inn__msync_page((p), (l), 0)
#endif

END_DECLS

#endif /* INN_MMAP_H */
