/*  $Id: newsfeeds.c 10122 2016-11-25 22:39:35Z iulius $
**
**  Routines for the in-core data structures for the newsfeeds file.
*/

#include "config.h"
#include "clibrary.h"

#include "inn/innconf.h"
#include "innd.h"

/*
** List of variables assigned in the configuration file.
*/
typedef struct _SITEVARIABLES {
    char      *Name;
    char      *Value;
    int               Elements;
    struct _SITEVARIABLES     *Next;
} SITEVARIABLES;

/* The character which introduces a variable assignment or reference. */
#define VARIABLE_CHAR '$'

static SITE	SITEnull;
static char	*SITEfeedspath = NULL;
static SITEVARIABLES  *SITEvariables = NULL;


/*
**  Return a copy of an array of strings.
*/
static char **
SITEcopystrings(char **av)
{
    char	**new;
    char	**pp;
    char	**save;

    for (pp = av; *pp; pp++)
	continue;
    for (new = save = xmalloc((pp - av + 1) * sizeof(char *)), pp = av; *pp; pp++)
	*new++ = xstrdup(*pp);
    *new = NULL;
    return save;
}

/*
** Adds a variable from a line.
*/
static bool
SITEaddvariable(char *line)
{
    char *p, *q;
    SITEVARIABLES *v, *w;
    
    if (*line != VARIABLE_CHAR)
    	return false;
    	
    for (p = line + 1; *p != '\0' && isalnum((unsigned char) *p); p++)
        ;
    if (*p != '=')
    	return false;
    if (p - line > 32) {
	syslog(L_FATAL, "%s bad_newsfeed variable name '%s' too long", 
               LogName, line+1);
	return false;
    }

    /* Chop off trailing spaces. */
    q = p + strlen(p) - 1;
    while (q > p && (*q == ' ' || *q == '\t'))
    	*q-- = '\0';

    /* Separate variable name from contents. */
    *p++ = '\0';	
    if (*p == '\0')
    	return false;

    /* Is variable already defined?  Free and reassign. */
    w = NULL;
    v = SITEvariables;
    while (v && strcmp(line + 1, v->Name)) {
        w = v;
    	v = v->Next;
    }
    if (v)
    	free(v->Value);
    else {
    	v = xmalloc(sizeof(SITEVARIABLES));
    	if (!SITEvariables)
    	    SITEvariables = v;
    	if (w)
            w->Next = v;
    	v->Name = xstrdup(line + 1);
    	v->Next = NULL;
    }

    /* Add variable's contents. */
    v->Elements = 1;
    for (q = v->Value = xmalloc(strlen(p) + 1); *p != '\0'; p++) {
        if (*p != ' ' && *p != '\t')
            *q++ = *p;
        if (*p == ',')
            v->Elements++;
    }
    *q = '\0';
    return true;        
}

static void
SITEclearvariables(void)
{
    SITEVARIABLES *v, *w;
    
    v = SITEvariables;
    while (v) {
    	free(v->Name);
    	free(v->Value);
    	w = v;
    	v = v->Next;
    	free(w);
    }
    SITEvariables = NULL;
}

static SITEVARIABLES *
SITEfindvariable(char *name)
{
    SITEVARIABLES *v;

    v = SITEvariables;
    while (v && strcmp(v->Name, name) != 0)
	v = v->Next;
    return v;
}

