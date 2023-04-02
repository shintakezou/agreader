/********************************************************
** AGObj.c : Specific functions for AmigaGuide Objects **
**           like Paragraph, Words, tabs, indent...    **
**-----------------------------------------------------**
** Started on 13/2/2001, free software under GNU PL    **
********************************************************/

#include "AGObj.h"
#include "AGNodes.h"
#include "AGReader.h"
#include "AG_lib.h"
#include <ctype.h>
#include <string.h>

char* LinkTokens[] = {
    "link", "system", NULL
};

char MemErr[] = "Out of memory!";

/*** Init default styles/indent for one node ***/
void InitContext(AGContext* agc, char wrap)
{
    agc->fgpen = DEF_FGPEN;
    agc->bgpen = DEF_BGPEN;
    agc->style = FS_NORMAL;
    agc->align = JM_NORMAL;
    agc->indent = 0;
    agc->wrap = wrap;
    agc->ts = NULL;
}

/*** Create a link between another node in the document ***/
AGWord CreateAGLink(char* format)
{
    AGWord new;

    /* Scheme: @{" node desc " [link|system] "dev:path/file/node" 999} */
    if ((new = (AGWord)malloc(sizeof(*new) + sizeof(*new->link))) != NULL) {
        char *p, LinkColor[] = { DEF_UNKNOWNLINK, DEF_FGLINK, DEF_SYSTEM };
        memset(new, 0, sizeof(new->lst));
        new->link = (AGLink)((char*)new + sizeof(*new));
        /* Search for anchor test description */
        new->data = format;
        for (p = format; *p != '\"'; p++)
            ;
        *p = '\0';
        /* Search if it's a node/file or a system command */
        do
            p++;
        while (isspace(*p));
        if (!strncasecmp(p, LinkTokens[0], strlen(LinkTokens[0])))
            new->link->type = LINK_TO_DOC;
        else if (!strncasecmp(p, LinkTokens[1], strlen(LinkTokens[1])))
            new->link->type = SYSTEM_COMMAND;
        else
            new->link->node = p;

        /* Makes them appear like web-page anchor */
        new->style = DEF_LINKSTYLE;
        new->fgpen = LinkColor[(int)new->link->type];
        /* Search content of command/node */
        if (new->link->type != UNKNOWN_TYPE)
            FindAGLinkInfo(new->link, p + strlen(LinkTokens[new->link->type - 1]));
    } else
        quit(MemErr, QUIT_ERROR);
    return new;
}

/*** Insert a word in the structure ***/
void InsertAGWord(AGPara par, AGWord new, AGWord ins)
{
    PREV(new) = ins;
    NEXT(new) = NULL;
    if (ins != NULL)
        NEXT(ins) = new;
    else if (par)
        par->line = new;
}

/*** Simple constructor with default colors ***/
AGWord NewWord(AGPara par, AGWord old, char* data, AGContext* agc)
{
    AGWord new;
    /* If first word begins with spaces, skip them */
    if (agc && agc->wrap == WRAP_SMART) {
        /* Paragraph may start with special objects */
        for (new = par->line; new&& new->data == NULL; new = NEXT(new))
            ;
        /* If it's really the first, skip spaces */
        if (new == NULL) {
            while (isspace(*data))
                data++;
            if (*data == '\0')
                return old;
        }
    }
    /* Fixed formatting: create just one word */
    if (NULL != (new = (AGWord)malloc(sizeof(*new)))) {
        InsertAGWord(par, new, old);
        new->data = data;
        new->link = NULL;
        if (agc)
            memcpy(&new->fgpen, &agc->fgpen, 3);
    } else
        quit(MemErr, QUIT_ERROR);
    return new;
}

/** private def **/
typedef struct {
    struct _AGWord w;
    char svg, *dat;
}* AGSplit;

