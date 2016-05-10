#ifndef STREAM_HPP_
#define STREAM_HPP_

#include <cstring>
#include "Util.hpp"

class Tellable {
public:
  virtual uint64_t tell()const{
    unimplementedError(__FUNCTION__);
    return 0;
  }
};

class Stream : public Tellable {
public:
  virtual int get() = 0;
  virtual void put(int c) = 0;
  virtual size_t read(uint8_t *buf, size_t n) {
    size_t count;
    for (count = 0; count < n; ++count) {
      auto c = get();
      if (c == EOF) {
        break;
      }
      buf[count] = c;
    }
    return count;
  }
  virtual size_t readat(uint64_t pos, uint8_t *buf, size_t n) {
    seek(pos);
    return read(buf, n);
  }
  virtual void write(const uint8_t *buf, size_t n) {
    for (; n; --n) {
      put(*(buf++));
    }
  }
  virtual void writeat(uint64_t pos, const uint8_t *buf, size_t n) {
    seek(pos);
    write(buf, n);
  }
  virtual void seek(uint64_t pos) {
    unimplementedError(__FUNCTION__);
  }
  virtual ~Stream() {
  }
  void put16(uint16_t n) {
    put(static_cast<uint8_t>(n >> 8));
    put(static_cast<uint8_t>(n >> 0));
  }
  uint16_t get16() {
    uint16_t ret = 0;
    ret = (ret << 8) | static_cast<uint16_t>(get());
    ret = (ret << 8) | static_cast<uint16_t>(get());
    return ret;
  }
#if 0
  inline void leb128Encode(int64_t n) {
    bool neg = n < 0;
    if (neg)n = -n;
    leb128Encode(static_cast<uint64_t>((n << 1) | (neg ? 1u : 0)));
  }
#endif
  inline void leb128Encode(uint64_t n) {
    while (n >= 0x80) {
      auto c = static_cast<uint8_t>(0x80 | (n & 0x7F));
      put(c);
      n >>= 7;
    }
    put(static_cast<uint8_t>(n));
  }
  uint64_t leb128Decode() {
    uint64_t ret = 0;
    uint64_t shift = 0;
    while (true) {
      const uint8_t c = get();
      ret |= static_cast<uint64_t>(c & 0x7F) << shift;
      shift += 7;
      if ((c & 0x80) == 0)break;
    }
    return ret;
  }
  void writeString(const char*str) {
    do{
      put(*str);
    } while (*(str++) != '\0');
  }
  std::string readString() {
    std::string s;
    for (;;) {
      int c = get();
      if (c == EOF || c == '\0')break;
      s.push_back(static_cast<char>(c));
    }
    return s;
  }
};

class OutputStream : public Stream {
public:
  virtual int get() {
    unimplementedError(__FUNCTION__);
    return 0;
  }
  virtual void put(int c) = 0;
  virtual ~OutputStream() {}
};

class InputStream : public Stream {
public:
  virtual int get() = 0;
  virtual void put(int c) {
    unimplementedError(__FUNCTION__);
  }
  virtual ~InputStream() {}
};

class ReadMemoryStream : public InputStream {
public:
  ReadMemoryStream(const std::vector<byte>*buffer)
    :buffer_(buffer->data())
    , pos_(buffer->data())
    , limit_(buffer->data() + buffer->size()) {
  }
  ReadMemoryStream(const byte *buffer, const byte *limit) :buffer_(buffer), pos_(buffer), limit_(limit) {
  }
  virtual int get() {
    if (pos_ >= limit_) {
      return EOF;
    }
    return *pos_++;
  }
  virtual size_t read(byte *buf, size_t n) {
    const size_t remain = limit_ - pos_;
    const size_t read_count = std::min(remain, n);
    std::copy(pos_, pos_ + read_count, buf);
    pos_ += read_count;
    return read_count;
  }
  virtual uint64_t tell() const {
    return pos_ - buffer_;
  }
  virtual void seek(uint64_t pos) {
    pos_ = buffer_ + pos;
  }
private:
  const byte *const buffer_;
  const byte *pos_;
  const byte *const limit_;
};

class WriteMemoryStream : public OutputStream {
public:
  explicit WriteMemoryStream(byte *buffer) : buffer_(buffer), pos_(buffer) {
  }
  virtual void put(int c) {
    *pos_++ = static_cast<byte>(static_cast<unsigned int>(c));
  }
  virtual void write(const byte *data, uint32_t count) {
    memcpy(pos_, data, count);
    pos_ += count;
  }
  virtual uint64_t tell() const {
    return pos_ - buffer_;
  }
private:
  byte *buffer_;
  byte *pos_;
};

