#ifndef CLASS_LUAVECTOR_H
#define CLASS_LUAVECTOR_H

#include <vector>

template <class T>
class LuaVector {
  typedef const std::vector<T> stltype;
  // typedef stltype::size_type size_type;
  typedef size_t size_type;
public:
  LuaVector(const stltype& data) : m_crdata(&data) {}
  size_type getSize() const { return m_crdata->size(); }
  T getAt(size_type index) const { return m_crdata->operator[](index); }
private:
  const stltype * m_crdata;
};

#endif