AGWord NewSplitWord(AGPara par, AGWord cut, char* split)
{
    AGSplit new;
    if (NULL != (new = (AGSplit)malloc(sizeof(*new)))) {
        InsertAGWord(par, (AGWord) new, NULL);
        new->w.data = split;
        new->w.link = NULL;
        new->svg = *(new->dat = --split);
        *split = 0;
        memcpy(&new->w.fgpen, &cut->fgpen, 3);
        new->w.style |= FSF_SPLITTED;
    } else
        quit(MemErr, QUIT_ERROR);
    return (AGWord) new;
}

AGWord FreeSplit(AGWord word)
{
    AGWord next = NEXT(word);
    if (((AGSplit)word)->dat)
        ((AGSplit)word)->dat[0] = ((AGSplit)word)->svg;
    free(word);
    return next;
}

/*** Alloc new paragraph ***/
AGPara NewPara(AGPara ins, AGContext* agc)
{
    AGPara new;
    if ((new = (AGPara)malloc(sizeof(*new))) != NULL) {
        memset(new, 0, sizeof(*new));
        if (agc)
            new->align = agc->align,
            new->ts = agc->ts;
        InsertAGWord(NULL, (AGWord) new, (AGWord)ins);
    } else
        quit(MemErr, QUIT_ERROR);
    return new;
}

/*** Specific handler for `indent' object ***/
void InitIndent(AGObj obj, ObjPara* op)
{
    op->nidt = LINDENT(obj);
}

/*** Reset para ***/
void RstPara(AGObj obj, ObjPara* op)
{
    op->nidt = LINDENT(obj);
    op->limit = WRAP(obj);
}

/*** Turn of word-wrapping while formatting paragraph ***/
void WrapOFF(AGObj obj, ObjPara* op)
{
    op->limit = 0;
}

/*** Init/reset tab-stops series object ***/
void InitTabs(AGObj obj, ObjPara* op)
{
    extern short* tabs;
    tabs = TABSTOP(obj);
}
void ClearTabs(AGObj obj, ObjPara* op)
{
    extern short* tabs;
    tabs = NULL;
}

/*** Create a new object with the init function provided ***/
AGWord NewObject(AGPara par, AGWord old, AGInit pfnInit, size_t mem)
{
    AGObj new;
    /* Create a new object */
    if (NULL != (new = (AGObj)malloc(sizeof(*new) + mem))) {
        InsertAGWord(par, (AGWord) new, old);
        new->data = NULL;
        new->func = pfnInit;

    } else
        quit(MemErr, QUIT_ERROR);
    return (AGWord) new;
}

/*** Object containing indenting information ***/
AGWord NewIndent(AGPara par, AGWord old, short indent)
{
    AGObj new = (AGObj)NewObject(par, old, InitIndent, sizeof(short));
    LINDENT(new) = indent;
    return (AGWord) new;
}

/*** Reset paragraph defaults ***/
AGWord NewResetPara(AGPara par, AGWord old, char wrap)
{
    AGObj new = (AGObj)NewObject(par, old, RstPara, sizeof(short) + 1);
    LINDENT(new) = 0;
    WRAP(new) = wrap;
    return (AGWord) new;
}

/*** Object containing series of tab stops in spaces ***/
AGWord NewTabs(AGPara par, AGWord old, char* fmt)
{
    short nb, *ts;
    char *p, a, b;
    AGObj new;
    /* First, count number of tab stops */
    for (p = fmt, nb = 1, a = 0; *p && *p != '}'; p++, a = b)
        if (a != (b = isdigit(*p) != 0))
            nb++;

    /* Alloc a new object */
    new = (AGObj)NewObject(par, old, InitTabs, sizeof(short) * nb);
    /* Init the tabstop numbers */
    for (ts = TABSTOP(new), p = fmt, a = 0; nb > 1; p++, a = b)
        if (a != (b = isdigit(*p) != 0))
            *ts++ = atoi(p), nb--;
    *ts = 0;

    return (AGWord) new;
}