static char *
SITEexpandvariables(char *site)
{
    char *p, *r, *s;
    char *q = NULL;
    int c = 0;
    char modifier;
    char varname[64];
    SITEVARIABLES *v;

    /* Count characters. */
    *varname = '\0';
    modifier = '\0';
    for (p = site; p <= site + strlen(site); p++) {
        /* In variable name. */
        if (*varname) {
            if (isalnum((unsigned char) *p)) {
                if (q - varname > 32) {
                    /* Add ignored modifier. */
                    if (modifier)
                        c++;
                    /* Add ignored $ and characters. */
                    c += strlen(varname);
                    /* Add this character. */
                    c++;
                    *varname = '\0';
                    modifier = '\0';
                    continue;
                }
                /* Append to variable name. */
                *q++ = *p;
                continue;
            } else {
                v = SITEfindvariable(varname + 1);
                if (v != NULL) {
                    /* Add length of contents. */
                    c += strlen(v->Value);
                    /* If modified add number of mods. */
                    if (modifier)
                        c += v->Elements;
                } else {
                    /* Add ignored modifier. */
                    if (modifier)
                        c++;
                    c += strlen(varname); /* add ignored $ and characters */
                }
                *varname = '\0';
                modifier = '\0';
            }
        }
        /* New variable starts */
        if (*p == VARIABLE_CHAR) {
            q = varname;
            memset(varname, 0, sizeof(varname));
            *q++ = VARIABLE_CHAR;
            continue;
        }
        if (modifier) {
            /* Add last modifier */
            c++;
            modifier = '\0';
        }
        if (*p == SUB_NEGATE || *p == SUB_POISON) {
            modifier = *p;
        } else {
            /* Add this character. */
            c++;
        }
    }

    /* Copy contents. */
    s = r = xmalloc(c + 1);
    *varname = '\0';
    modifier = '\0';
    for (p = site; p <= site + strlen(site); p++) {
        /* In variable name. */
        if (*varname) {
            if (isalnum((unsigned char) *p)) {
                if (q - varname > 32) {
                    if (modifier)
                        *s++ = modifier;
                    for (q = varname; *q; q++)
                        *s++ = *q;
                    *s++ = *p;
                    *varname = '\0';
                    modifier = '\0';
                    continue;
                }
                *q++ = *p;
                continue;
            } else {
                v = SITEfindvariable(varname + 1);
                if (v != NULL) {
                    if (modifier)
                        *s++ = modifier;
                    for (q = v->Value; *q; q++) {
                        *s++ = *q;
                        if (*q == ',' && modifier)
                            *s++ = modifier;
                    }
                } else {
                    if (modifier)
                        *s++ = modifier;
                    for (q = varname; *q; q++)
                        *s++ = *q;
                }
                *varname = '\0';
                modifier = '\0';
            }
        }
        /* New variable starts. */
        if (*p == VARIABLE_CHAR) {
            q = varname;
            memset(varname, 0, sizeof(varname));
            *q++ = VARIABLE_CHAR;
            continue;
        }
        if (modifier) {
            *s++ = modifier;
            modifier = '\0';
        }
        if (*p == SUB_NEGATE || *p == SUB_POISON)
            modifier = *p;
        else
            *s++ = *p;
    }
    *s++ = '\0';

    return r;
}

/*
**  Read the newsfeeds file, return a string array of entries.
*/
char **
SITEreadfile(const bool ReadOnly)
{
    static char		**old_strings;
    static time_t	old_mtime;
    static ino_t	old_ino;
    static off_t	old_size;
    char		*p;
    char		*to;
    char		*site;
    int			i;
    struct stat		Sb;
    char		*data;

    if (SITEfeedspath == NULL)
	SITEfeedspath = concatpath(innconf->pathetc, INN_PATH_NEWSFEEDS);
    if (old_strings != NULL) {
	/* If the file hasn't changed, return a copy of the old data. */
	if (stat(SITEfeedspath, &Sb) >= 0
	 && Sb.st_ino == old_ino
	 && Sb.st_size == old_size
	 && Sb.st_mtime == old_mtime)
	    return ReadOnly ? old_strings : SITEcopystrings(old_strings);

	/* Data's bad, toss it. */
	for (i = 0; old_strings[i] != NULL; i++)
	    free(old_strings[i]);
	free(old_strings);
    }

    /* Read in the file, note its statistics. */
    if ((data = ReadInFile(SITEfeedspath, &Sb)) == NULL) {
	syslog(L_FATAL, "%s cant read %s %m", LogName, SITEfeedspath);
	exit(1);
    }
    old_mtime = Sb.st_mtime;
    old_ino = Sb.st_ino;
    old_size = Sb.st_size;

    /* Get a gross count of the number of sites. */
    for (p = data, i = 0; (p = strchr(p, '\n')) != NULL; p++, i++)
	continue;

    /* Scan the file, parse all multi-line entries. */
    for (old_strings = xmalloc((i + 1) * sizeof(char *)), i = 0, to = p = data; *p; ) {
	for (site = to; *p; ) {
	    if (*p == '\n') {
		p++;
		*to = '\0';
		break;
	    }
	    if (*p == '\\' && p[1] == '\n')
		while (*++p && isspace((unsigned char) *p))
		    continue;
	    else
		*to++ = *p++;
	}
	*to++ = '\0';
	if (*site == '#' || *site == '\0')
            continue ;
        if (*site == VARIABLE_CHAR && SITEaddvariable(site))
	    continue ;        
        if (strspn(site," \t") == strlen (site))
	    continue;
	if (SITEvariables)
	    old_strings[i++] = SITEexpandvariables(site);
	else
	    old_strings[i++] = xstrdup(site);
    }
    old_strings[i] = NULL;
    
    SITEclearvariables();
    free(data);
    return ReadOnly ? old_strings : SITEcopystrings(old_strings);
}


