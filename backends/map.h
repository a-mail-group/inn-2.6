/*  $Id: map.h 5292 2002-03-10 08:59:54Z vinocur $
**
*/

void  MAPfree(void);                    /* free the map */
void  MAPread(const char *name);        /* read the map file */
char *MAPname(char *p);                 /* lookup in the map */
