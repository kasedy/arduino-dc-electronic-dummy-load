#ifndef __STD_HEADER_INITIALIZER_LIST
#define __STD_HEADER_INITIALIZER_LIST

#pragma GCC visibility push(default)

namespace std { 

template<class T> 
class initializer_list {
  
private:
  const T* array; 
  size_t len; 

  // Initialize from a { ... } construct
  initializer_list(const T *a, size_t l): array(a), len(l) { }

public:
  
  // default constructor
  initializer_list() : array(NULL), len(0) {}

  size_t size() const {
    return len;
  }

  const T *begin() {
    return array; 
  }

  const T *end() {
    return array + len;
  }

};

}

#endif
