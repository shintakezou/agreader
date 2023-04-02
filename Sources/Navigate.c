/******************************************************
** Navigate.c : Manage navigation between different  **
**              AmigaGuide file, by T.Pierron        **
**---------------------------------------------------**
** Started on 19/9/2000, free software under GNU PL  **
******************************************************/

#include "AGNodes.h"
#include "AGObj.h"
#include "AGReader.h"
#include "AG_lib.h"
#include "IO_tty.h"
#include "Input.h"
#include "Navig.h"
#include "Text.h"
#include <ctype.h>
#include <string.h>

extern char buffer[512];
extern short* tabs;
char command[] = "xv %s"; /* Command executed for viewing picture */

/** Manage history of visited pages **/
static Stack stack = NULL, first = NULL;

void AmigaToUnixPath(char*, char*, short);

/*** System-specific execute command ***/
int myExecute(char* fmt, char* arg)
{
    char *space, *drive, *dest = NULL;
    int pid;

    /* Command comes from AmigaOS, try to convert to Unix */
    if (strncasecmp(fmt, "Run ", 4) == 0)
        fmt += 4;
    /* Get command name */
    if ((space = strchr(fmt, ' ')))
        *space = 0;
    /* Get device specifier */
    if ((drive = strchr(fmt, ':'))) {
        *drive = 0;
        if (NULL != (dest = getenv(fmt)) || NULL != (dest = getenv(AGR_PATH_ENV_NAME))) {
            strcpy(buffer, dest);
            dest = strchr(buffer, 0);
            if (dest[-1] != '/')
                *dest++ = '/';
            AmigaToUnixPath(drive + 1, dest, 255);
        }
        fmt = drive + 1;
        *drive = ':';
    }
    if (space != NULL)
        *space = ' ';
    /* Write command args */
    sprintf(dest ? dest : buffer, fmt, arg);

    /* system() waits child process finishes... */
    switch (pid = fork()) {
        case -1:
            return -1;
        case 0:
            close(1);
            close(2); /* Don't trash the display */
            execlp("/bin/sh", "sh", "-c", buffer, NULL);
            ThrowError("Failed executing `%s'.", buffer);
            exit(0);
        default:
            break;
    }
    return pid;
}

/*** Go through the end of a line ***/
AGWord EndOfLine(AGPara start)
{
    AGWord last = NULL;
    if (start && start->line)
        for (last = start->line; NEXT(last); last = NEXT(last))
            ;
    return last;
}

/*** Returns words of a paragraph ***/
AGWord WordsPara(AGPara para)
{
    return para ? para->line : NULL;
}

/*** Find the line number of activated link ***/
void SetActiveLine(AGNode node)
{
    AGPara par;
    long nb;

    if (node->ActiveLink) {
        for (nb = 0, par = node->AGContent; par; par = NEXT(par), nb++)
            if (par == node->StartLine)
                goto stop;

        node->ActiveLink->fgpen = '1';
        node->ActiveLink = NULL;
        return;
    stop:
        node->ActiveLine = nb;
    }
}

/*** Go through the next word ***/
void NextLink(AGNode src, char dir)
{
    if (dir == 1)
        if (src->ActiveLink)
            src->ActiveLink = NEXT(src->ActiveLink);
        else
            src->ActiveLine++, src->ActiveLink = WordsPara(src->StartLine = NEXT(src->StartLine));
    else if (src->ActiveLink)
        src->ActiveLink = PREV(src->ActiveLink);
    else
        src->ActiveLine--, src->ActiveLink = EndOfLine(src->StartLine = PREV(src->StartLine));
}

