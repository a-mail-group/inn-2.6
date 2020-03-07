/*  $Id: conffile.h 9782 2015-01-07 21:34:22Z iulius $
**
**  Data structures, functions and cetera used for config file parsing.
*/

#include "portable/macros.h"

BEGIN_DECLS

typedef struct {
    FILE *f;
    char *buf;
    unsigned int sbuf;
    int lineno;
    int array_len;
    char **array;
    char *filename;
} CONFFILE;

typedef struct {
    int type;
#define CONFstring	-1
    char *name;
} CONFTOKEN;

extern char CONFerror[];

extern CONFFILE *CONFfopen(const char *);
extern void CONFfclose(CONFFILE *);

extern CONFTOKEN *CONFgettoken(CONFTOKEN *, CONFFILE *);

END_DECLS
