/*  $Id: xfopena.c 7585 2006-11-21 09:37:51Z eagle $
**
*/

#include "config.h"
#include "clibrary.h"
#include <fcntl.h>

#include "inn/libinn.h"

/*
**  Open a file in append mode.  Since not all fopen's set the O_APPEND
**  flag, we do it by hand.
*/
FILE *xfopena(const char *p)
{
    int		fd;

    /* We can't trust stdio to really use O_APPEND, so open, then fdopen. */
    fd = open(p, O_WRONLY | O_APPEND | O_CREAT, 0666);
    return fd >= 0 ? fdopen(fd, "a") : NULL;
}
