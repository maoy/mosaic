/* $Id: csv.hpp 211 2009-04-14 22:16:01Z maoy $ */

#ifndef __CSV_HPP__
#define __CSV_HPP__

#include <fstream>
#include <iostream>
#include "types.hpp"

class CSVLoader {
private:
  const char* fn_;
  std::ifstream fs_;
public:
  CSVLoader() {}
  bool open(const char* fn);
  bool eof();
  string_vector next();
  void close();
};

#endif