/*
**  Modify "subbed" according to the patterns in "patlist."
*/
static void
SITEsetlist(char **patlist, char *subbed, char *poison, bool *poisonEntry)
{
    char	*pat;
    char	*p;
    char	*u;
    char	subvalue;
    char	poisonvalue;
    NEWSGROUP	*ngp;
    int		i;

    while ((pat = *patlist++) != NULL) {
	subvalue = *pat != SUB_NEGATE && *pat != SUB_POISON;
	poisonvalue = *pat == SUB_POISON;
	if (!subvalue)
	    pat++;
	if (!*pat)
	    continue;
	if (!*poisonEntry && poisonvalue)
	    *poisonEntry = true;

	/* See if pattern is a simple newsgroup name.  If so, set the
	 * right subbed element for that one group (if found); if not,
	 * pattern-match against all the groups. */
	for (p = pat; *p; p++)
	    if (*p == '?' || *p == '*' || *p == '[')
		break;
	if (*p == '\0') {
	    /* Simple string; look it up, set it. */
	    if ((ngp = NGfind(pat)) != NULL) {
		subbed[ngp - Groups] = subvalue;
		poison[ngp - Groups] = poisonvalue;
	    }
	}
	else
	    for (p = subbed, u = poison, ngp = Groups, i = nGroups;
			--i >= 0; ngp++, p++, u++)
		if (uwildmat(ngp->Name, pat)) {
		    *p = subvalue;
		    *u = poisonvalue;
		}
    }
}

/*
**  Split text into slash-separated fields.  Return an allocated
**  NULL-terminated array of the fields within the modified argument that
**  the caller is expected to save or free.  We don't use strchr() since
**  the text is expected to be either relatively short or "slash-dense."
*/
static char **
SlashSplit(char *text)
{
    int		i;
    char	*p;
    char	**av;
    char	**save;

    /* How much space do we need? */
    for (i = 2, p = text; *p; p++)
	if (*p == '/')
	    i++;

    for (av = save = xmalloc(i * sizeof(char *)), *av++ = p = text; *p; )
	if (*p == '/') {
	    *p++ = '\0';
	    *av++ = p;
	}
	else
	    p++;
    *av = NULL;
    return save;
}

