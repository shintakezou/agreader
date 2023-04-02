/*********************************************************
** Text.c : Create AmigaGuide node from ANSI text file. **
**          by T.Pierron, started on 3/10/2000.         **
*********************************************************/

#include "Text.h"
#include "AGNodes.h"
#include "AGObj.h"
#include "AGReader.h"
#include "Input.h"
#include <string.h>

/** Redefines only functions that differs from AG_lib.c **/

/*** Create file and structure information for an ANSI text file ***/
AGFile CreateTextNodes(char* filename)
{
    AGFile new;
    if ((new = CreateFile(filename))) {
        AGNode node;
        /* Only one node is defined for text file */
        if ((new->Content = (AGList)(node = (AGNode)malloc(sizeof(*node))))) {
            memset(node, 0, sizeof(*node));
            node->name = "MAIN";
            node->title = new->FName;
            node->start = new->Buffer;
            node->bgpen = DEF_BGPEN;
            node->tabsize = DEF_TABSIZE;
            if (filename == NULL)
                return new;
            { /* Quickly counts the number of lines */
                register char* p = new->Buffer;
                for (; *p; p++)
                    if (*p == '\n')
                        new->NbNodes++;
            }
            AGFileInfo(new, "line", "lines");
            return new;

        } else
            ThrowError(ERROR_NO_FREE_STORE, NULL);
    }
    return NULL;
}

/*** Create information for an ANSI text stream ***/
AGFile CreateTextFromStream(char* stream, char* title)
{
    AGFile new;

    if ((new = CreateTextNodes(NULL))) {
        AGNode node = (AGNode) new->Content;
        node->start = new->Buffer = stream;
        node->title = title;
        return new;
    }
    return NULL;
}

/*** Search for standard ANSI styles modifiers ***/
void FindANSIStyle(short nb, AGContext* agc)
{
    /* Foreground and background modifiers */
    if (30 <= nb && nb <= 37)
        agc->fgpen = nb - (30 - '0');
    else if (40 <= nb && nb <= 47)
        agc->bgpen = nb - (40 - '0');
    else if (90 <= nb && nb <= 97)
        agc->fgpen = nb - (90 - '0'), agc->style |= FSF_FGSHINE;
    else if (100 <= nb && nb <= 107)
        agc->bgpen = nb - (100 - '0'), agc->style |= FSF_BGSHINE;
    else

        /* Styles modifiers */
        switch (nb) {
            case 0:
                InitContext(agc, 0);
                break;
            case 2:
                agc->style |= FSF_ITALIC;
                break;
            case 22:
            case 1:
                agc->style |= FSF_BOLD;
                break;
            case 24:
            case 4:
                agc->style |= FSF_UNDERLINED;
                break;
            case 27:
            case 7:
                agc->style |= FSF_INVERSID;
                break;
            case 39:
                agc->fgpen = DEF_FGPEN;
                break;
            case 49:
                agc->bgpen = DEF_BGPEN;
                break;
        }
}

char* CharName[] = {
    "^@", "^A", "^B", "^C", "^D", "^E", "^F", "^G", "BS", 0, 0, "^K", "^L", "CR", "^N", "^O", "^P",
    "^Q", "^R", "^S", "^T", "^U", "^V", "^W", "^X", "^Y", "^Z", "ESC", "^\\", "^]", "^^", "^_"
};
char* CharSpec[] = {
    "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F"
};

/*** Special characters may trash the display ***/
AGWord DisableSpecialChar(AGPara par, AGWord old, unsigned char c)
{
    AGContext AGC;
    InitContext(&AGC, 0);
    AGC.style = FSF_INVERSID;
    return NewWord(par, old, (c < 32 ? CharName[c] : CharSpec[c - 128]), &AGC);
}

