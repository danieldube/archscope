#include "beta.cpp"

struct Derived : Beta {};

struct Alpha {
  Beta member;
  Beta make(Beta value);
};
