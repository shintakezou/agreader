/**********************************************************
**** AGObj.h : Datatypes for custom AmigaGuide Objects ****
****           T.Pierron, 15/02/2001, Free software    ****
**********************************************************/

#ifndef AGOBJ_H
#define AGOBJ_H

#include "AGReader.h"
#include "AGNodes.h"

/** Context for one given position in an AG node **/
typedef struct
{
    char fgpen; /* Foreground pen color number */
    char bgpen; /* Background */
    char style; /* C.f. FSF_* in AGNodes.h */
    char align; /* Justification method C.f JMF_* */
    char indent; /* Amount of spaces to add before/after */
    char wrap; /* 1, if node is smartwrap'ed */
    short* ts; /* Tabstop series */
} AGContext;

/** Special objects argument passing **/
typedef struct
{
    short nidt;
    short limit;
} ObjPara;

/** When rendering an special objet, simply call this function **/
struct _AGObj;
typedef void (*AGInit)(struct _AGObj*, ObjPara*);

/** Special object that have specifie properties **/
typedef struct _AGObj {
    struct _AGList lst; /* Linked list mixed with AGWord */
    char* data; /* Always NULL */
    AGInit func; /* Function instanciating object */
    /* Special field follows here */
}* AGObj;

#define INIT_OBJ(obj, param) ((AGObj)obj)->func((AGObj)obj, param)

/** Special private fields for **/
#define LINDENT(obj) *((short*)(obj + 1))
#define TABSTOP(obj) ((short*)(obj + 1))
#define WRAP(obj) ((char*)(obj + 1))[sizeof(short)]

/*** Init default styles/indent for one node ***/
void InitContext(AGContext*, char wrap);

/** Simple paragraph creation **/
AGPara NewPara(AGPara old, AGContext*);

/** Standard AmigaGuide word creation **/
AGWord NewWord(AGPara par, AGWord old, char* data, AGContext*);

/** AmigaGuide ling creation (fixed style) **/
AGWord CreateAGLink(char*);

/** Object that do not require special field (@{cleartabs}, @{code}...) **/
AGWord NewObject(AGPara par, AGWord old, AGInit pfnInit, size_t);

/** New indenting object **/
AGWord NewIndent(AGPara par, AGWord old, short indent);

/*** Reset paragraph defaults ***/
AGWord NewResetPara(AGPara par, AGWord old, char wrap);

/*** Object containing series of tab stops in spaces ***/
AGWord NewTabs(AGPara par, AGWord old, char* fmt);

void InsertAGWord(AGPara par, AGWord new, AGWord ins);

/** pfnInit parameter for NewObject ***/
void WrapOFF(AGObj, ObjPara*);
void ClearTabs(AGObj, ObjPara*);

AGWord FreeSplit(AGWord);
AGWord NewSplitWord(AGPara, AGWord, char*);

#endif