/*** Create style-separated words ***/
char CreateTextWords(AGNode node)
{
    static AGContext AGC;
    static AGPara par;
    static AGWord new;
    char *buf, *p;

    InitContext(&AGC, JM_NORMAL);
    /* Alloc a first paragraph */
    if ((par = NewPara(NULL, &AGC)) == NULL)
        return 0;
    p = buf = node->start;
    new = NULL;

    /* Scan through the buffer */
    for (; *buf; buf++) {
        if (*buf == '\n') /* A newline */
        {
            /* End of line, finishes the preceding word */
            *buf = '\0';
            if (p < buf)
                new = NewWord(par, new, p, &AGC);

            /* Next line and word start here */
            par = NewPara(par, &AGC);
            new = NULL;
            node->maxlines++;
            p = buf + 1;
        }
        /* Starts of an ANSI sequence? */
        else if (IS_SPECIAL((unsigned char)*buf)) {
            short nb = *buf;
            *buf = '\0';
            if (p < buf)
                new = NewWord(par, new, p, &AGC);

            if (nb == 27) {
                /* Start of an ANSI sequence, search for its end */
                for (p = buf++; !('A' <= *buf && *buf <= 'Z') && !('a' <= *buf && *buf <= 'z')
                     && *buf && *buf != '\n';
                     buf++)
                    ;

                /* Only styles modifier are supported */
                if (*buf == 'm') {
                    for (p++, nb = 0; p <= buf; p++) {
                        if (*p == ';' || *p == 'm')
                            FindANSIStyle(nb, &AGC), nb = 0;
                        if ('0' <= *p && *p <= '9')
                            nb *= 10, nb += *p - '0';
                    }
                } else
                    new = DisableSpecialChar(par, new, 27), buf = p;
            } else
                new = DisableSpecialChar(par, new, nb);

            p = buf + 1;
        }
    }
    /* First line shown */
    if (node->AGContent == NULL) {
        AGPara fpar = par;
        for (; PREV(fpar); fpar = PREV(fpar))
            ;
        node->AGContent = node->Shown = fpar;
        node->column = node->line = 0;
    }
    return par ? 1 : 0;
}

/*** Special function to convert Amiga color to Unix one ***/
char NewLookToUnix(char color, char* style)
{
    static char TabCol[] = "70743421";
    static char TabSty[] = "00011111";
    if ('0' <= color && color <= '7') {
        if (style && TabSty[color - '0'] != '0')
            *style |= FSF_BOLD;
        return TabCol[color - '0'];
    } else
        return color;
}

/*** Apply conversion to all words from a node ***/
void AdjustColors(AGNode node)
{
    AGPara para;
    AGWord word;
    for (para = node->AGContent; para; para = NEXT(para))
        for (word = para->line; word; word = NEXT(word))
            word->fgpen = NewLookToUnix(word->fgpen, &word->style),
            word->bgpen = NewLookToUnix(word->bgpen, NULL);
}

/*** Special functions for reading through piped files ***/
extern struct scrpos terminfo;

#if 0
/*** New bytes are available, read them ***/
int UpdateBuffer( void )
{
	AGFile file;
	char  *buf;
	int    len;

	file = terminfo.file;

	/* Alloc first a buffer or check if there is an already non-empty allocated */
	if(file->Last != NULL && file->Length < BUF_PIPE_SIZE)
		buf = ((AGBuf)file->Last)->Stream + file->Length,
		len = BUF_PIPE_SIZE - file->Length;
	else
	{
		/* Alloc a new buffer */
		AGBuf new;
		if(new = (AGBuf) malloc(sizeof(*new)))
		{
			if( (AGBuf)file->Last ) ((AGBuf)file->Last)->Next = new;
			else ((AGBuf)file->Buffer2) = new;
			((AGBuf)file->Last) = new;
			new->Next = NULL;
			file->Length = 0;
			len = BUF_PIPE_SIZE;
			buf = new->Stream;
		} else ThrowError(ERROR_NO_FREE_STORE, NULL), len=0;
	}
	/* Still more bytes to read? */
	if( (len = read(0,buf,len)) <= 0 ) return 0;
	file->Length += len;
	return 1;
}

/*** It's time to refresh data read ***/
void RefreshBuffer( void )
{
	AGFile file = terminfo.file;
	static long lastpos;

	/* Search where and how many bytes has been read */
	len = (file->Last == file->Buffer2 ? file->Length : BUF_PIPE_SIZE);
	if(txt == NULL) txt = (agb = file->Buffer2)->Stream, lastpos = 0;
	else
	{
		if(txt-agb->Stream == BUF_PIPE_SIZE) txt = (agb = agb->Next)->Stream, lastpos = 0;
		len -= lastpos;
	}
	/* Format data stream into words */
	CreateTextWords((AGNode) file->Content);
	ReRenderAGNode();
}
#endif
