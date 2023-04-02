/****************************************************
** Input.c: Get and process key-in characters.     **
**          by T.Pierron, 11/9/2000, free software **
****************************************************/

#include "AGNodes.h"
#include "AGReader.h"
#include "Help.h"
#include "IO_tty.h"
#include "Navig.h"
#include "Text.h"
#include <string.h>

static char InfDisp = 0; /* 1 if information screen is displayed */
static char TmpBuf[80]; /* Buffer used for various output */
static AGFile AGHelp = NULL; /* Reference guide is requested */

/*** Print a prompt on stdout at the bottom of screen ***/
void Prompt(char* str)
{
    extern struct scrpos terminfo;
    AGNode node = AGNODE(&terminfo);
    char svg[2] = { 0, 0 };
    short plen;

    if (str == NULL)
        str = node->name;
    plen = strlen(str);

    if (terminfo.width >= 8) {
        /* Fucking printf doesn't cut string if larger than * */
        if (plen > terminfo.width - 7)
            memcpy(svg, str + (plen = terminfo.width - 8), 2),
                memcpy(str + plen, "\xbb", 2);

        printf("\033[%d;H\033[0;7m%4d%%%*s\033[0m\033[%d;6H",
            terminfo.height, (node && node->maxlines > terminfo.height ? (100 * node->line) / ((int)node->maxlines - terminfo.height + 1) : 100),
            terminfo.width - 5, str, terminfo.height);
        if (svg[0])
            memcpy(str + plen, svg, 2);
    } else /* Strange terminal size... */
        printf("\033[%d;H\033[0;7m%*s\033[0m", terminfo.height, terminfo.width - 1, "");

    fflush(stdout);
}

/*** Horizontal scroll ***/
void scroll_hori(struct scrpos* inf, short step)
{
    long pos = AGNODE(inf)->column + step;

    if (pos < 0)
        pos = 0;
    if (pos != AGNODE(inf)->column)
        /* A whole redraw is required */
        AGNODE(inf)->column = pos,
        ReRenderAGNode();
}

/*** Callback to ScrollDisplay, with boundary check ***/
void scroll_vert(struct scrpos* inf, short step)
{
    long pos = AGNODE(inf)->line + step, tmp;

    if (pos > (tmp = AGNODE(inf)->maxlines - inf->height + 1))
        pos = tmp;
    if (pos < 0)
        pos = 0;

    if (pos != AGNODE(inf)->line)
        ScrollDisplay(inf, pos), Prompt(AGNODE(inf)->title);
}

/*** Be sure that the first line shown is the right one ***/
void SetTopLine(AGNode node)
{
    short nb = node->line;
    AGPara par;
    if (nb >= node->maxlines)
        nb = node->maxlines - 1;
    for (par = node->AGContent; nb--; par = NEXT(par))
        ;
    node->Shown = par;
}

/*** Display an error message in the status bar ***/
void ThrowError(char* msg, char* param)
{
    char *d, *s;
    int i; /* silence warnings */

    /* Frequently error message encountered */
    if (msg == ERROR_NO_FREE_STORE)
        msg = "Not enough memory!";

    /* Can do better, I know */
    for (d = TmpBuf, s = msg; *s && d - TmpBuf < sizeof(TmpBuf); *d++ = *s++)
        if (*s == '%' && s[1] == 's' && param)
            msg = s + 2, s = param, param = (char*)1;

    if (param == (char*)1)
        for (s = msg; *s && d - TmpBuf < sizeof(TmpBuf); *d++ = *s++)
            ;

    /* If GUI isn't already set, display on stderr */
    if (is_rawmode())
        *d = '\0', Prompt(TmpBuf);
    else /* fputs doesn't write any \n */
        *d++ = '\n', i = write(2, TmpBuf, d - TmpBuf);
}

/*** Toggle display between node and information ***/
static AGList old;
void toggle_disp(struct scrpos* inf)
{
    AGList strinfo;
    if (InfDisp)
        inf->node = old;
    else
        /* The node may not yet exists */
        if ((strinfo = (AGList)FindAGNode(inf->file, STR_INFONAME)))
            old = inf->node, inf->node = strinfo;
        else
            return;

    ReRenderAGNode();
    InfDisp = !InfDisp;
}

