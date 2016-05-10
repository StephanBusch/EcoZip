#ifndef _UTF_8_HPP_
#define _UTF_8_HPP_
#include "Util.hpp"
template<bool error_check = false>
class UTF8Decoder{
  uint32_t extra;
  uint32_t acc;
  bool error;
public:
  UTF8Decoder(){
    init();
    error = false;
  }
  void init(){
    extra = 0;
    acc = 0;
    error = false;
  }
  forceinline bool done()const{
    return!extra;
  }
  forceinline uint32_t getAcc()const{
    return acc;
  }
  forceinline bool err()const{
    return error;
  }
  forceinline void clear_err()const{
    error = false;
  }
  forceinline void update(uint32_t c){
    if (extra){
      acc = (acc << 6) | (c & 0x3F);
      --extra;
      if (error_check && (c >> 6) != 2){
        error = true;
      }
    }
    else if (!(c & 0x80)){
      acc = c & 0x7F;
    }
    else if ((c&(0xFF << 1)) == ((0xFE << 1) & 0xFF)){
      acc = c & 0x1;
      extra = 5;
    }
    else if ((c&(0xFF << 2)) == ((0xFE << 2) & 0xFF)){
      acc = c & 0x3;
      extra = 4;
    }
    else if ((c&(0xFF << 3)) == ((0xFE << 3) & 0xFF)){
      acc = c & 0x7;
      extra = 3;
    }
    else if ((c&(0xFF << 4)) == ((0xFE << 4) & 0xFF)){
      acc = c & 0xF;
      extra = 2;
    }
    else if ((c&(0xFF << 5)) == ((0xFE << 5) & 0xFF)){
      acc = c & 0x1F;
      extra = 1;
    }
    else{
      acc = c;
      if (error_check){
        error = true;
      }
    }
  }
};
#endif
