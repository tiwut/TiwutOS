#pragma once
#include "../api/types.h"

extern "C" void *malloc(int size) {
  static uint8_t heap[1024 * 1024 * 64];
  static int offset = 0;
  void *ptr = &heap[offset];
  offset += size;
  return ptr;
}
extern "C" void free(void *ptr) { (void)ptr; }
extern "C" {
void *memset(void *dest, int val, int len) {
  unsigned char *ptr = (unsigned char *)dest;
  while (len-- > 0)
    *ptr++ = val;
  return dest;
}
void *memcpy(void *dest, const void *src, int len) {
  char *d = (char *)dest;
  const char *s = (const char *)src;
  while (len--)
    *d++ = *s++;
  return dest;
}

void rep_movsd(uint32_t *dest, const uint32_t *src, uint32_t count) {
  asm volatile("rep movsl" : "+D"(dest), "+S"(src), "+c"(count) : : "memory");
}

int _fltused = 0;
}
class String {
public:
  char *data;
  int len;
  String() {
    data = (char *)malloc(1);
    data[0] = 0;
    len = 0;
  }
  String(const char *s) {
    len = 0;
    while (s[len])
      len++;
    data = (char *)malloc(len + 1);
    for (int i = 0; i < len; i++)
      data[i] = s[i];
    data[len] = 0;
  }
  String(const String &other) {
    len = other.len;
    data = (char *)malloc(len + 1);
    for (int i = 0; i <= len; i++)
      data[i] = other.data[i];
  }
  String &operator=(const String &other) {
    len = other.len;
    data = (char *)malloc(len + 1);
    for (int i = 0; i <= len; i++)
      data[i] = other.data[i];
    return *this;
  }
  bool operator==(const String &other) const {
    if (len != other.len)
      return false;
    for (int i = 0; i < len; i++)
      if (data[i] != other.data[i])
        return false;
    return true;
  }
  bool operator==(const char *other) const {
    int olen = 0;
    while (other[olen])
      olen++;
    if (len != olen)
      return false;
    for (int i = 0; i < len; i++)
      if (data[i] != other[i])
        return false;
    return true;
  }
  String operator+(const String &other) const {
    String res;
    res.len = len + other.len;
    res.data = (char *)malloc(res.len + 1);
    for (int i = 0; i < len; i++)
      res.data[i] = data[i];
    for (int i = 0; i < other.len; i++)
      res.data[len + i] = other.data[i];
    res.data[res.len] = 0;
    return res;
  }
  String operator+(const char *other) const { return *this + String(other); }
  String substring(int start, int length) {
    String res;
    res.len = length;
    res.data = (char *)malloc(length + 1);
    for (int i = 0; i < length; i++)
      res.data[i] = data[start + i];
    res.data[length] = 0;
    return res;
  }
};

template <typename T> class Vector {
public:
  T *data;
  int capacity;
  int count;
  Vector() {
    capacity = 16;
    count = 0;
    data = (T *)malloc(sizeof(T) * capacity);
  }
  void init() {
    capacity = 16;
    count = 0;
    data = (T *)malloc(sizeof(T) * capacity);
  }
  void push_back(const T &val) {
    if (count >= capacity) {
      capacity = capacity == 0 ? 16 : capacity * 2;
      T *newdata = (T *)malloc(sizeof(T) * capacity);
      for (int i = 0; i < count; i++)
        newdata[i] = data[i];
      data = newdata;
    }
    data[count++] = val;
  }
  T &operator[](int index) { return data[index]; }
  int size() { return count; }
};

template <typename K, typename V> class Map {
public:
  struct Node {
    K key;
    V value;
  };
  Vector<Node> nodes;
  void init() { nodes.init(); }
  void set(K key, V value) {
    for (int i = 0; i < nodes.size(); i++) {
      if (nodes[i].key == key) {
        nodes[i].value = value;
        return;
      }
    }
    Node n;
    n.key = key;
    n.value = value;
    nodes.push_back(n);
  }
  V get(K key) {
    for (int i = 0; i < nodes.size(); i++) {
      if (nodes[i].key == key)
        return nodes[i].value;
    }
    return V();
  }
  bool contains(K key) {
    for (int i = 0; i < nodes.size(); i++) {
      if (nodes[i].key == key)
        return true;
    }
    return false;
  }
};