/*** AG Link finder, to work with TAB key ***/
void FindNextLink(struct scrpos* inf, char dir)
{
    AGNode src = (AGNode)inf->node;

    /* If there was a previous active link, reset styles */
    if (src->ActiveLink)
        src->ActiveLink->bgpen = DEF_BGPEN,
        src->ActiveLink->style = DEF_LINKSTYLE;

    /* If there is no previous selected link, or it is now outside **
    ** the visible area, choose a new one, starting from topline.  */
    if (src->ActiveLink == NULL || src->ActiveLine < src->line || src->ActiveLine >= src->line + inf->height - 1) {
        short nb;
        src->ActiveLink = WordsPara(src->StartLine = src->Shown);
        src->ActiveLine = src->line;
        /* Start at bottom of screen, if backward scan */
        if (dir == -1) {
            for (nb = inf->height - 2; nb && NEXT(src->StartLine);
                 nb--, src->StartLine = NEXT(src->StartLine), src->ActiveLine++)
                ;
            src->ActiveLink = EndOfLine(src->StartLine);
        }

    } else {
        /* Unhilight previous visible link */
        tabs = src->StartLine->ts;
        set_cursor_pos(src->ActiveLine - src->line + 1, 0);
        RenderLine(src->StartLine, src->column, inf->width, OVERWRITE);
        NextLink(src, dir);
    }

    /* Search for the next available link following current position */
    while (src->StartLine) {
        AGWord wrd = src->ActiveLink;
        if (wrd && wrd->data != NULL && wrd->link)
            break;
        NextLink(src, dir);
        /* If `cursor' go outside visible area, resets it */
        if (src->ActiveLine >= src->line + inf->height - 1 || src->ActiveLine < src->line) {
            src->ActiveLink = NULL;
            break;
        }
    }
    /* Hilite the new one */
    if (src->ActiveLink) {
        src->ActiveLink->bgpen = DEF_ACTIVELINK;
        src->ActiveLink->style = DEF_LINKSTYLE | FSF_INVERSID;

        tabs = src->StartLine->ts;
        set_cursor_pos(src->ActiveLine - src->line + 1, 0);
        RenderLine(src->StartLine, src->column, inf->width, OVERWRITE);
    }
    set_cursor_pos(inf->height, 6);
    fflush(stdout);
}

/*** Convert a string to a environment variable ***/
char* ToEnv(char* src, char* dest, char eos)
{
    char* p;
    /* Unix variables are case-sensitive, but AmigaDOS   **
    ** aren't Therefore to avoid mixed-case device name, **
    ** we write them always using upper case letters.    */
    for (p = dest; *src && *src != eos; *p++ = toupper(*src), src++)
        ;
    *p = '\0';
    return dest;
}

/*** Convert AmigaDOS path to Unix one ***/
void AmigaToUnixPath(char* src, char* dst, short max)
{
    char parent;

    /* Naming conventions are very closer, but need some **
    ** conversion to be fully compliant with Unix specs. */
    if (*src == ':')
        *dst++ = '/', src++;
    for (parent = 1; max > 0 && *src; max--, src++) {
        if (*src == '/' && parent)
            *dst++ = '.', *dst++ = '.', max -= 2;
        *dst++ = *src;
        parent = (*src == '/' ? 1 : 0);
    }
    *dst = '\0';
}

/** Simply extract the basename of a path **/
char* basename(char* path, char* dest)
{
    char* p;
    int nb;
    p = strrchr(path, '/');
    if (p != NULL)
        memcpy(dest, path, nb = p - path + 1);
    else
        nb = 0;
    return dest + nb;
}

/** Open a file from env variable **/
int OpenFromENV(char* envname, char* filename)
{
    char* path;
    if (NULL != (path = getenv(envname))) {
        int len = strlen(path);
        if (len <= 256) {
            memcpy(buffer, path, len);
            path = buffer + len;
            /* Some env may not have a final slash */
            if (path[-1] != '/')
                *path++ = '/';
            AmigaToUnixPath(filename, path, 256);

            return open(buffer, O_RDONLY);
        }
    }
    return -1;
}

