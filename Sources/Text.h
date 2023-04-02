/*** Standard functions handling ANSI text files ***/

#ifndef ANSI_TEXT_H
#define ANSI_TEXT_H

#include "AGReader.h"
#include "AGNodes.h"

typedef struct _stream /* Buffer for reading unknown file size */
{
    struct _stream* prev; /* linked list of buffer */
    short size; /* Nb of bytes in the buffer */
    char buffer[1]; /* Start of buffer */
} Stream;

#define STREAM_SIZE 1024 /* Size of block */

/** Create file and structure information **/
AGFile CreateTextNodes(char* filename);

/** Create style-separated words for an ANSI text file **/
char CreateTextWords(AGNode);

/** Create a AGFile directly from a char stream **/
AGFile CreateTextFromStream(char* stream, char* title);

void AdjustColors(AGNode node);

#endif
