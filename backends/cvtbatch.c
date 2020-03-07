/*  $Id: cvtbatch.c 9623 2014-03-15 22:29:46Z iulius $
**  Read file list on standard input and spew out batch files.
*/

#include "config.h"
#include "clibrary.h"

#include "inn/innconf.h"
#include "inn/messages.h"
#include "inn/qio.h"
#include "inn/wire.h"
#include "inn/libinn.h"
#include "inn/paths.h"
#include "inn/storage.h"


int
main(int ac, char *av[]) {
    int		i;
    QIOSTATE	*qp;
    char	*line;
    const char	*text;
    char	*format;
    char	*p, *q;
    const char	*r;
    bool	Dirty;
    TOKEN	token;
    ARTHANDLE	*art;
    size_t	len;
    time_t      arrived;

    /* First thing, set up our identity. */
    message_program_name = "cvtbatch";
    if (!innconf_read(NULL))
        exit(1);

    /* Parse JCL. */
    format = xstrdup("nm");
    while ((i = getopt(ac, av, "w:")) != EOF)
	switch (i) {
	default:
            die("usage error");
            break;
	case 'w':
            free(format);
	    for (p = format = optarg; *p; p++) {
		switch (*p) {
		case FEED_BYTESIZE:
                case FEED_TIMERECEIVED:
		case FEED_FULLNAME:
		case FEED_MESSAGEID:
		case FEED_NAME:
		    continue;
		}
                warn("ignoring %c in -w flag", *p);
	    }
	}
    ac -= optind;
    if (ac)
	die("usage error");

    if (!SMinit())
        die("cannot initialize storage manager: %s", SMerrorstr);

    /* Loop over all input. */
    qp = QIOfdopen((int)fileno(stdin));
    while ((line = QIOread(qp)) != NULL) {
	for (p = line; *p; p++)
	    if (ISWHITE(*p)) {
		*p = '\0';
		break;
	    }

	if (!IsToken(line))
	    continue;
	token = TextToToken(line);
	if ((art = SMretrieve(token, RETR_ALL)) == NULL)
	    continue;
        text = wire_findheader(art->data, art->len, "Message-ID", true);
	if (text == NULL) {
	    SMfreearticle(art);
	    continue;
	}
	len = art->len;
        arrived = art->arrived;

	for (r = text; r < art->data + art->len; r++) {
	    if (*r == '\r' || *r == '\n')
		break;
	}
	if (r == art->data + art->len) {
	    SMfreearticle(art);
	    continue;
	}
	q = xmalloc(r - text + 1);
	memcpy(q, text, r - text);
	SMfreearticle(art);
	q[r - text] = '\0';

	/* Write the desired info. */
	for (Dirty = false, p = format; *p; p++) {
	    switch (*p) {
	    default:
		continue;
	    case FEED_BYTESIZE:
		if (Dirty)
		    putchar(' ');
		printf("%lu", (unsigned long)len);
		break;
	    case FEED_FULLNAME:
	    case FEED_NAME:
		if (Dirty)
		    putchar(' ');
		printf("%s", line);
		break;
	    case FEED_MESSAGEID:
		if (Dirty)
		    putchar(' ');
		printf("%s", q);
		break;
            case FEED_TIMERECEIVED:
                if (Dirty)
                    putchar(' ');
                printf("%lu", (unsigned long) arrived);
                break;
	    }
	    Dirty = true;
	}
	free(q);
	if (Dirty)
	    putchar('\n');
    }

    exit(0);
    /* NOTREACHED */
}