/*** Try to locate a file, depending where program runs ***/
char* LocateFile(char* guidename, char* filename)
{
    char *p = NULL, *path, isdev = 0;
    int fd;

    /* Retrieve directory where remains the document */
    path = basename(guidename, buffer);

    /* If no guide name is provided, therefore filename comes **
    ** from Unix command line, and should not be translated.  */
    if (*guidename) {
        /* Check if there is a device name in the path */
        for (p = filename; *p && *p != ':'; p++)
            ;
        /* This is a totally Amiga specific path component... */
        if (*p == ':' && p > filename)
            p++, isdev = 1;
        else
            p = filename;

        /* Translate the Amiga path next to the unix one */
        AmigaToUnixPath(p, path, 256);
    } else
        strcpy(buffer, filename);

    /* Try to open the path from cwd, without the device name */
    if ((fd = open(path, O_RDONLY)) != -1)
        goto malloc_it;

    /* Failed! Look for an environment variable describing device name */
    if (isdev && -1 != (fd = OpenFromENV(ToEnv(filename, buffer, ':'), p)))
        goto malloc_full;

    /* Failed! Try to locate from where first document remains */
    if (first != NULL) {
        path = basename(first->file->FName, buffer);
        AmigaToUnixPath(p, path, 256);
        if ((fd = open(buffer, O_RDONLY)) != -1)
            goto malloc_full;
    }

    /* Failed! Last ressort: use a default path */
    if (-1 != (fd = OpenFromENV(AGR_PATH_ENV_NAME, p)))
        goto malloc_full;

    return NULL;

malloc_full:
    path = buffer;
malloc_it:
    close(fd);
    if ((p = (char*)malloc(strlen(path) + 1)) != NULL)
        return strcpy(p, path);
    else
        return NULL;
}

/*** Simple scan for file type ***/
char WhatIs(char* GuideName, AGLink link, char** path)
{
    int fd, len;

    /* This routine is far to be exhaustive, but covers 90% of possible cases */
    if (link->file == NULL)
        return LOCAL_NODE;

    if ((*path = LocateFile(GuideName, link->file)) != NULL) {
        extern char AGHeader[];
        char* p;

        /* Read a few bytes from this file */
        if ((fd = open(*path, O_RDONLY)) == -1)
            return NOT_LOCATED;

        buffer[len = read(fd, buffer, 128)] = '\0';
        close(fd);

        if (len == 0)
            return UNKNOWN_TYPE;
        if (!strncasecmp(buffer, AGHeader, strlen(AGHeader)))
            /* Target is an AmigaGuide file */
            return EXTERN_NODE;

        if (!strncasecmp(buffer, "FORM", 4) && !strncasecmp(buffer + 8, "ILBM", 4))
            /* IFF ILBM picture have a such chunk Id */
            return PICTURE;

        p = link->file + strlen(link->file);
        /* That's coarse, but sometimes works */
        if (!strncasecmp(p - 3, "gif", 3) || !strncasecmp(p - 3, "jpg", 3) || !strncasecmp(p - 4, "jpeg", 4))
            return PICTURE;

        /* Check for a few bytes, whether it's ASCII text */
        for (p = buffer + len - 1; (char*)p >= buffer; p--) {
            unsigned char u = *p;
            /* Non-ISO-8859-1 characters */
            if (u > 126 && u < 160)
                break;
            if (u < 32 && u != '\r' && u != '\n' && u != '\t' && u != 27)
                break;
        }
        if ((char*)p < buffer)
            return TEXT_FILE;

        return UNKNOWN_TYPE;
    }
    return NOT_LOCATED;
}

/*** Find the Nth line of a node ***/
void FindNth(AGNode node, long line, struct scrpos* inf)
{
    AGPara ln;

    if (line + inf->height - 1 > node->maxlines)
        line = node->maxlines - inf->height + 1;
    if (line < 0)
        line = 0;

    node->line = line;
    for (ln = node->AGContent; line; line--, ln = NEXT(ln))
        ;
    node->Shown = ln;
}

void PushAGNode(AGFile file, AGNode node, char free)
{
    Stack new;
    if ((new = (Stack)malloc(sizeof(*new))) != NULL) {
        if (first == NULL)
            first = new;
        new->prev = stack;
        stack = new;
        new->file = file;
        new->node = node;
        new->free = free;
    }
}

/*** Free all allocated things for a stack ***/
void FreeStack(Stack s)
{
    if (s->free) {
        if (s->file->FName)
            free(s->file->FName);
        FreeAGFile(s->file);
    }
    free(s);
}

