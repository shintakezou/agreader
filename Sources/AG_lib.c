/******************************************************
** AG_Lib.c : Some handy functions for manipulatings **
**            AmigaGuide file format, by T.Pierron   **
**---------------------------------------------------**
** Started on 28/8/2000, free software under GNU PL  **
******************************************************/

#include "AGNodes.h"
#include "AGObj.h"
#include "AGReader.h"
#include "IO_tty.h"
#include "Input.h"
#include "Version.h"
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>

/** Global buffer used to render one line of text **/
char buffer[512], ts, bgpen;
short* tabs;

/** AmigaGuide tokens supported **/
char AGHeader[] = "@DATABASE ";
char* ColorTokens[] = {
    "detail", "block", "text", "shine", "shadow", "filltext",
    "fill", "back", "highlighttext", NULL
};
char* InfoTokens[] = {
    AGHeader, "@Author ", "@(c) ", "@Master ", "$VER:", NULL
};
char* AttrTokens[] = {
    "b", "i", "u", "ub", "ui", "uu", "jleft", "jcenter", "jright", "lindent",
    "pari", "plain", "pard", "code", "line", "settabs", "cleartabs",
    "amigaguide", "par", "fg ", "bg ", "\"", NULL
};
char* CommandTokens[] = {
    "endnode", "node ", "title ", "tab ", "help ", "index ", "toc ",
    "prev ", "next ", "wordwrap", "smartwrap", "remark", "rem", NULL
};

#define AG_COMMAND 1 /* Marker for known AG commands */

#ifdef DEBUG_MEM
#undef free
#undef malloc
long nb_alloc = 0, nb_free = 0, mem_alloc = 0;
void* AllocMem(size_t size)
{
    mem_alloc += size;
    nb_alloc++;
    return malloc(size);
}
void FreeMem(void* mem)
{
    nb_free++;
    free(mem);
}
#define free(X) FreeMem(X)
#define malloc(X) AllocMem(X)
#endif

/*** Get a string from the buffer ***/
char* GetAGString(char** dst, char* src, char eos)
{
    while (*src != '\n' && isspace(*src))
        src++;
    if (*src == '\n') {
        *dst = 0;
        *src = 0;
        return src + 1;
    }
    if (*src == '\"')
        eos = '\"', src++;
    for (*dst = src; *src != eos && *src != '\n'; src++)
        ;
    while (*src != '\n')
        *src++ = '\0';
    *src = 0;
    return src + 1;
}

/*** Search for an integer in a AG command ***/
int ParseAGInt(char* src, char eos, int def)
{
    char* buf;
    for (buf = src; *buf && *buf != '-' && !isdigit(*buf) && *buf != eos; buf++)
        ;
    return isdigit(*buf) || *buf == '-' ? atoi(buf) : def;
}

/*** Find internal and descriptive node name ***/
char* FindNodeNames(AGNode node, char* src)
{
    char eos = ' ';
    /* Skip starting white spaces */
    while (isspace(*src))
        src++;
    /* String enclosed in "..." ? */
    if (*src == '\"')
        eos = '\"', src++;
    node->name = src;
    for (; *src != eos && *src != '\n'; src++)
        ;
    /* Search for a possibly longer description string */
    if (*src == '\n')
        *src++ = 0, node->title = NULL;
    else
        *src++ = 0, src = GetAGString(&node->title, src, ' ');
    /* The buffer start at next line */
    return node->start = src;
}

