#include "Huffman.hpp"
uint32_t HuffmanStatic::compressBytes(byte*in, byte*out, uint32_t count){
  size_t freq[256] = { 0 };
  uint32_t length = 0;
  for (uint32_t i = 0; i < count; ++i){
    ++freq[in[i]];
  }
  Huffman huff;
  auto*tree = huff.buildTreePackageMerge(&freq[0], kAlphabetSize, kCodeBits);
#if 0
  ent.init();
  uint32_t lengths[kAlphabetSize] = { 0 };
  tree->getLengths(&lengths[0]);
  std::cout << "Encoded huffmann tree in ~" << sout.getTotal() << " bytes" << std::endl;
  ent.EncodeBits(sout, length, 31);
  std::cout << std::endl;
  for (;;){
    int c = sin.read();
    if (c == EOF)break;
    const auto&huff_code = getCode(c);
    ent.EncodeBits(sout, huff_code.value, huff_code.length);
    meter.addBytePrint(sout.getTotal());
  }
  std::cout << std::endl;
  ent.flush(sout);
  return sout.getTotal();
#endif
  return 0;
}
void HuffmanStatic::decompressBytes(byte*in, byte*out, uint32_t count){
}
