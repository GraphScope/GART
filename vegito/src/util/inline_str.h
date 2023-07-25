/*
* The MIT License (MIT)

* Copyright (C) 2015 Kangnyeon Kim, Tianzheng Wang, Ryan Johnson and Ippokratis
Pandis
* Copyright (C) 2013 Stephen Tu and other contributors

* Permission is hereby granted, free of charge, to any person obtaining a copy
of
* this software and associated documentation files (the "Software"), to deal in
* the Software without restriction, including without limitation the rights to
* use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
* of the Software, and to permit persons to whom the Software is furnished to do
* so, subject to the following conditions:

* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
 */

#ifndef VEGITO_SRC_UTIL_INLINE_STR_H_
#define VEGITO_SRC_UTIL_INLINE_STR_H_

#include <cstring>
#include <string>

#include "util/serializer.h"

// equivalent to VARCHAR(N) in MySQL
template <typename IntSizeType, unsigned int N>
class inline_str_base {
  static_assert(N <= 65535, "N must be <= 65535");
  template <typename T, bool DoCompress>
  friend class serializer;

 public:
  inline_str_base() : sz(0) {}

  explicit inline_str_base(const char* s) { assign(s); }

  inline_str_base(const char* s, size_t n) { assign(s, n); }

  explicit inline_str_base(const std::string& s) { assign(s); }

  inline_str_base(const inline_str_base& that) : sz(that.sz) {
    NDB_MEMCPY(&buf[0], &that.buf[0], sz);
  }

  inline_str_base& operator=(const inline_str_base& that) {
    if (this == &that)
      return *this;
    sz = that.sz;
    NDB_MEMCPY(&buf[0], &that.buf[0], sz);
    return *this;
  }

  inline size_t max_size() const { return N; }

  inline const char* c_str() const {
    buf[sz] = 0;
    return &buf[0];
  }

  inline std::string str(bool zeropad = false) const {
    if (zeropad) {
      INVARIANT(N >= sz);
      std::string r(N, 0);
      NDB_MEMCPY((char*) r.data(), &buf[0], sz);
      return r;
    } else {
      return std::string(&buf[0], sz);
    }
  }

  inline ALWAYS_INLINE const char* data() const { return &buf[0]; }

  inline ALWAYS_INLINE size_t size() const { return sz; }

  inline ALWAYS_INLINE void assign(const char* s) { assign(s, strlen(s)); }

  inline void assign(const char* s, size_t n) {
    INVARIANT(n <= N);
    NDB_MEMCPY(&buf[0], s, n);
    sz = n;
  }

  inline ALWAYS_INLINE void assign(const std::string& s) {
    assign(s.data(), s.size());
  }

  inline ALWAYS_INLINE void assign(const std::string_view& sv) {
    assign(sv.data(), sv.size());
  }

  inline void resize(size_t n, char c = 0) {
    INVARIANT(n <= N);
    if (n > sz)
      NDB_MEMSET(&buf[sz], c, n - sz);
    sz = n;
  }

  inline void resize_junk(size_t n) {
    INVARIANT(n <= N);
    sz = n;
  }

  inline bool operator==(const inline_str_base& other) const {
    return memcmp(buf, other.buf, sz) == 0;
  }

  inline bool operator!=(const inline_str_base& other) const {
    return !operator==(other);
  }

 private:
  IntSizeType sz;
  mutable char buf[N + 1];
} PACKED_ATTR;

template <typename IntSizeType, unsigned int N>
inline std::ostream& operator<<(std::ostream& o,
                                const inline_str_base<IntSizeType, N>& s) {
  o << std::string(s.data(), s.size());
  return o;
}

template <unsigned int N>
class inline_str_8 : public inline_str_base<uint8_t, N> {
  typedef inline_str_base<uint8_t, N> super_type;

 public:
  inline_str_8() : super_type() {}
  explicit inline_str_8(const char* s) : super_type(s) {}
  inline_str_8(const char* s, size_t n) : super_type(s, n) {}
  explicit inline_str_8(const std::string& s) : super_type(s) {}
} PACKED_ATTR;

template <unsigned int N>
class inline_str_16 : public inline_str_base<uint16_t, N> {
  typedef inline_str_base<uint16_t, N> super_type;

 public:
  inline_str_16() : super_type() {}
  explicit inline_str_16(const char* s) : super_type(s) {}
  inline_str_16(const char* s, size_t n) : super_type(s, n) {}
  explicit inline_str_16(const std::string& s) : super_type(s) {}
} PACKED_ATTR;

// equiavlent to CHAR(N)
template <unsigned int N, char FillChar = ' '>
class inline_str_fixed {
  // XXX: argh...
  template <typename T, bool DoCompress>
  friend class serializer;

 public:
  inline_str_fixed() { NDB_MEMSET(&buf[0], FillChar, N); }

  explicit inline_str_fixed(const char* s) {
    // NDB_MEMSET(&buf[0],0, N);
    assign(s, strlen(s));
  }

