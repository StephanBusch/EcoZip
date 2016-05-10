#ifndef _DIV_TABLE_HPP_
#define _DIV_TABLE_HPP_

template<typename T, const uint32_t shift_, const uint32_t size_>
class DivTable {
  T data[size_];
public:
  DivTable(){
  }
  void init(){
    for (uint32_t i = 0; i < size_; ++i) {
      data[i] = int(1 << shift_) / int(i * 16 + 1);
    }
  }
  forceinline uint32_t size() const {
    return size_;
  }
  forceinline T&operator[](T i) {
    return data[i];
  }
  forceinline T operator[](T i) const {
    return data[i];
  }
};

#endif