/*
**  Parse an individual site entry.  Subbed is used to build the subscription
**  list.  Since this routine is called once for each site, the caller
**  allocates subbed once, and frees it after the last site has been parsed.
**  If subbed is NULL, we don't update the SITE array, since we're just
**  doing syntax checking.
*/
const char *
SITEparseone(char *Entry, SITE *sp, char *subbed, char *poison)
{
    int	                i;
    int	                j;
    NEWSGROUP	        *ngp;
    char	        *p;
    char	        *u;
    char		*f2;
    char		*f3;
    char		*f4;
    char		**save;
    char		**argv;
    bool		JustModerated;
    bool		JustUnmoderated;
    int			isp;
    SITE		*nsp;
    struct buffer	b;
    HASHFEEDLIST        *hf;

    b = sp->Buffer;
    *sp = SITEnull;
    sp->Buffer = b;
    sp->Master = NOSITE;
    sp->Funnel = NOSITE;
    sp->PoisonEntry = false;
    sp->Process = -1;
    sp->Next = sp->Prev = NOSITE;
    sp->Entry = Entry;
    sp->Originator = NULL;
    sp->FileFlags[0] = FEED_NAME;
    sp->FileFlags[1] = '\0';
    sp->Nice = innconf->nicekids;
    sp->ControlOnly = false;
    sp->DontWantNonExist = false;
    sp->NeedOverviewCreation = false;
    sp->FeedwithoutOriginator = false;
    sp->DropFiltered = false;
    sp->FeedTrash = false;
    sp->HashFeedList = NULL;

    /* Nip off the first field, the site name. */
    if ((f2 = strchr(Entry, NF_FIELD_SEP)) == NULL)
	return "missing field 2";
    *f2++ = '\0';
    sp->Name = Entry;
    if ((p = strchr(sp->Name, NF_SUBFIELD_SEP)) != NULL) {
	/* Exclusions within the site field. */
	*p++ = '\0';
	if (*p)
	sp->Exclusions = CommaSplit(p);
    }
    sp->NameLength = strlen(sp->Name);

    /* Parse the second field, the subscriptions. */
    if ((f3 = strchr(f2, NF_FIELD_SEP)) == NULL)
	return "missing field 3";
    *f3++ = '\0';
    if ((p = strchr(f2, NF_SUBFIELD_SEP)) != NULL) {
	/* Distributions within the subscription field. */
	*p++ = '\0';
	if (*p)
	sp->Distributions = CommaSplit(p);
    }
    if (f2)
    sp->Patterns = CommaSplit(f2);

    if (subbed) {
	/* Read the subscription patterns and set the bits. */
	memset(subbed, SUB_DEFAULT, nGroups);
	memset(poison, SUB_DEFAULT, nGroups);
	if (ME.Patterns)
	    SITEsetlist(ME.Patterns, subbed, poison, &ME.PoisonEntry);
	SITEsetlist(sp->Patterns, subbed, poison, &sp->PoisonEntry);
    }

    /* Get the third field, the flags. */
    if ((f4 = strchr(f3, NF_FIELD_SEP)) == NULL)
	return "missing field 4";
    *f4++ = '\0';
    JustModerated = false;
    JustUnmoderated = false;
    sp->Type = FTfile;
    for (save = argv = CommaSplit(f3); (p = *argv++) != NULL; )
	switch (*p) {
	default:
	    return "unknown field 3 flag";
	case '\0':
	    break;
	case '<':
	    if (*++p && isdigit((unsigned char) *p))
		sp->MaxSize = atol(p);
	    break;
	case '>':
	    if (*++p && isdigit((unsigned char) *p))
		sp->MinSize = atol(p);
	    break;
	case 'A':
	    while (*++p)
		switch (*p) {
		default:
		    return "unknown A param in field 3";
		case 'c': sp->IgnoreControl = true;
			  sp->ControlOnly = false;
			  break;
		case 'C': sp->ControlOnly = true;
			  sp->IgnoreControl = false;
			  break;
		case 'd': sp->DistRequired = true;	break;
		case 'e': sp->DontWantNonExist = true;	break;
		case 'f': sp->DropFiltered = true;	break;
                case 'j': sp->FeedTrash = true;         break;
		case 'o': sp->NeedOverviewCreation = true;	break;
		case 'O': sp->FeedwithoutOriginator = true;	break;
		case 'p': sp->IgnorePath = true;	break;
		}
	    break;
	case 'B':
	    if (*++p && isdigit((unsigned char) *p)) {
		sp->StartWriting = atoi(p);
		if ((p = strchr(p, NF_SUBFIELD_SEP)) != NULL
		 && *++p
		 && isdigit((unsigned char) *p))
		    sp->StopWriting = atoi(p);
	    }
	    break;
	case 'C':
	    if (*++p && isdigit((unsigned char) *p))
		sp->Crosscount = atoi(p);
	    else
		sp->Crosscount = 2;
	    break;
	case 'F':
	    if (*++p == '\0')
		return "missing file name for F param in field 3";
	    else
                sp->SpoolName = concatpath(innconf->pathoutgoing, p);
	    break;
	case 'G':
	    if (*++p && isdigit((unsigned char) *p))
		sp->Groupcount = atoi(p);
	    else
		sp->Groupcount = 1;
	    break;
	case 'H':
	    if (*++p && isdigit((unsigned char) *p))
		sp->Hops = atoi(p);
	    else
		sp->Hops = 1;
	    break;
	case 'I':
	    if (*++p && isdigit((unsigned char) *p))
		sp->Flushpoint = strtoul(p, NULL, 10);
	    break;
	case 'N':
	    while (*++p)
		switch (*p) {
		default:
		    return "unknown N param in field 3";
		case 'm': JustModerated = true;		break;
		case 'u': JustUnmoderated = true;	break;
		}
	    break;
	case 'O':
	    if (*++p == '\0')
		return "missing originator name for O param in field 3";
	    sp->Originator = SlashSplit(p);
	    break;
        case 'P':
            if (*++p && isdigit((unsigned char) *p))
                sp->Nice = atoi(p);
            break;
        case 'Q':
            hf = xmalloc(sizeof(HASHFEEDLIST));
            p++;
            /* Check whether it is a quickhash or a MD5 hashfeed. */
            if (*p == '@') {
                p++;
                hf->type = HASHFEED_QH;
            } else {
                hf->type = HASHFEED_MD5;
            }
            /* Check the presence of a starting byte-offset for hashfeed. */
            if ((u = strchr(p, '_')) != NULL) {
                if (sscanf(u + 1, "%u", &hf->offset) != 1 || hf->offset > 12) {
                    free(hf);
                    return "invalid hash offset for Q param in field 3";
                }
            } else {
                hf->offset = 0;
            }
            if (sscanf(p, "%u/%u", &hf->begin, &hf->mod) == 2) {
                hf->end = hf->begin;
            } else if (sscanf(p, "%u-%u/%u", &hf->begin, &hf->end,
                              &hf->mod) != 3) {
                free(hf);
                return "hash not in x/z or x-y/z format for Q param in field 3";
            }
            if (hf->begin > hf->end || hf->end > hf->mod) {
                free(hf);
                return "incorrect hash values for Q param in field 3";
            }
            hf->next = sp->HashFeedList;
            sp->HashFeedList = hf;
            break;
	case 'S':
	    if (*++p && isdigit((unsigned char) *p))
		sp->StartSpooling = atol(p);
	    break;
	case 'T':
	    switch (*++p) {
	    default:
		return "unknown T param in field 3";
	    case 'c': sp->Type = FTchannel;	break;
	    case 'l': sp->Type = FTlogonly;	break;
	    case 'f': sp->Type = FTfile;	break;
	    case 'm': sp->Type = FTfunnel;	break;
	    case 'p': sp->Type = FTprogram;	break;
	    case 'x': sp->Type = FTexploder;	break;
	    }
	    break;
	case 'U':
	    if (*++p && isdigit((unsigned char) *p))
		sp->Followcount = atoi(p);
	    else
		sp->Followcount = 1;
	    break;
	case 'W':
	    for (i = 0; *++p && i < FEED_MAXFLAGS; ) {
		switch (*p) {
		default:
		    return "unknown W param in field 3";
		case FEED_FNLNAMES:		/* Funnel feed names	*/
		    sp->FNLwantsnames = true;
		    break;
		case FEED_HEADERS:		/* Article headers	*/
		    NeedHeaders = true;
		    break;
		case FEED_OVERVIEW:
		    NeedOverview = true;	/* Overview data	*/
		    break;
		case FEED_PATH:			/* Path			*/
		    NeedPath = true;
		    break;
		case FEED_BYTESIZE:		/* Size in bytes	*/
		case FEED_FULLNAME:		/* Full filename	*/
		case FEED_HASH:			/* Hash			*/
		case FEED_HDR_DISTRIB:		/* Distribution header	*/
		case FEED_STOREDGROUP:		/* stored newsgroup	*/
		    NeedStoredGroup = true;
		    break;
		case FEED_HDR_NEWSGROUP:	/* Newsgroup header	*/
		case FEED_MESSAGEID:		/* Message-ID		*/
		case FEED_NAME:			/* Filename		*/
		case FEED_NEWSGROUP:		/* Newsgroup		*/
		case FEED_REPLIC:		/* Replication data	*/
		    NeedReplicdata = true;
		    break;
		case FEED_SITE:			/* Site that gave it	*/
		case FEED_TIMERECEIVED:		/* When received	*/
		case FEED_TIMEPOSTED:		/* When posted		*/
		case FEED_TIMEEXPIRED:		/* When will be expired	*/
		    break;
		}
		sp->FileFlags[i++] = *p;
	    }
	    if (*p)
		return "too many W param values";
	    sp->FileFlags[i] = '\0';
	    break;
	}
    free(save);
    if (sp->Flushpoint && sp->Type != FTfile)
	return "I param with non-file feed";
    if (sp->Flushpoint == 0 && sp->Type == FTfile)
	sp->Flushpoint = SITE_BUFFER_SIZE;

    if (subbed) {
	/* Modify the subscription list based on the flags. */
	if (JustModerated)
	    for (p = subbed, ngp = Groups, i = nGroups; --i >= 0; ngp++, p++)
		if (ngp->Rest[0] != NF_FLAG_MODERATED)
		    *p = false;
	if (JustUnmoderated)
	    for (p = subbed, ngp = Groups, i = nGroups; --i >= 0; ngp++, p++)
		if (ngp->Rest[0] == NF_FLAG_MODERATED)
		    *p = false;
    }

    /* Get the fourth field, the param. */
    if (*f4 == '\0' && sp != &ME) {
	if (sp->Type != FTfile && sp->Type != FTlogonly)
	    return "empty field 4";
        sp->Param = concatpath(innconf->pathoutgoing, sp->Name);
    }
    else if (sp->Type == FTfile && *f4 != '/')
        sp->Param = concatpath(innconf->pathoutgoing, f4);
    else
	sp->Param = xstrdup(f4);

    if (sp->SpoolName == NULL)
        sp->SpoolName = concatpath(innconf->pathoutgoing, sp->Name);

    /* Make sure there is only one %s, and only one *. */
    if (sp->Type == FTprogram) {
	f3 = NULL;
	for (f2 = sp->Param; *f2; f2 = p + 1) {
	    p = strchr(f2, '%');
	    if (p == NULL)
		break;
	    if (p[1] == '%') {
		p++;
		continue;
	    }
	    if (f3 != NULL)
		return "bad (extra) sprintf format for field 4";
	    f3 = p;
	    while (*++p && *p != '*' && !isalpha((unsigned char) *p))
		continue;
	    if (*p != 's')
		return "bad sprintf format for field 4";
	}
	if (sp->FNLwantsnames
	 && ((p = strchr(sp->Param, '*')) == NULL
	  || strchr(++p, '*') != NULL))
	    return "multiple or no *'s in field 4";
    }

    /* Now tell the groups this site gets that they should feed this site. */
    if (sp != &ME && subbed) {
	isp = sp - Sites;
	for (p = subbed, u = poison, ngp = Groups, i = nGroups;
		--i >= 0; ngp++) {
	    if (*p++) {
		for (j = 0; j < ngp->nSites; j++)
		    if (ngp->Sites[j] == NOSITE) {
			ngp->Sites[j] = isp;
			break;
		    }
		if (j == ngp->nSites)
		    ngp->Sites[ngp->nSites++] = isp;
	    }
	    if (*u++) {
		for (j = 0; j < ngp->nPoison; j++)
		    if (ngp->Poison[j] == NOSITE) {
			ngp->Poison[j] = isp;
			break;
		    }
		if (j == ngp->nPoison)
		    ngp->Poison[ngp->nPoison++] = isp;
	    }
        }
    }

    /* If this is a duplicate name, find the master. */
    nsp = SITEfind(sp->Name);
    if (nsp == sp)
	nsp = SITEfindnext(sp->Name, nsp);
    if (nsp != NULL) {
	if (nsp->Master != NOSITE)
	    nsp = &Sites[nsp->Master];
	if (nsp != sp) {
	    sp->Master = nsp - Sites;
	    nsp->IsMaster = true;
	}
    }

    return NULL;
}