  inline_str_fixed(const char* s, size_t n) { assign(s, n); }

  explicit inline_str_fixed(const std::string& s) {
    //    fprintf(stdout,"assign base\n");
#if 0
    if (s.size() + 1 > N) {
      fprintf(stdout, " sizeof s %d , check %s\n", s.size(), s.c_str());
      assert(false);
    }
#endif
    assert(s.size() + 1 <= N);
    assign(s.data(), s.size() + 1);
  }

  inline_str_fixed(const inline_str_fixed& that) {
    NDB_MEMCPY(&buf[0], &that.buf[0], N);
  }

  inline_str_fixed& operator=(const inline_str_fixed& that) {
    if (this == &that)
      return *this;
    NDB_MEMCPY(&buf[0], &that.buf[0], N);
    return *this;
  }

  inline ALWAYS_INLINE std::string str() const {
    return std::string(&buf[0], N);
  }

  inline ALWAYS_INLINE const char* data() const { return &buf[0]; }

  inline ALWAYS_INLINE size_t size() const { return N; }

  inline ALWAYS_INLINE void assign(const char* s) { assign(s, strlen(s)); }

  inline void assign(const char* s, size_t n) {
    INVARIANT(n <= N);
    NDB_MEMCPY(&buf[0], s, n);
    if ((N - n) > 0)                         // to suppress compiler warning
      NDB_MEMSET(&buf[n], FillChar, N - n);  // pad with spaces
  }

  inline ALWAYS_INLINE void assign(const std::string& s) {
    assign(s.data(), s.size());
  }

  inline ALWAYS_INLINE void assign(const std::string_view& sv) {
    assign(sv.data(), sv.size());
  }

  inline bool operator==(const inline_str_fixed& other) const {
    return memcmp(buf, other.buf, N) == 0;
  }

  inline bool operator!=(const inline_str_fixed& other) const {
    return !operator==(other);
  }

 private:
  char buf[N];
} PACKED_ATTR;

template <unsigned int N, char FillChar>
inline std::ostream& operator<<(std::ostream& o,
                                const inline_str_fixed<N, FillChar>& s) {
  o << std::string(s.data(), s.size());
  return o;
}

// serializer<T> specialization
template <typename IntSizeType, unsigned int N, bool Compress>
struct serializer<inline_str_base<IntSizeType, N>, Compress> {
  typedef inline_str_base<IntSizeType, N> obj_type;
  static inline uint8_t* write(uint8_t* buf, const obj_type& obj) {
    buf = serializer<IntSizeType, Compress>::write(buf, &obj.sz);
    NDB_MEMCPY(buf, &obj.buf[0], obj.sz);
    return buf + obj.sz;
  }

  static const uint8_t* read(const uint8_t* buf, obj_type* obj) {
    buf = serializer<IntSizeType, Compress>::read(buf, &obj->sz);
    NDB_MEMCPY(&obj->buf[0], buf, obj->sz);
    return buf + obj->sz;
  }

  static const uint8_t* failsafe_read(const uint8_t* buf, size_t nbytes,
                                      obj_type* obj) {
    const uint8_t* const hdrbuf =
        serializer<IntSizeType, Compress>::failsafe_read(buf, nbytes, &obj->sz);
    if (unlikely(!hdrbuf))
      return nullptr;
    nbytes -= (hdrbuf - buf);
    if (nbytes < obj->sz)
      return nullptr;
    buf = hdrbuf;
    NDB_MEMCPY(&obj->buf[0], buf, obj->sz);
    return buf + obj->sz;
  }

  static inline size_t nbytes(const obj_type* obj) {
    return serializer<IntSizeType, Compress>::nbytes(&obj->sz) + obj->sz;
  }

  static inline size_t skip(const uint8_t* stream, uint8_t* oldv) {
    IntSizeType sz = 0;
    const uint8_t* const body =
        serializer<IntSizeType, Compress>::read(stream, &sz);
    const size_t totalsz = (body - stream) + sz;
    if (oldv)
      NDB_MEMCPY(oldv, stream, totalsz);
    return totalsz;
  }

  static inline size_t failsafe_skip(const uint8_t* stream, size_t nbytes,
                                     uint8_t* oldv) {
    IntSizeType sz = 0;
    const uint8_t* const body =
        serializer<IntSizeType, Compress>::failsafe_read(stream, nbytes, &sz);
    if (unlikely(!body))
      return 0;
    nbytes -= (body - stream);
    if (unlikely(nbytes < sz))
      return 0;
    const size_t totalsz = (body - stream) + sz;
    if (oldv)
      NDB_MEMCPY(oldv, stream, totalsz);
    return totalsz;
  }

  static inline constexpr size_t max_nbytes() {
    return serializer<IntSizeType, Compress>::max_bytes() + N;
  }
};

#endif  // VEGITO_SRC_UTIL_INLINE_STR_H_