/*** Fill AmigaGuide link information ***/
char* FindAGLinkInfo(AGLink link, char* format)
{
    char *p = format, eos;
    while (isspace(*p))
        p++;
    if (*p == '\"')
        eos = '\"', p++;
    else
        eos = ' ';
    link->node = p;
    for (p[-1] = '\0'; *p != eos && *p != '}' && *p != '\n'; p++)
        ;

    switch (link->type) {
        case UNKNOWN_TYPE:
            /* Forget to set the type? */
            link->type = LINK_TO_DOC;
        case LINK_TO_DOC: { /* If it's a link to a node, check if it remains in the same file */
            register char* q = p - 1;
            if (*p == eos)
                *p++ = '\0';
            while (*q && *q != '/')
                q--;
            if (*q == '/') {
                /* This is actually a link to a different file */
                link->file = link->node;
                link->node = q + 1;
                *q = '\0';
            } else
                link->file = NULL;
        } break;
        case SYSTEM_COMMAND:
            if (eos == '\"')
                *p++ = '\0';
            /* Let's command interpreter analyse the string */
            return p;
    }
    /* At last, a number may indicate where to start displaying node */
    if (*p) {
        /* Find the first digit */
        while (!isdigit(*p) && *p != '}' && *p != '\n')
            p++;
        link->line = (isdigit(*p) ? atoi(p) : 0);
    } else
        link->line = 0;
    return p;
}

/*** Check, open and read a static file ***/
AGFile CreateFile(char* filename)
{
    struct stat buf;
    int fh;
    AGFile new;

    buf.st_size = 0;
    /* Try to get information about file */
    if (!filename || stat(filename, &buf) != -1) {
        /* Alloc a large buffer, to read the whole file */
        if ((new = (AGFile)malloc(sizeof(*new) + 1 + buf.st_size)) != NULL) {
            memset(new, 0, sizeof(*new));
            new->Length = buf.st_size;
            new->FName = filename;
            if (filename == NULL)
                return new;
            new->Buffer = (char*)new + sizeof(*new);
            new->Buffer[buf.st_size] = '\0';
            /* Open and read it */
            if ((fh = open(filename, O_RDONLY)) != -1) {
                /* TODO improve... */
                if (read(fh, new->Buffer, buf.st_size) == buf.st_size) {
                    close(fh);
                    return new;

                } else
                    ThrowError("Error while reading `%s'.", filename);
                close(fh);
            } else
                ThrowError("Error atempting to open '%s'.", filename);
            free(new);
        } else
            ThrowError(ERROR_NO_FREE_STORE, NULL);
    } else
        ThrowError("Can't get stat for `%s'.", filename);
    /* If we reach this point, something failed */
    return NULL;
}

