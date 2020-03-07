/*  $Id: cache.c 8709 2009-11-06 21:47:11Z iulius $
**
**  Message-ID to storage token cache.
**
**  Written by Alex Kiernan (alex.kiernan@thus.net).
**
**  Implementation of a message-ID to storage token cache which can be
**  built during (X)OVER/(X)HDR/XPAT/NEWNEWS.  If we hit in the cache when
**  retrieving articles, the (relatively) expensive cost of a trip
**  through the history database is saved.
*/

#include "config.h"
#include "clibrary.h"

#include "inn/innconf.h"
#include "inn/tst.h"
#include "inn/list.h"
#include "inn/libinn.h"
#include "inn/storage.h"

#include "cache.h"

/*
**  Pointer to the message-ID to storage token ternary search tree.
*/
static struct tst *msgidcache;

/*
**  Count of message-IDs in the cache so that someone doing GROUP,
**  (X)OVER, GROUP, (X)OVER, etc. for example doesn't blow up with
**  out of memory.
*/
static unsigned long msgcachecount;

struct cache_entry {
    struct node node;
    HASH hash;
    TOKEN token;
};

static struct list unused, used;

/*
**  Add a translation from HASH, h, to TOKEN, t, to the message-ID
**  cache.
*/
void
cache_add(const HASH h, const TOKEN t)
{
    if (innconf->msgidcachesize != 0) {
	struct cache_entry *entry, *old;
	const unsigned char *p;
        void *exist;

	if (!msgidcache) {
	    msgidcache = tst_init((innconf->msgidcachesize + 9) / 10);
	    list_new(&unused);
	    list_new(&used);
	}

	entry = xmalloc(sizeof *entry);
	entry->hash = h;
	entry->token = t;
	p = (unsigned char *) HashToText(h);
	if (tst_insert(msgidcache, p, entry, 0, &exist) == TST_DUPLICATE_KEY) {
	    free(entry);
            old = exist;
	    list_remove(&old->node);
	    list_addtail(&unused, &old->node);
	} else {
	    list_addtail(&unused, &entry->node);
	    ++msgcachecount;
	}
	if (msgcachecount >= innconf->msgidcachesize) {
	    /* Need to throw away a node. */
	    entry = (struct cache_entry *)list_remhead(&used);
	    if (entry == NULL)
		entry = (struct cache_entry *)list_remhead(&unused);
	    if (entry != NULL) {
		tst_delete(msgidcache,
			   (unsigned char *) HashToText(entry->hash));
		free(entry);
	    }
	}
    }
}


/*
**  Lookup (and remove if found) a message-ID to TOKEN mapping.  If this
**  is a final lookup (ARTICLE, BODY, (X)HDR, XPAT), we remove it if we
**  find it since this matches the observed behaviour of most clients, but
**  cache it just in case we can reuse it if they issue multiple
**  commands against the same message-ID (e.g. HEAD, STAT).
*/
TOKEN
cache_get(const HASH h, bool final)
{
    static HASH last_hash;
    static TOKEN last_token;
    static const TOKEN empty_token = { TOKEN_EMPTY, 0, "" };

    if (HashCompare(&h, &last_hash) == 0 && !HashEmpty(last_hash))
	return last_token;

    if (msgidcache) {
	struct cache_entry *entry;

	entry = tst_search(msgidcache, (unsigned char *) HashToText(h));
	if (entry != NULL) {
	    list_remove(&entry->node);
	    if (!final)
		list_addtail(&unused, &entry->node);
	    else
		list_addtail(&used, &entry->node);
	    last_hash = entry->hash;
	    last_token = entry->token;
	    return last_token;
	}
    }
    return empty_token;
}
