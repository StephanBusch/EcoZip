#ifndef _MEMORY_HPP_
#define _MEMORY_HPP_
#include "Util.hpp"
class MemMap{
  void*storage;
  size_t size;
public:
  inline size_t getSize()const{
    return size;
  }
  void resize(size_t bytes);
  void release();
  void zero();
  inline const void*getData()const{
    return storage;
  }
  inline void*getData(){
    return storage;
  }
  MemMap();
  virtual~MemMap();
};
template<typename T, bool kBigEndian>
T readBytes(byte*ptr, size_t bytes){
  T acc = 0;
  if (!kBigEndian){
    ptr += bytes;
  }
  for (size_t i = 0; i < bytes; ++i){
    acc = (acc << 8) | (kBigEndian ? *ptr++ : *--ptr);
  }
  return acc;
}
template<typename T, bool kBigEndian>
void writeBytes(byte*ptr, size_t bytes, T value){
  T acc = 0;
  if (kBigEndian){
    ptr += bytes;
  }
  for (size_t i = 0; i < bytes; ++i){
    *(kBigEndian ? --ptr : ptr++) = value;
    value >>= 8;
  }
}
#endif
