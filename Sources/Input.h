#ifndef INPUT_H
#define INPUT_H

#include "AGNodes.h"

void Prompt(char* str);
void SetTopLine(AGNode node);
void ThrowError(char* msg, char* param);
void ProcessKeys(void);

#endif
