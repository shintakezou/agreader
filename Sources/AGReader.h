/*********************************************************************
**** AGReader.h : standard datatypes for parsing AmigaGuide files ****
****              By T.Pierron, free software under GNU license.  ****
*********************************************************************/

#ifndef AGREADER_H
#define AGREADER_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

/* Used to represent linked list */
typedef struct _AGList {
    void *next, *prev;
}* AGList;

/* Internal representation of an AmigaGuide file */
typedef struct _AGFile {
    AGList Content; /* AmigaGuide linked list of nodes */
    char* Buffer; /* Start of buffer (to be free()'ed) */
    void *Buffer2, *Last; /* Buffer allocated by chunk */
    long Length; /* Length in bytes of the buffer */
    char *FName, *DBName; /* Name of AmigaGuide database */
    char *Author, *Copy; /* Miscellaneous information */
    char *Master, *SVer;
    short NbNodes;

}* AGFile;

/* The structure used to represent a current config */
struct scrpos {
    short width, height; /* Width and height of terminal */
    void* node; /* Current node shown */
    AGFile file; /* In current file */
};

/* To set properly common fields */
#define SetNodeFile(ps, n, f) \
    (ps)->node = (AGList)n;   \
    (ps)->file = f

/** Buffer size for reading piped files **/
#define BUF_PIPE_SIZE (1024 - sizeof(void*))

typedef struct _AGBuf {
    struct _AGBuf* Next;
    char Stream[BUF_PIPE_SIZE];
}* AGBuf;

#define QUIT_OK 0
#define QUIT_ERROR 1

void quit(char* msg, int status);

#endif