/*
**  Patch up the funnel references.
*/
bool
SITEfunnelpatch(void)
{
    int		i;
    size_t	length;
    SITE	*sp;
    SITE	*funnel;
    bool	result;

    /* Get worst-case length of all sitenames. */
    for (length = 0, i = nSites, sp = Sites; --i >= 0; sp++)
	if (sp->Name != NULL)
	    length += 1 + strlen(sp->Name);

    /* Loop over all funnel feeds. */
    for (result = true, i = nSites, sp = Sites; --i >= 0; sp++) {
	if (sp->Name == NULL || sp->Type != FTfunnel)
	    continue;

	/* Find the entry they feed in to, give that entry a buffer. */
	if (sp->Param == NULL) {
	    syslog(L_FATAL, "%s funnel NULL", sp->Name);
	    SITEfree(sp);
	    result = false;
	    continue;
	}
	if ((funnel = SITEfind(sp->Param)) == NULL) {
	    syslog(L_FATAL, "%s funnel_bad", sp->Name);
	    SITEfree(sp);
	    result = false;
	    continue;
	}
	if (funnel->Type == FTfunnel) {
	    syslog(L_FATAL, "%s funnels to funnel %s", sp->Name, funnel->Name);
	    SITEfree(sp);
	    result = false;
	    continue;
	}
	buffer_resize(&funnel->FNLnames, length);
	sp->Funnel = funnel - Sites;
    }

    return result;
}


