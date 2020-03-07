/* $Id: reallocarray-t.c 10246 2018-02-16 21:12:42Z iulius $
 *
 * reallocarray test suite.
 *
 * This does some simple sanity checks and checks some of the overflow
 * detection, but isn't particularly thorough.
 *
 * The canonical version of this file is maintained in the rra-c-util package,
 * which can be found at <https://www.eyrie.org/~eagle/software/rra-c-util/>.
 *
 * Written by Russ Allbery <eagle@eyrie.org>
 * Copyright 2014 Russ Allbery <eagle@eyrie.org>
 * Copyright 2014
 *     The Board of Trustees of the Leland Stanford Junior University
 *
 * Copying and distribution of this file, with or without modification, are
 * permitted in any medium without royalty provided the copyright notice and
 * this notice are preserved.  This file is offered as-is, without any
 * warranty.
 *
 * SPDX-License-Identifier: FSFAP
 */

#define LIBTEST_NEW_FORMAT 1

#include "config.h"
#include "clibrary.h"

#include <errno.h>

#include "tap/basic.h"

void *test_reallocarray(void *, size_t, size_t);


int
main(void)
{
    char *p, *base;
    size_t sqrt_max;
    int oerrno;

    plan(15);

    /* Test success cases and write to the memory for valgrind checks. */
    p = test_reallocarray(NULL, 2, 5);
    memcpy(p, "123456789", 10);
    is_string("123456789", p, "reallocarray of NULL");
    p = test_reallocarray(p, 4, 5);
    is_string("123456789", p, "reallocarray after resize");
    memcpy(p + 9, "0123456789", 11);
    is_string("1234567890123456789", p, "write to larger memory segment");
    free(p);

    /*
     * If nmemb or size are 0, we should either get NULL or a pointer we can
     * free.  Make sure we don't get something weird, like division by zero.
     */
    p = test_reallocarray(NULL, 0, 100);
    if (p != NULL)
        free(p);
    p = test_reallocarray(NULL, 100, 0);
    if (p != NULL)
        free(p);

    /* Test the range-checking error cases. */
    p = test_reallocarray(NULL, 2, SIZE_MAX / 2);
    oerrno = errno;
    ok(p == NULL, "reallocarray fails for 2, SIZE_MAX / 2");
    is_int(ENOMEM, oerrno, "...with correct errno");
    base = malloc(10);
    p = test_reallocarray(base, 3, SIZE_MAX / 3);
    oerrno = errno;
    ok(p == NULL, "reallocarray fails for 3, SIZE_MAX / 3");
    is_int(ENOMEM, oerrno, "...with correct errno");
    sqrt_max = (1UL << (sizeof(size_t) * 4));
    p = test_reallocarray(base, sqrt_max, sqrt_max);
    oerrno = errno;
    ok(p == NULL, "reallocarray fails for sqrt(SIZE_MAX), sqrt(SIZE_MAX)");
    is_int(ENOMEM, oerrno, "...with correct errno");
    p = test_reallocarray(base, 1, SIZE_MAX);
    oerrno = errno;
    ok(p == NULL, "reallocarray fails for 1, SIZE_MAX");
    is_int(ENOMEM, oerrno, "...with correct errno");
    p = test_reallocarray(base, SIZE_MAX, 1);
    oerrno = errno;
    ok(p == NULL, "reallocarray fails for SIZE_MAX, 1");
    is_int(ENOMEM, oerrno, "...with correct errno");
    p = test_reallocarray(base, 2, SIZE_MAX);
    oerrno = errno;
    ok(p == NULL, "reallocarray fails for 2, SIZE_MAX");
    is_int(ENOMEM, oerrno, "...with correct errno");

    /* Clean up and exit. */
    free(base);
    return 0;
}
