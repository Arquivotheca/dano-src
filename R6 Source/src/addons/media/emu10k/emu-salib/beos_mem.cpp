#include "datatype.h"

extern "C" void* osHeapAlloc(DWORD dwNumBytes, DWORD dwFlags);
extern "C" void osHeapFree(void* virtAddr);

void*
NewAlloc(DWORD dwSize)
{
  return osHeapAlloc(dwSize, 0);
}

void
DeleteAlloc(void *pMem)
{
  osHeapFree(pMem);
}