/*
**  Read the entries in the newsfeeds file, and parse them one at a time.
*/
void
SITEparsefile(bool StartSite)
{
  int		i;
  char *	p;
  char **	strings;
  SITE *	sp;
  char *	subbed;
  char *	poison;
  const char *	error;
  int		errors;
  int		setuperrors;

  /* Free old sites info. */
  if (Sites) {
    for (i = nSites, sp = Sites; --i >= 0; sp++) {
      SITEflush(sp, false);
      SITEfree(sp);
    }
    free(Sites);
    SITEfree(&ME);
  }

  /* Count the number of sites. */
  for (strings = SITEreadfile(false), nSites = 0; strings[nSites]; nSites++)
    continue;
  Sites = xcalloc(nSites, sizeof(SITE));

  /* Set up scratch subscription list. */
  subbed = xmalloc(nGroups);
  poison = xmalloc(nGroups);
  /* reset global variables */
  NeedHeaders = NeedOverview = NeedPath = NeedStoredGroup = NeedReplicdata
    = false;

  ME.Prev = 0; /* Used as a flag to ensure exactly one ME entry */
  for (sp = Sites, errors = 0, setuperrors = 0, i = 0; i < nSites; i++) {
    p = strings[i];
    if (p[0] == 'M' && p[1] == 'E' &&
      ((p[2] == NF_FIELD_SEP) || (p[2] == NF_SUBFIELD_SEP))) {
      if (ME.Prev == NOSITE) {
	syslog(L_FATAL, "bad_newsfeeds. Must have exactly one ME entry");
	errors++;
      } else if ((error = SITEparseone(p, &ME, subbed, poison)) != NULL) {
	syslog(L_FATAL, "%s bad_newsfeeds %s", MaxLength(p, p), error);
	errors++;
      }
      continue;
    }
    if ((error = SITEparseone(p, sp, subbed, poison)) != NULL) {
      syslog(L_FATAL, "%s bad_newsfeeds %s", MaxLength(p, p), error);
      errors++;
      continue;
    }
    if (StartSite && !SITEsetup(sp)) {
      syslog(L_FATAL, "%s cant setup %m", sp->Name);
      setuperrors++;
      continue;
    }
    sp->Working = true;
    sp++;
  }
  if (ME.Prev != NOSITE) {
    syslog(L_FATAL, "bad_newsfeeds. Must have exactly one ME entry");
    errors++;
  }

  if (errors || setuperrors) {
    if (errors)
      syslog(L_FATAL, "%s syntax_error %s", LogName, SITEfeedspath);
    if (setuperrors)
      syslog(L_FATAL, "%s setup_error %s", LogName, SITEfeedspath);
    JustCleanup();
    exit(1);
  }

  /* Free our scratch array, set up the funnel links. */
  nSites = sp - Sites;
  free(subbed);
  free(poison);
  free(strings);
  if (!SITEfunnelpatch()) {
    JustCleanup();
    exit(1);
  }
}