/*** Create AGFile and nodes information ***/
AGFile CreateAGNodes(char* filename)
{
    /* Find the starting point of each nodes */
    struct _AGLink Defaults[5];
    char **token, *p, ts = DEF_TABSIZE, wrap = WRAP_OFF;
    AGNode lst;
    AGFile new;

    /* Open and read file content */
    if ((new = CreateFile(filename)) == NULL)
        return NULL;

    /* Scan through the buffer for node information */
    memset(Defaults, 0, sizeof(Defaults));
    for (p = new->Buffer, lst = NULL; *p; p++)
        if (*p == '@' && (p[-1] == '\n' || p[-1] == '\0')) {
            for (token = CommandTokens; *token; token++)
                if (!strncasecmp(p + 1, *token, strlen(*token)))
                    break;

            if (*token == NULL)
                continue;
            /* Mark this command as a known one (so we can skip them later */
            p[1] = AG_COMMAND;
            /* Jump at the end of token, except for the first */
            if (token > CommandTokens)
                p += strlen(*token) + 1;
            switch (token - CommandTokens) {
                case 0: /* End of a node */
                    *p++ = '\0';
                    break;
                case 1: /* Start of a node */
                {
                    AGNode new;
                    if ((new = (AGNode)malloc(sizeof(*new))) != NULL) {
                        memset(new, 0, sizeof(*new));
                        /* Set default index/content/next/prev/... */
                        memcpy(&new->help, Defaults, 3 * sizeof(new->help));
                        PREV(new) = lst;
                        if (lst)
                            NEXT(lst) = new;
                        lst = new;
                        new->tabsize = ts;
                        new->bgpen = DEF_BGPEN;
                        new->wordwrap = wrap;
                        /* Search for interresting information */
                        p = FindNodeNames(new, p) - 1;
                    } else {
                        ThrowError(ERROR_NO_FREE_STORE, NULL);
                        goto panic;
                    }
                } break;
                case 2: /* Separate long-title node */
                    if (lst != NULL)
                        lst->start = p = GetAGString(&lst->title, p, ' ');
                    break;
                case 3: /* Tabstop number */
                    if (lst)
                        lst->tabsize = ParseAGInt(p, '\n', DEF_TABSIZE);
                    else
                        ts = ParseAGInt(p, '\n', DEF_TABSIZE);
                    break;
                case 4:
                case 5:
                case 6:
                case 7:
                case 8: /* Index, Toc, Prev, Next, Help */
                    p = FindAGLinkInfo(
                        (lst == NULL ? Defaults : &lst->help) + (token - CommandTokens - 4),
                        p);
                    if (lst == NULL)
                        *p = '\0';
                    break;
                case 9: /* This node will be wordwrapped */
                    if (lst != NULL)
                        lst->wordwrap = WRAP_LINE;
                    else
                        wrap = WRAP_LINE;
                    break;
                case 10: /* This one smart-wrapped */
                    if (lst != NULL)
                        lst->wordwrap = WRAP_SMART;
                    else
                        wrap = WRAP_SMART;
                    break;
            }
        }

panic:
    if (lst) {
        /* Fill some missing information */
        for (;; lst = PREV(lst)) {
            /* Browsing order can be set only now, as node **
            ** structure need to be build before.          */
            if (lst->next.node == NULL && NEXT(lst))
                lst->next.node = ((AGNode)NEXT(lst))->name,
                lst->next.type = LINK_TO_DOC;
            if (lst->prev.node == NULL && PREV(lst))
                lst->prev.node = ((AGNode)PREV(lst))->name,
                lst->prev.type = LINK_TO_DOC;
            if (PREV(lst) == NULL)
                break;
        }
        new->Content = (AGList)lst;
        /* Information about author, copyright... */
        FindMiscInfo(new);
        /* Build information node */
        AGFileInfo(new, "node", "nodes");
    } else
        ThrowError("File `%s' contains no node!", filename);
    return new;
}

/*** Find some information in the header of the file ***/
void FindMiscInfo(AGFile File)
{
    /* AmigaGuide intructions */
    char *eob = CONTENT(File)->start, *p, **token;
    /* Scan through the first lines for interresting information */
    for (p = File->Buffer; p < eob; p++)
        for (token = InfoTokens; *token; token++)
            if (!strncasecmp(p, *token, strlen(*token)))
                /* Search for the start & end of line */
                p = GetAGString((&File->DBName) + (token - InfoTokens), p + strlen(*token), '\n') - 1;

    /* Count nodes in the file */
    {
        AGNode lst;
        for (lst = CONTENT(File); lst; lst = NEXT(lst), File->NbNodes++)
            ;
    }
}

/*** Search for an node header in an AG file ***/
AGNode FindAGNode(AGFile file, char* NodeName)
{
    AGNode node;
    /* AmigaGuide bug: if internal node name contains spaces, all what **
    ** is contained after and including the first space is ignored...  */
    for (node = CONTENT(file); node && strcasecmp(node->name, NodeName); node = NEXT(node))
        ;
    return node;
}

/*** Search for a predefined color index name ***/
void FillAGStyles(char* pen, char* styles, char* name)
{
    /* Name are taken from <intuition/screen.h> */
    char Pens[] = "34" STR_FGPEN "7"
                  "031" STR_BGPEN "6";
    char Styles[] = "110111110";
    char** token;
    /* Skip starting white spaces */
    while (isspace(*name))
        name++;

    for (token = ColorTokens; *token; token++)
        if (!strncasecmp(name, *token, strlen(*token))) {
            int nb = token - ColorTokens;
            *pen = Pens[nb];
            /* Background pens didn't have their style mdified */
            if (styles)
                if (Styles[nb] == '1')
                    *styles |= FSF_BOLD;
                else
                    *styles &= ~FSF_BOLD;
            else if (*pen == '\0' && nb != 7)
                *pen = '7';
            return;
        }
}

