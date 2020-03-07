/*  $Id: cache.h 8014 2008-09-07 11:54:42Z iulius $
**
**  Message-ID to storage token cache.
*/

#ifndef CACHE_H
#define CACHE_H

#include "inn/libinn.h"
#include "inn/storage.h"

BEGIN_DECLS

void cache_add(const HASH, const TOKEN);
TOKEN cache_get(const HASH, bool final);

END_DECLS

#endif /* CACHE_H */