class WriteVectorStream : public OutputStream {
public:
  explicit WriteVectorStream(std::vector<byte> *buffer) : buffer_(buffer), pos_(-1) {
  }
  virtual void put(int c) {
    if (pos_ == -1) pos_ = buffer_->size();
    if (pos_ < buffer_->size())
      (*buffer_)[pos_] = (byte)c;
    else
      buffer_->push_back(c);
    pos_++;
  }
  virtual void write(const byte *data, uint32_t count) {
    if (pos_ == -1) pos_ = buffer_->size();
    unsigned i = 0;
    for (; i < count && pos_ < buffer_->size(); i++, pos_++) {
      (*buffer_)[pos_] = data[i];
    }
    if (i < count) {
      buffer_->insert(buffer_->end(), data + i, data + count - i);
      pos_ = buffer_->size();
    }
  }
  virtual uint64_t tell() const {
    if (pos_ == -1) return buffer_->size();
    return pos_;
  }
  virtual void seek(uint64_t pos) {
    if (pos <= buffer_->size())
      pos_ = pos;
    else {
      buffer_->resize(pos);
      pos_ = pos;
    }
  }
private:
  std::vector<byte> *const buffer_;
  uint64_t pos_;
};

template<const uint32_t buffer_size>
class BufferedStreamReader {
public:
  Stream *stream;
  size_t buffer_count, buffer_pos;
  byte buffer[buffer_size];
  BufferedStreamReader(Stream *stream) {
    assert(stream != nullptr);
    init(stream);
  }
  virtual ~BufferedStreamReader() {
  }
  void init(Stream *new_stream) {
    stream = new_stream;
    buffer_pos = 0;
    buffer_count = 0;
  }
  forceinline size_t remain() const {
    return buffer_count - buffer_pos;
  }
  forceinline int get() {
    if (UNLIKELY(remain() == 0)) {
      buffer_pos = 0;
      buffer_count = stream->read(buffer, buffer_size);
      if (UNLIKELY(buffer_count == 0)) {
        return EOF;
      }
    }
    return buffer[buffer_pos++];
  }
  void put(int c) {
    unimplementedError(__FUNCTION__);
  }
  uint64_t tell() const {
    return stream->tell() + buffer_pos;
  }
};

template<const uint32_t kBufferSize>
class BufferedStreamWriter {
public:
  BufferedStreamWriter(Stream *stream) {
    assert(stream != nullptr);
    init(stream);
  }
  virtual ~BufferedStreamWriter() {
    flush();
    assert(ptr_ == buffer_);
  }
  void init(Stream *new_stream) {
    stream_ = new_stream;
    ptr_ = buffer_;
  }
  void flush() {
    stream_->write(buffer_, ptr_ - buffer_);
    ptr_ = buffer_;
  }
  forceinline void put(uint8_t c) {
    if (UNLIKELY(ptr_ >= end())) {
      flush();
    }
    *(ptr_++) = c;
  }
  int get() {
    unimplementedError(__FUNCTION__);
    return 0;
  }
  uint64_t tell() const {
    return stream_->tell() + (ptr_ - buffer_);
  }
private:
  const uint8_t *end() const {
    return &buffer_[kBufferSize];
  }
  Stream *stream_;
  uint8_t buffer_[kBufferSize];
  uint8_t *ptr_;
};

template<const bool kLazy = true>
class MemoryBitStream {
  byte *no_alias data_;
  uint32_t buffer_;
  uint32_t bits_;
  static const uint32_t kBitsPerSizeT = sizeof(uint32_t)*kBitsPerByte;
public:
  forceinline MemoryBitStream(byte *data) :data_(data), buffer_(0), bits_(0) {
  }
  byte *getData() {
    return data_;
  }
  forceinline void tryReadByte() {
    if (bits_ <= kBitsPerSizeT - kBitsPerByte) {
      readByte();
    }
  }
  forceinline void readByte() {
    buffer_ = (buffer_ << kBitsPerByte) | *data_++;
    bits_ += kBitsPerByte;
  }
  forceinline uint32_t readBits(uint32_t bits) {
    if (kLazy) {
      while (bits_ < bits) {
        readByte();
      }
    }
    else{
      tryReadByte();
      tryReadByte();
      tryReadByte();
    }
    bits_ -= bits;
    uint32_t ret = buffer_ >> bits_;
    buffer_ -= ret << bits_;
    return ret;
  }
  forceinline void flushByte() {
    bits_ -= kBitsPerByte;
    uint32_t byte = buffer_ >> bits_;
    buffer_ -= byte << bits_;
    *data_++ = byte;
  }
  void flush() {
    while (bits_ > kBitsPerByte) {
      flushByte();
    }
    *data_++ = buffer_;
  }
  forceinline void writeBits(uint32_t data, uint32_t bits) {
    bits_ += bits;
    buffer_ = (buffer_ << bits) | data;
    while (bits_ >= kBitsPerByte) {
      flushByte();
    }
  }
};

#endif
