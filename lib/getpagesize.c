/*  $Id: getpagesize.c 5596 2002-08-17 21:29:35Z rra $
**
**  Replacement for a missing getpagesize.
**
**  Provides getpagesize implemented in terms of sysconf for those systems
**  that don't have the getpagesize function.  Defaults to a page size of 16KB
**  if sysconf isn't available either.
*/

#include "config.h"
#include <unistd.h>

int
getpagesize(void)
{
    int pagesize;

#ifdef _SC_PAGESIZE
    pagesize = sysconf(_SC_PAGESIZE);
#else
    pagesize = 16 * 1024;
#endif
    return pagesize;
}
