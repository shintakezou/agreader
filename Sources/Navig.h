/*** Standard procedures for navigation through AG files ***/

#ifndef NAVIG_H
#define NAVIG_H

/** Environment variable that may contain a alternate path for AmigaDOS named device **/
#define AGR_PATH_ENV_NAME "AGR_PATH"

/** Stacked nodes **/
typedef struct _Stack {
    struct _Stack* prev;
    struct _AGFile* file;
    struct _AGNode* node;
    char free;
}* Stack;

/** Highlight the next/previous link, if possible **/
void FindNextLink(struct scrpos*, char dir);

/** Simple file-type checker **/
char WhatIs(char* GuideName, AGLink, char** path);

/** Returns to the previous visited page **/
void HistoryBack(struct scrpos*);

/** Free all ressources allocated for stack **/
void PopAGNodes(void);

/** Search for node/file pointed by link and display it **/
int Navigate(char* GuideName, AGLink);

void SetActiveLine(AGNode node);

/** Function use to create file / node **/
typedef AGFile (*pfnCreateNodes)(char* path);
typedef char (*pfnCreateWords)(AGNode node);

/** Constants returned by WhatIs() **/
#define LOCAL_NODE 1
#define EXTERN_NODE 2
#define PICTURE 3
#define TEXT_FILE 4
#define NOT_LOCATED 99

#endif