/*** Search for styles in an AmigaGuide node ***/
char CreateAGWords(AGNode node)
{
    AGPara par = NULL;
    AGWord new = NULL;
    AGContext AGC;
    char *buf, *p;

    InitContext(&AGC, node->wordwrap);
    /* Alloc a first paragraph */
    if ((par = NewPara(NULL, &AGC)) == NULL)
        return 0;
    /* Default style, attributes and justification */
    for (p = buf = node->start; *buf; buf++) {
        if (IS_SPECIAL((unsigned char)buf[0])) {
            char c = *buf;
            /* Smartwrapped para ends only with 2 line feeds */
            if (AGC.wrap == WRAP_SMART && c == '\n' && buf[1] != '\n') {
                *buf = ' ';
                continue;
            }

            *buf = '\0';
            if (p < buf)
                new = NewWord(par, new, p, &AGC);
            if (c == '\n')
                node->maxlines++, par = NewPara(par, &AGC), new = NULL;
            else
                new = DisableSpecialChar(par, new, c);
            p = buf + 1;
        } else if (*buf == '\\' && (buf[1] == '@' || buf[1] == '\\')) {
            /* Old version of AmigaGuide (v39 and below) process '\' **
            ** like normal character, whereas newer discard it. Here **
            ** is processed only special characters.                 */
            *buf = '\0';
            if (p < buf)
                new = NewWord(par, new, p, &AGC);
            p = ++buf;

        } else if (*buf == '@') { /* Start of an AmigaGuide style modifier */
            if (buf[1] == '{') {
                char** token;
                *buf = '\0';
                /* Create a word with previous characters */
                if (p < buf)
                    new = NewWord(par, new, p, &AGC);

                /* Search for attribute modifier */
                for (buf += 2, token = AttrTokens; *token; token++) {
                    short len = strlen(*token);
                    if (!strncasecmp(buf, *token, len) && (buf[len] == '}' || token - AttrTokens >= 9)) {
                        /* Search for a known token */
                        switch (token - AttrTokens) {
                            case 0:
                                AGC.style |= FSF_BOLD;
                                break;
                            case 1:
                                AGC.style |= FSF_ITALIC;
                                break;
                            case 2:
                                AGC.style |= FSF_UNDERLINED;
                                break;
                            case 3:
                                AGC.style &= ~FSF_BOLD;
                                break;
                            case 4:
                                AGC.style &= ~FSF_ITALIC;
                                break;
                            case 5:
                                AGC.style &= ~FSF_UNDERLINED;
                                break;
                            case 6:
                                par->align = AGC.align = JM_NORMAL;
                                break;
                            case 7:
                                par->align = AGC.align = JMF_CENTER;
                                break;
                            case 8:
                                par->align = AGC.align = JMF_RIGHT;
                                break;
                            case 9:
                                new = NewIndent(par, new, AGC.indent = ParseAGInt(buf + 7, '}', 0));
                                break;
                            case 10:
                                par->alinea = ParseAGInt(buf + 4, '}', 0);
                                break;
                            case 11:
                                AGC.style = FS_NORMAL;
                                break;
                            case 12:
                                par->alinea = 0;
                                par->align = AGC.align = JM_NORMAL;
                                AGC.fgpen = DEF_FGPEN;
                                AGC.bgpen = DEF_BGPEN;
                                if (AGC.wrap != node->wordwrap || AGC.indent != 0)
                                    new = NewResetPara(par, new, AGC.wrap = node->wordwrap);
                                AGC.indent = 0;
                                break;
                            case 13:
                                new = NewObject(par, new, WrapOFF, 0);
                                AGC.wrap = WRAP_OFF;
                                break;
                            case 14:
                                par = NewPara(par, &AGC);
                                node->maxlines++;
                                new = NULL;
                                break;
                            case 15:
                                new = NewTabs(par, new, buf + 7);
                                AGC.ts = TABSTOP((AGObj) new);
                                break;
                            case 16:
                                new = NewObject(par, new, ClearTabs, 0);
                                AGC.ts = NULL;
                                break;
                            case 17: {
                                char style = AGC.style;
                                AGC.style = FSF_BOLD;
                                new = NewWord(par, new, "AmigaGuide\xa9", &AGC);
                                AGC.style = style;
                            } break;
                            case 18:
                                par = NewPara(par, &AGC);
                                node->maxlines++;
                                new = NULL;
                                if (AGC.wrap != WRAP_SMART)
                                    par = NewPara(par, &AGC), node->maxlines++;
                                break;
                            case 19:
                                FillAGStyles(&AGC.fgpen, &AGC.style, buf + 3);
                                break;
                            case 20:
                                FillAGStyles(&AGC.bgpen, NULL, buf + 3);
                                break;
                            case 21: {
                                AGWord lnk;
                                /* This is a special case */
                                if ((lnk = CreateAGLink(buf + 1)) != NULL)
                                    InsertAGWord(par, lnk, new),
                                        lnk->bgpen = AGC.bgpen, new = lnk;
                            }
                        }
                        break;
                    }
                }
                /* Go through the end of tag */
                while (*buf != '}')
                    buf++;
                *buf = '\0';
                p = buf + 1;

            } else if (buf[1] == AG_COMMAND) {

                /* AmigaGuide commands are situated at beginning of line and **
                ** already processed in CreateAGNodes(), so just skip them.  */
                while (*buf != '\n')
                    buf++;
                *buf = '\0';
                p = buf + 1;
            }
        }
    }
    for (; PREV(par); par = PREV(par))
        ;
    node->AGContent = node->Shown = par;
    node->column = node->line = 0;
    return par ? 1 : 0;
}

