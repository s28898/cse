#include <iostream>
#include "AbstractTest.h"

#include <ostream>

// #define DOCTEST_CONFIG_IMPLEMENT

int main()
{
  OpeSimpleTest::run();
  std::cout << "Hello, World!" << std::endl;
  return 0;
}
