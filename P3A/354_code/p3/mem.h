#ifndef __mem_h__
#define __mem_h__

#define P_BESTFIT  (0)
#define P_WORSTFIT (1)
#define P_FIRSTFIT (2)

int Mem_Init(int sizeOfRegion, int policy);
void *Mem_Alloc(int size);
int Mem_Free(void *ptr);
void Mem_Dump();


#endif // __mem_h__