/*** Compute real number of space to add before a line ***/
void CalcIndent(AGPara par, short len, short width, short indent)
{
    if (len > width)
        len = width;
    switch ((unsigned char)par->align & ~JMF_PREVIOUS) {
        case JMF_CENTER:
            par->spaces = (width - len + indent) >> 1;
            break;
        case JMF_RIGHT:
            par->spaces = width - len;
            break;
        default:
            par->spaces = indent + par->alinea;
    }
    if (par->spaces < 0)
        par->spaces = 0;
}

/*** Split a line into two part ***/
AGPara SplitLine(AGPara par, AGWord cut, short nbc)
{
    AGPara new, next = NEXT(par);
    AGWord wrd;
    if ((new = NewPara(par, NULL))) {
        if (next)
            PREV(next) = new, NEXT(new) = next;
        /* Paragraph's indenting properties equal previous */
        new->align = par->align | JMF_PREVIOUS;
        new->ts = tabs;

        if (nbc > 0) {
            wrd = NewSplitWord(new, cut, cut->data + nbc + 1);
            NEXT(wrd) = NEXT(cut);
            if (NEXT(cut))
                PREV((AGWord)NEXT(cut)) = wrd;
            NEXT(cut) = NULL;
        } else /* Split the whole word */
        {
            if ((wrd = PREV(cut)))
                NEXT(wrd) = NULL;
            new->line = cut;
            PREV(cut) = NULL;
        }
    }
    return new;
}

/*** Join two lines ***/
AGWord JoinLines(AGPara par)
{
    AGPara next = NEXT(par);
    AGWord ins, wrd = par->line;
    /* Is this a splitted word? */
    if (wrd->style & FSF_SPLITTED)
        wrd = FreeSplit(wrd);
    else if (isspace(wrd->data[-1]))
        wrd->data--;
    for (ins = ((AGPara)PREV(par))->line; NEXT(ins); ins = NEXT(ins))
        ;
    if ((NEXT(ins) = wrd))
        PREV(wrd) = ins;
    if ((NEXT((AGPara)PREV(par)) = next))
        PREV(next) = PREV(par);
    free(par);
    return ins;
}