/*** Go back to the previous visited page ***/
void HistoryBack(struct scrpos* scr)
{
    /* It should have at least one node */
    if (stack && stack->prev) {
        Stack prev = stack->prev;
        SetNodeFile(scr, prev->node, prev->file);
        if (AGNODE(scr)->width != scr->width)
            FormatPara(AGNODE(scr), scr->width),
                SetTopLine(scr->node),
                SetActiveLine(scr->node);
        ReRenderAGNode();
        FreeStack(stack);
        stack = prev;
    }
}

/*** Free all ressources allocated for stack ***/
void PopAGNodes(void)
{
    Stack tmp;
    for (; stack; tmp = stack->prev, FreeStack(stack), stack = tmp)
        ;
}

/*** Search for node/file pointed by link and display it ***/
int Navigate(char* Guide, AGLink link)
{
    extern struct scrpos terminfo;
    pfnCreateNodes CreateN;
    pfnCreateWords CreateW;
    AGNode node;
    char* path = NULL;

    switch (link->type) {
        case LINK_TO_DOC:
            /* Search for new file location, according to `Guide' one */
            switch (WhatIs(Guide, link, &path)) {
                case LOCAL_NODE:
                    /* Search for entry node information */
                    if ((node = FindAGNode(terminfo.file, link->node)) != NULL) {
                        if (node->AGContent == NULL)
                            CreateAGWords(node);
                        if (node->width != terminfo.width)
                            FormatPara(node, terminfo.width),
                                SetActiveLine(node);
                        /* Set the first displayed line */
                        FindNth(node, link->line, &terminfo);
                        if (node != (AGNode)terminfo.node) {
                            terminfo.node = node;
                            PushAGNode(terminfo.file,
                                terminfo.node, 0);
                        }
                        if (is_rawmode())
                            ReRenderAGNode();
                    } else
                        ThrowError("Can't find node `%s'.", link->node);
                    return 1;
                case EXTERN_NODE:
                    CreateN = CreateAGNodes;
                    CreateW = CreateAGWords;
                    goto case_NEW_FILE;
                case TEXT_FILE:
                    CreateN = CreateTextNodes;
                    CreateW = CreateTextWords;
                case_NEW_FILE : {
                    AGFile new;
                    /* Load and create the structure of the new file */
                    if ((new = CreateN(path)) != NULL) {
                        /* The the requested node */
                        if ((node = FindAGNode(new, link->node)) != NULL) {
                            /* Yes it exists! */
                            CreateW(node);
                            FormatPara(node, terminfo.width);
                            PushAGNode(new, node, 1);
                            SetNodeFile(&terminfo, node, new);
                            FindNth(node, link->line, &terminfo);
                            if (is_rawmode())
                                ReRenderAGNode();
                            return 1;

                        } else
                            ThrowError("Node `%s' does not exist in target file.",
                                link->node);

                        /* Something has failed */
                        FreeAGFile(new);
                    }
                } break;
                case PICTURE:
                    /* Execute asynchronously a new process */
                    myExecute(command, path);
                    break;
                case NOT_LOCATED:
                    /* Couldn't find the file! */
                    ThrowError("Can't locate file `%s'.", link->file);
                    break;
                default:
                    /* Unknown target file-type */
                    ThrowError("Unknown datatype in file `%s'.", link->file);
                    break;
            }
            break;
        case SYSTEM_COMMAND:
            /* The command string is normally designed to be executed on **
            ** an Amiga system, but as path and command differ notably,  **
            ** it would quite impossible to translate it into a Unix env */
            myExecute(link->node, NULL);
            break;
        case DYNAMIC_FILE: {
            AGFile new;
            /* Load and create the structure of the new file */
            if ((new = CreateTextNodes(NULL)) != NULL) {
                node = (AGNode) new->Content;
                node->title = "Standard input";
                PushAGNode(new, node, 1);
                SetNodeFile(&terminfo, node, new);
                FindNth(node, link->line, &terminfo);
                return 1;
            }
        } break;
        case UNKNOWN_TYPE:
            ThrowError("Unknown link type (%s)", link->node);
            break;
    }
    /* If this point is reached, something has failed */
    if (path)
        free(path);
    return 0;
}
