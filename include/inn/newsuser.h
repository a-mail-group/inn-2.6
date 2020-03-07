/*  $Id: newsuser.h 9782 2015-01-07 21:34:22Z iulius $
**
**  Declarations of functions to ensure running as "news" user/group.
**
**  By Ivan Shmakov, 2007.
**  This code is in the public domain.
*/

#ifndef INN_NEWSUSER_H
#define INN_NEWSUSER_H 1

#include "inn/portable-macros.h"
#include "inn/portable-stdbool.h"

BEGIN_DECLS

int get_news_uid_gid(uid_t *uid, gid_t *gid, bool may_die);

/* The following functions die() on failure. */
void ensure_news_user(bool may_setuid);
void ensure_news_grp(bool may_setgid);
void ensure_news_user_grp(bool may_setuid, bool may_setgid);

END_DECLS

#endif /* INN_NEWSUSER_H */