/*** Makes justifications according to terminal width ***/
char FormatPara(AGNode node, short width)
{
    AGPara par;
    AGWord word;
    ObjPara op;
    short len, lg, spc = 0, indent;

    ts = node->tabsize;
    tabs = NULL;
    node->width = ((op.limit = node->wordwrap) ? width : 0x7fff);
    op.nidt = indent = 0;
    for (par = node->AGContent; par; indent = op.nidt, par = NEXT(par)) {
        char *p = NULL, *q, nbwrd;

    redo:
        for (len = indent + par->alinea, nbwrd = 1, word = par->line; word; word = NEXT(word)) {
            /* Empty word => special object */
            if ((p = word->data) == 0)
                INIT_OBJ((AGObj)word, &op);
            else
            /* Scan through the line */
            noinit:
                do {
                    /* Skip spaces */
                    for (spc = lg = 0, q = p; *p && isspace(*p); p++)
                        lg += (*p == '\t' ? tabstop(len + lg) : 1);
                    /* Skip first spaces */
                    if (par->align & JMF_PREVIOUS && nbwrd == 1 && !word->link)
                        word->data = p, lg = 0;
                    /* Doesn't take care of last spaces */
                    if (*p == '\0')
                        q = p, nbwrd--, spc = lg, lg = 0;
                    /* Then go through the end of word */
                    for (; *p && !isspace(*p); lg++, p++)
                        ;

                    /* Split the word here */
                    if (len + lg > (op.limit ? width : 0x7fff) && nbwrd > 1) {
                        CalcIndent(par, len, width, indent);
                        par = SplitLine(par, word, q - word->data);
                        node->maxlines++;
                        indent = op.nidt;
                        goto redo;
                    } else
                        len += lg + spc;
                    nbwrd++;
                } while (*p);
        }
        /* Should we join the next line? */
        {
            AGPara tmp = NEXT(par);
            char* t;
            if (tmp && (tmp->align & JMF_PREVIOUS) && p) {
                /* This reduce a lot of computing with opaque resizing */
                if ((word = tmp->line))
                    for (t = word->data, spc = 0; *t && !isspace(*t); t++, spc++)
                        ;

                if (len + spc <= width) {
                    if (tmp == node->StartLine)
                        node->StartLine = par;
                    word = JoinLines(tmp);
                    node->maxlines--;
                    goto noinit;
                }
            }
        }
        /* Line that fits the terminal width */
        CalcIndent(par, len, width, indent);
    }
    return 1;
}

/*** Free all ressources allocated for just one node ***/
void FreeAGParas(AGPara Start)
{
    AGWord line, word;
    AGPara para;
    for (para = Start; Start; Start = para) {
        for (word = para->line; word; line = NEXT(word), free(word), word = line)
            ;
        para = NEXT(Start);
        free(Start);
    }
}

/*** Free allocated ressources for an AG file ****/
void FreeAGFile(AGFile File)
{
    if (File) {
        AGNode node, tmp;
        for (node = CONTENT(File); node; node = tmp) {
            tmp = NEXT(node);
            FreeAGParas(node->AGContent);
            free(node);
        }
        free(File);
    }
}

/*** Return the amount of spaces to add to reach the next tabstop ***/
char tabstop(short abs_pos)
{
    if (tabs == NULL)
        return ts - abs_pos % ts;
    else {
        register short* t;
        for (t = tabs; *t && abs_pos >= *t; t++)
            ;
        return (*t ? *t - abs_pos : ts - abs_pos % ts);
    }
}

