/**********************************************************
**** AGNodes.h : Datatypes describing internal Amiga-  ****
****             Guide nodes, by T.Pierron             ****
**********************************************************/

#ifndef AGNODES_H
#define AGNODES_H

#ifndef AGREADER_H
#include "AGReader.h"
#endif

/** Private access to some fields **/
#define CONTENT(file) ((AGNode)file->Content)
#define AGNODE(scr_ptr) ((AGNode)(scr_ptr)->node)
#define NEXT(ptr) (ptr)->lst.next
#define PREV(ptr) (ptr)->lst.prev
#define EOT(table) (table + sizeof(table))

/** AmigaGuide hyper-link, a la HTML **/
typedef struct _AGLink {
    char* node; /* Node to reach */
    char* file; /* File where node remain */
    long line; /* Starting line in file */
    char type; /* Entended data */
}* AGLink;

/** Possible values for `type' field **/
#define UNKNOWN_TYPE 0 /* Generic unknown object type */
#define LINK_TO_DOC 1 /* Link to same/another document */
#define SYSTEM_COMMAND 2 /* A command to be executed by the system */
#define DYNAMIC_FILE 3 /* File will be read through pipes */

/** To represent style-separated words **/
typedef struct _AGWord {
    struct _AGList lst; /* Linked list of words */
    char* data; /* String's characters */
    struct _AGLink* link; /* !NULL if it's an AG link */
    char fgpen, bgpen; /* Fore & background pens */
    char style; /* Underlined / italic / bold ... */
}* AGWord;

/** Possible styles **/
#define FS_NORMAL 0x00
#define FSF_BOLD 0x01
#define FSF_ITALIC 0x02
#define FSF_UNDERLINED 0x04
#define FSF_INVERSID 0x08
#define FSF_FGSHINE 0x10
#define FSF_BGSHINE 0x20
#define FSF_SPLITTED 0x40

/** Default ANSI colors (0 means standard color) **/
#define DEF_FGPEN '\0'
#define DEF_BGPEN '\0'
#define STR_FGPEN "\0" /* Must be equal to DEF_FGPEN */
#define STR_BGPEN "\0" /* Must be equal to DEF_BGPEN */
#define DEF_FGLINK '4'
#define DEF_SYSTEM '5'
#define DEF_UNKNOWNLINK '1'
#define DEF_ACTIVELINK '7'
#define DEF_TABSIZE 8
#define DEF_LINKSTYLE (FSF_BOLD | FSF_UNDERLINED)

/** AmigaGuide paragraph **/
typedef struct _AGPara {
    struct _AGList lst; /* Linked list of paragraphs */
    struct _AGWord* line; /* A single line of words */
    char align; /* Alignment method */
    short alinea; /* First line indenting */
    short spaces; /* Real nb of spaces to add */
    short* ts; /* List of tabstops for this line */
}* AGPara;

/** Possible justification methods **/
#define JM_NORMAL 0x00
#define JMF_LEFT 0x01
#define JMF_CENTER 0x02
#define JMF_RIGHT 0x04
#define JMF_PREVIOUS 0x80

/** Main structure representing part of an AG file **/
typedef struct _AGNode {
    struct _AGList lst; /* Linked list of AG nodes */
    struct _AGLink help; /* Node for help */
    struct _AGLink index, toc; /* Main index/Table of content */
    struct _AGLink prev, next; /* Document browsing order */
    struct _AGPara* AGContent; /* Content of the node */
    struct _AGPara* Shown; /* First displayed line */
    struct _AGWord* ActiveLink; /* Activated link */
    struct _AGPara* StartLine; /* Activated line where link is */
    short ActiveLine; /* Line number equivalent */
    short line, column; /* Number of first line/col shown */
    short width; /* Width for which node has been formatted */
    char* name; /* Name of the node */
    char* title; /* Longer description of the node */
    char* start; /* Start of displayable buffer */
    long maxlines; /* Max number of lines in the node */
    char bgpen; /* Default background color */
    char tabsize; /* tabstop size for the node */
    char wordwrap; /* Word wrapping method */
}* AGNode;

/** Possible word wrapping methods **/
#define WRAP_OFF 0
#define WRAP_LINE 1
#define WRAP_SMART 2

/** Special name for information node **/
#define STR_INFONAME ""

AGFile CreateFile(char*); /* Alloc and fill buffer for a file */
AGFile CreateAGNodes(char*); /* Build AmigaGuide structure only */
AGNode FindAGNode(AGFile, char*); /* Returns node struct corresponding to the name */
char FormatPara(AGNode, short); /* Formats paragraph according to justif. */
char CreateAGWords(AGNode); /* Build rendering information */
void FreeAGFile(AGFile); /* Free all ressources */
void FindMiscInfo(AGFile); /* Search for AmigaGuide-file information */
void ReRenderAGNode(void); /* Refresh last visited node */
char tabstop(short pos); /* Amount of spaces to add to reach tabstop  */

void RenderAGNode(AGPara, short nbl, short left, short wid, char mode);
void RenderLine(AGPara para, short start, short max, char ins_mode);
void ScrollDisplay(struct scrpos*, short pos);
void AGFileInfo(AGFile, char* obj, char* objs);
char* FindAGLinkInfo(AGLink link, char* format);

/*** Special character may trashed the display ***/
AGWord DisableSpecialChar(AGPara par, AGWord old, unsigned char);

#define IS_SPECIAL(c) (c != '\t' && (c < 32 || (128 <= c && c < 160)))

/** Possible values for `mode' parameter **/
#define INSERT_TOP 0 /* Insert sequence \033[H\033M at beginning */
#define INSERT_BOTTOM 1 /* Insert sequence \n at beginning */
#define OVERWRITE 2 /* Print line as-is */

/** Classical error message **/
#define ERROR_NO_FREE_STORE ((char*)-1)

#endif
