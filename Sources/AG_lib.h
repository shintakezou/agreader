#ifndef AGLIB_H
#define AGLIB_H

#ifdef DEBUG_MEM /* Track memory deallocation */
void* AllocMem(size_t size);
void FreeMem(void* mem);

#define free(X) FreeMem(X)
#define malloc(X) AllocMem(X)
#endif // DEBUG_MEM

#endif // AGLIB_H