/*** Rendering of a line in a buffer according to styles ***/
void RenderLine(AGPara para, short start, short max, char ins_mode)
{
    register char *p = buffer, *cp = NULL;
    register short i = (para ? para->spaces : 0);
    extern char underlined;
    AGWord word = (para ? para->line : NULL);

    /* Skip first non-displayed characters */
    if (i < start) {
        for (; word; word = NEXT(word))
            if (word->data)
                for (cp = word->data; *cp && i <= start;) {
                    i += (*cp == '\t' ? tabstop(i) : 1);
                    if (i >= start)
                        goto big_break;
                    cp++;
                }
    big_break:
        if (i == start)
            cp++;
        i = start;
    }

    /* To scroll display down, there is a special escape sequence (\eM), **
    ** that moves cursor up, only if cursor isn't already at topmost pos **
    ** otherwise, display is scrolled. This is how scrolling down works. */
    switch (ins_mode) {
        case INSERT_TOP:
            strcpy(p, "\033[H\033M");
            p += 5;
            break;
        case INSERT_BOTTOM:
            *p++ = '\n';
            break;
    }

    /* There is much spaces than skipped columns */
    if (i > start) {
        short nb = i - start;
        if (nb > max)
            nb = max;
        if (bgpen != '\0')
            *p++ = '\033', *p++ = '[', *p++ = '4', *p++ = bgpen, *p++ = 'm';
        memset(p, ' ', nb);
        p += nb;
    }

    for (max += start; word && i < max; word = NEXT(word)) {
        if (word->data == NULL) {
            INIT_OBJ((AGObj)word, (void*)p);
            continue;
        }
        /* Fill the styles of the `word' */
        *p++ = '\033';
        *p++ = '[';
        *p++ = '0';
        if (word->fgpen)
            *p++ = ';', *p++ = (word->style & FSF_FGSHINE ? '9' : '3'), *p++ = word->fgpen;
        if (word->bgpen)
            *p++ = ';', *p++ = '4', *p++ = word->bgpen;
        if (word->style & FSF_INVERSID)
            *p++ = ';', *p++ = '7';
        if (word->style & FSF_BOLD)
            *p++ = ';', *p++ = '1';
        if (word->style & FSF_ITALIC)
            *p++ = ';', *p++ = '2';
        /* All terminal does not have underscoring possibility */
        if (word->style & FSF_UNDERLINED && (underlined || p[-1] == '0'))
            *p++ = ';', *p++ = '4';
        *p++ = 'm';
        *p = '\0';
        fputs(p = buffer, stdout);

        /* Write content of string */
        if (!cp)
            cp = word->data;
        for (; i < max && *cp;) {
            if (*cp == '\t') {
                short ts = tabstop(i);
                if (i + ts > max)
                    ts = max - i;
                if (ts > EOT(buffer) - p)
                    ts = EOT(buffer) - p;
                else
                    cp++;
                memset(p, ' ', ts);
                i += ts;
                p += ts;
            } else
                *p++ = *cp++, i++;
            /* Buffer full ? */
            if (p == EOT(buffer))
                fputs(p = buffer, stdout);
        }
        cp = NULL;
    }
    *p = 0;
    /* `Clears' end of line */
    if (p > buffer)
        fputs(p = buffer, stdout);
    printf("\033[0m");
    if (bgpen != 0)
        printf("\033[4%cm", bgpen);
    if (i < max)
        printf("%*s", max - i, " ");
}

/*** Print part of content of an AmigaGuide node into stdout ***/
void RenderAGNode(AGPara lst, short nbl, short left, short wid, char mode)
{
    AGPara para;
    short ln;

    for (ln = nbl, para = lst; para && ln;) {
        tabs = para->ts;
        RenderLine(para, left, wid, mode);
        if (nbl > 0)
            ln--, para = NEXT(para);
        else
            ln++, para = PREV(para);
    }
    /* Empty lines at bottom of node? */
    for (; ln > 0; ln--)
        RenderLine(NULL, left, wid, OVERWRITE);
    fflush(stdout);
}

/*** Need to refresh without be bored with weird parameters ***/
void ReRenderAGNode(void)
{
    extern struct scrpos terminfo;
    AGNode node = AGNODE(&terminfo);

    set_cursor_pos(0, 0);
    /* No need to bother with this piece of information between function call */
    ts = node->tabsize;
    bgpen = node->bgpen;
    RenderAGNode(
        node->Shown, terminfo.height - 1,
        node->column, terminfo.width, OVERWRITE);
    Prompt(node->title);
}

