#pragma once

#include <boost/iterator/iterator_facade.hpp>

template<typename S>
struct cast_iterator :
public boost::iterator_facade<
cast_iterator<S>,S,std::random_access_iterator_tag> {
  const size_t stride;
  char* pointer;
  template<typename T> cast_iterator(T it) :
    stride(sizeof(typename std::iterator_traits<T>::value_type)),
    pointer(const_cast<char*>(reinterpret_cast<const char*>((&(*it))))) {}
  S& dereference() const { return *reinterpret_cast<S*>(pointer); }
  bool equal(const cast_iterator<S>& other) const {
    return pointer==other.pointer;
  }
  void increment() { pointer+=stride; }
  void decrement() { pointer-=stride; }
  void advance(ptrdiff_t n) { pointer+=(n*stride); }
  ptrdiff_t distance_to(const cast_iterator<S>& other) const {
    return (other.pointer-pointer)/stride;
  }
};
