#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include "swap_space.hpp"
#include <iostream>
#include <string>
#include <map>
#include <cassert>

class swap_space;

class serialization_context {
  public:
  serialization_context(swap_space& sspace)
      : ss(sspace)
  {
  }
  swap_space& ss;
};

class serializable {
  public:
  virtual void _serialize(std::iostream& fs, serialization_context& context) = 0;
  virtual void _deserialize(std::iostream& fs, serialization_context& context) = 0;
  virtual ~serializable(void) {};
};

void serialize(std::iostream& fs, serialization_context& context, uint64_t x);
void deserialize(std::iostream& fs, serialization_context& context, uint64_t& x);

void serialize(std::iostream& fs, serialization_context& context, int64_t x);
void deserialize(std::iostream& fs, serialization_context& context, int64_t& x);

void serialize(std::iostream& fs, serialization_context& context, std::string x);
void deserialize(std::iostream& fs, serialization_context& context, std::string& x);

template <class Key, class Value>
void serialize(std::iostream& fs,
    serialization_context& context,
    std::map<Key, Value>& mp)
{
  fs << "map " << mp.size() << " {" << std::endl;
  assert(fs.good());
  for (auto it = mp.begin(); it != mp.end(); ++it) {
    fs << "  ";
    serialize(fs, context, it->first);
    fs << " -> ";
    serialize(fs, context, it->second);
    fs << std::endl;
  }
  fs << "}" << std::endl;
}

template <class Key, class Value>
void deserialize(std::iostream& fs,
    serialization_context& context,
    std::map<Key, Value>& mp)
{
  std::string dummy;
  int size = 0;
  fs >> dummy >> size >> dummy;
  assert(fs.good());
  for (int i = 0; i < size; i++) {
    Key k;
    Value v;
    deserialize(fs, context, k);
    fs >> dummy;
    deserialize(fs, context, v);
    mp[k] = v;
  }
  fs >> dummy;
}

template <class X>
void serialize(std::iostream& fs, serialization_context& context, X*& x)
{
  fs << "pointer ";
  serialize(fs, context, *x);
}

template <class X>
void deserialize(std::iostream& fs, serialization_context& context, X*& x)
{
  std::string dummy;
  x = new X;
  fs >> dummy;
  assert(dummy == "pointer");
  deserialize(fs, context, *x);
}

template <class X>
void serialize(std::iostream& fs, serialization_context& context, X& x)
{
  x._serialize(fs, context);
}

template <class X>
void deserialize(std::iostream& fs, serialization_context& context, X& x)
{
  x._deserialize(fs, context);
}

//Methods to serialize/deserialize different kinds of objects.
//You shouldn't need to touch these.
void serialize(std::iostream &fs, serialization_context &context, uint64_t x)
{
  fs << x << " ";
  assert(fs.good());
}

void deserialize(std::iostream &fs, serialization_context &context, uint64_t &x)
{
  fs >> x;
  assert(fs.good());
}

void serialize(std::iostream &fs, serialization_context &context, int64_t x)
{
  fs << x << " ";
  assert(fs.good());
}

void deserialize(std::iostream &fs, serialization_context &context, int64_t &x)
{
  fs >> x;
  assert(fs.good());
}

void serialize(std::iostream &fs, serialization_context &context, std::string x)
{
  fs << x.size() << ",";
  assert(fs.good());
  fs.write(x.data(), x.size());
  assert(fs.good());
}

void deserialize(std::iostream &fs, serialization_context &context, std::string &x)
{
  size_t length;
  char comma;
  fs >> length >> comma;
  assert(fs.good());
  char *buf = new char[length];
  assert(buf);
  fs.read(buf, length);
  assert(fs.good());
  x = std::string(buf, length);
  delete buf;
}


#endif