/*** Setup some interresting information about AmigaGuide file ***/
void AGFileInfo(AGFile File, char* obj, char* objs)
{
    static char none[] = "none";
    static char title[] = "Information";
    AGPara para;
    AGWord word;
    AGNode info;
    AGContext AGC;

#define NewLine(WRD1, WRD2)                                \
    NewWord(para = NewPara(para, &AGC), NULL, WRD1, &AGC); \
    AGC.style = FSF_BOLD;                                  \
    NewWord(NULL, word, WRD2 ? WRD2 : none, &AGC);         \
    AGC.style = FS_NORMAL

    if ((info = (AGNode)malloc(sizeof(*info) + 50)) != NULL) {
        /* Fill node information header */
        memset(info, 0, sizeof(*info));
        NEXT(info) = File->Content;
        if (File->Content)
            File->Content->prev = info;
        File->Content = (AGList)info;
        info->name = STR_INFONAME;
        info->maxlines = 11;
        info->title = title;
        info->bgpen = DEF_BGPEN;

        InitContext(&AGC, 0);
        sprintf((char*)(info + 1), "%d %s, %ld %s",
            File->NbNodes, File->NbNodes > 1 ? objs : obj,
            File->Length, File->Length > 1 ? "bytes" : "byte");
        /* We must create an `AmigaGuide' node to prevent strange **
        ** size of the console window and default attributes.     */
        para = NewPara(info->Shown = info->AGContent = NewPara(NULL, &AGC), &AGC);
        info++;
        word = NewWord(para, NULL, "AmigaGuide reader v" SVER " build on " __DATE__ ", written by T.Pierron", &AGC);
        para = NewPara(para, &AGC);
        AGC.style = FSF_BOLD;
        word = NewWord(para, 0, "Free software under GNU public license.", &AGC);
        AGC.style = FS_NORMAL;
        word = NewWord(para, word, " Information about ", &AGC);
        AGC.style = FSF_BOLD;
        word = NewWord(NULL, word, File->FName, &AGC);
        para = NewPara(para, &AGC);
        AGC.style = FS_NORMAL;
        word = NewLine("DataBase............. ", File->DBName);
        word = NewLine("Author............... ", File->Author);
        word = NewLine("Copyright............ ", File->Copy);
        word = NewLine("Master............... ", File->Master);
        word = NewLine("Version.............. ", File->SVer);
        word = NewLine("Statistics........... ", (char*)info);
        para = NewPara(para, &AGC);
        AGC.style = FSF_BOLD;
        word = NewWord(NewPara(para, &AGC), NULL, "  Press BS to quit this page.", &AGC);
    }
}

/*** Scroll display to absolute position in file ***/
void ScrollDisplay(struct scrpos* inf, short pos)
{
    AGNode node = AGNODE(inf);
    short skipy = pos - node->line;
    if (skipy < 0)
        skipy = -skipy;

    /* Attempt to optimize scrolling? */
    if (skipy < inf->height - 3) {
        AGPara first = node->Shown;
        int nb;
        /* Find the first line to render */
        if (pos < node->line) {
            /* Scroll backward */
            AGPara para;
            for (nb = skipy, para = first; nb; nb--, para = PREV(para))
                ;
            first = PREV(first);
            node->Shown = para;
        } else {
            /* Scroll forward */
            for (nb = skipy; nb; nb--, first = NEXT(first))
                ;
            node->Shown = first;
            for (nb = inf->height - skipy - 1; first && nb; nb--, first = NEXT(first))
                ;
        }

        /* Starting screen position for refresh */
        skipy = pos - node->line;
        node->line = pos;

        /* Insert new lines */
        set_scroll_region(inf->height - 1);
        set_cursor_pos(inf->height - 1, 0);
        RenderAGNode(first, skipy, node->column, inf->width,
            skipy < 0 ? INSERT_TOP : INSERT_BOTTOM);
    } else {
        /* We want to jump too far, redraw everything */
        AGPara ln = node->Shown;
        if (pos < node->line)
            for (; skipy; skipy--, ln = PREV(ln))
                ;
        else
            for (; skipy; skipy--, ln = NEXT(ln))
                ;
        node->Shown = ln;
        node->line = pos;
        set_cursor_pos(0, 0);
        RenderAGNode(ln, inf->height - 1, node->column, inf->width, OVERWRITE);
    }
}
