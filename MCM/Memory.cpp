#include<cstdlib>
#include<cstring>
#include "Memory.hpp"
#include "Util.hpp"
#define USE_MALLOC 1
#ifdef WIN32
#include<Windows.h>
#else
#endif
MemMap::MemMap() :storage(nullptr), size(0){
}
MemMap::~MemMap(){
  release();
}
void MemMap::resize(size_t bytes){
  if (bytes == size){
    std::fill(reinterpret_cast<uint8_t*>(storage), reinterpret_cast<uint8_t*>(storage)+size, 0);
    return;
  }
  release();
  size = bytes;
#if USE_MALLOC
  storage = std::calloc(1, bytes);
#elif WIN32
  storage = (void*)VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
#error UNIMPLEMENTED
#endif
}
void MemMap::release(){
  if (storage != nullptr){
#if USE_MALLOC
    std::free(storage);
#elif WIN32
    BOOL result = VirtualFree((LPVOID)storage, size, MEM_DECOMMIT);
#else
#error UNIMPLEMENTED
#endif
    storage = nullptr;
  }
}
void MemMap::zero(){
#ifdef USE_MALLOC
  std::memset(storage, 0, size);
#elif WIN32
  storage = (void*)VirtualAlloc(storage, size, MEM_RESET, PAGE_READWRITE);
#else
  madvise(storage, size, MADV_DONTNEED);
#endif
}