/*** Display internal reference guide ***/
void toggle_help(struct scrpos* inf)
{
#ifdef STR_HELP
    static char HelpStr[] = STR_HELP;

    if (InfDisp)
        toggle_disp(inf);
    else if (AGHelp)
        goto toggle;
    else {
        if ((AGHelp = CreateTextFromStream(HelpStr, "Reference guide")) != NULL) {
            if (CreateTextWords((AGNode)AGHelp->Content)) {
            toggle:
                old = inf->node;
                inf->node = AGHelp->Content;
                ReRenderAGNode();
                InfDisp = 1;
                return;
            }
            FreeAGFile(AGHelp);
        }
        ThrowError(ERROR_NO_FREE_STORE, NULL);
    }
#endif
}

/*** Change current tabstop node and show in the toolbar ***/
void ChangeTabstop(AGNode node, short step)
{
    short ts = node->tabsize + step;

    if (ts < 1)
        ts = 1;
    if (ts > 127)
        ts = 127;
    if (ts != node->tabsize)
        node->tabsize = ts, ReRenderAGNode();

    sprintf(TmpBuf, "tabstop = %d ", ts);
    Prompt(TmpBuf);
}

/*** Process key-in commands ***/
void ProcessKeys(void)
{
    static char* Keys[] = {
        "OA", "OB", "OC", "OD", /* Cursor keys */
        "[5~", "[6~", "[H", "[F", /* PgDown, PgUp, End, Home */
        "[1~", "[4~", "OH", "OF", /* 2x Alternative End, Home */
        "[11~", "[12~", "[13~", /* F1, F2, F3 */
        "OP", "OQ", "OR", /* Alternative F1, F2, F3 */
        "[[A", "[[B", "[[C", NULL /* Linux console F1, F2, F3 */
    };
    static char* StrNode[] = {
        "help", "index", "table of content"
    };
    extern struct scrpos terminfo;
    static char buffer[10], *p;

    /* Display node's name in status bar */
    Prompt(AGNODE(&terminfo)->title);
    for (p = buffer;;) {

        /* Get a single char from stdin */
        *p = getchr();
        /* A escape char indicates beginning of command */
        if (*p == '\033')
            *(p = buffer) = '\033';

        *++p = 0;

        /* Special sequence ? */
        if (buffer[0] == '\033') {
            char** key;
            if (p - buffer > 2) {
                for (key = Keys; *key; key++)
                    if (!strcmp(*key, buffer + 1))
                        break;

                /* A key has been found ! */
                if (*key) {
                    switch (key - Keys) {
                        case 0:
                            scroll_vert(&terminfo, -1);
                            break;
                        case 1:
                            scroll_vert(&terminfo, 1);
                            break;
                        case 2:
                            scroll_hori(&terminfo, 5);
                            break;
                        case 3:
                            scroll_hori(&terminfo, -5);
                            break;
                        case 4:
                            scroll_vert(&terminfo, -terminfo.height + 2);
                            break;
                        case 5:
                            scroll_vert(&terminfo, terminfo.height - 2);
                            break;
                        case 6:
                            scroll_vert(&terminfo, -0x7fff);
                            break;
                        case 7:
                            scroll_vert(&terminfo, 0x7fff);
                            break;
                        case 8:
                            scroll_vert(&terminfo, -0x7fff);
                            break;
                        case 9:
                            scroll_vert(&terminfo, 0x7fff);
                            break;
                        case 10:
                            scroll_vert(&terminfo, -0x7fff);
                            break;
                        case 11:
                            scroll_vert(&terminfo, 0x7fff);
                            break;
                        case 12:
                            p[-1] = '1';
                            goto singleton;
                        case 13:
                            p[-1] = '2';
                            goto singleton;
                        case 14:
                            p[-1] = '3';
                            goto singleton;
                        case 15:
                            p[-1] = '1';
                            goto singleton;
                        case 16:
                            p[-1] = '2';
                            goto singleton;
                        case 17:
                            p[-1] = '3';
                            goto singleton;
                        case 18:
                            p[-1] = '1';
                            goto singleton;
                        case 19:
                            p[-1] = '2';
                            goto singleton;
                        case 20:
                            p[-1] = '3';
                            goto singleton;
                    }
                    p = buffer;
                } else
                    goto singleton;
            } else
                goto singleton;
        } else {
        /* A single-char command */
        singleton:
            switch (p[-1]) {
                case '1':
                case '2':
                case '3': {
                    AGLink link = &(AGNODE(&terminfo)->help) + (p[-1] - '1');
                    if (InfDisp != 0)
                        break;
                    if (link->node != NULL)
                        Navigate(terminfo.file->FName, link);
                    else
                        ThrowError(
                            "No %s found in this node.",
                            StrNode[p[-1] - '1']);
                } break;
                case '\b':
                case 127:
                    if (InfDisp)
                        toggle_disp(&terminfo);
                    else
                        HistoryBack(&terminfo);
                    break;
                case 'h':
                    toggle_help(&terminfo);
                    break;
                case 'a':
                case 'p':
                    FindNextLink(&terminfo, -1);
                    break;
                case '\t':
                    FindNextLink(&terminfo, 1);
                    break;
                case ' ':
                case '\n':
                case '\r':
                    if (AGNODE(&terminfo)->ActiveLink)
                        Navigate(terminfo.file->FName, AGNODE(&terminfo)->ActiveLink->link);
                    break;
                case 'q':
                case 'Q':
                    goto exit_for;
                case 'r':
                case 'R':
                case '\f':
                    ReRenderAGNode();
                    break;
                case 'g':
                    scroll_vert(&terminfo, -0x7fff);
                    break;
                case 'G':
                    scroll_vert(&terminfo, 0x7fff);
                    break;
                /* Broken keyboard ? */
                case 'i':
                    scroll_vert(&terminfo, -1);
                    break;
                case 'k':
                    scroll_vert(&terminfo, 1);
                    break;
                case 'j':
                    scroll_hori(&terminfo, -5);
                    break;
                case 'l':
                    scroll_hori(&terminfo, 5);
                    break;
                case 'J':
                    scroll_hori(&terminfo, -0x7fff);
                    break;
                case 'I':
                    scroll_vert(&terminfo, -terminfo.height + 2);
                    break;
                case 'K':
                    scroll_vert(&terminfo, terminfo.height - 2);
                    break;
                case '?':
                case ',':
                    toggle_disp(&terminfo);
                    break;
                case '+':
                    ChangeTabstop(AGNODE(&terminfo), 1);
                    break;
                case '-':
                    ChangeTabstop(AGNODE(&terminfo), -1);
                    break;
                case 't':
                    ChangeTabstop(AGNODE(&terminfo), 0);
                    break;
                case 'C':
                    /* Convert colors of node */
                    AdjustColors(AGNODE(&terminfo));
                    ReRenderAGNode();
                    break;
                case 'u':
                case 'U':
                    /* Underlined mode can be sometimes disturbing, disable it */
                    {
                        extern char underlined;
                        underlined = !underlined;
                        ReRenderAGNode();
                    }
                    break;
                case 'v':
                case 'V':
                    /* Display content of a system comand link */
                    if (AGNODE(&terminfo)->ActiveLink)
                        Prompt(AGNODE(&terminfo)->ActiveLink->link->node);
                    break;
                case '=':
                    /* Display line statistics */
                    sprintf(TmpBuf, "line %d of %d ", AGNODE(&terminfo)->line + 1,
                        (int)AGNODE(&terminfo)->maxlines);
                    Prompt(TmpBuf);
                    break;
                case 'b':
                case 'B':
                    /* Browse backward through the nodes */
                    if (AGNODE(&terminfo)->prev.node)
                        Navigate(terminfo.file->FName, &AGNODE(&terminfo)->prev);
                    break;
                case 'n':
                case 'N':
                    /* Browse forward through the nodes */
                    if (AGNODE(&terminfo)->next.node)
                        Navigate(terminfo.file->FName, &AGNODE(&terminfo)->next);
                    break;
            }
        }
        /* Cancel command, if too long */
        if (p - buffer > sizeof(buffer) - 2)
            p = buffer;
    }
exit_for:
    if (AGHelp)
        FreeAGFile(AGHelp);
}
