/*  $Id: cnfs.h 8817 2009-11-17 18:57:19Z iulius $
**
**  Storage manager module header for Cyclic News File System method.
*/

#ifndef __CNFS_H__
#define __CNFS_H__

bool cnfs_init(SMATTRIBUTE *attr);
TOKEN cnfs_store(const ARTHANDLE article, const STORAGECLASS class);
ARTHANDLE *cnfs_retrieve(const TOKEN token, const RETRTYPE amount);
ARTHANDLE *cnfs_next(ARTHANDLE *article, const RETRTYPE amount);
void cnfs_freearticle(ARTHANDLE *article);
bool cnfs_cancel(TOKEN token);
bool cnfs_ctl(PROBETYPE type, TOKEN *token, void *value);
bool cnfs_flushcacheddata(FLUSHTYPE type);
void cnfs_printfiles(FILE *file, TOKEN token, char **xref, int ngroups);
char *cnfs_explaintoken(const TOKEN token);
void cnfs_shutdown(void);

#endif
