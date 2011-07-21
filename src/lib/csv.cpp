/* $Id: csv.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <vector>
#include <string>
#include <boost/tokenizer.hpp>
#include <string>

#include "csv.hpp"

string_vector csv_parser(const gc_string line){
  string_vector tokens;
  typedef boost::tokenizer< boost::escaped_list_separator<char>,
    gc_string::const_iterator,
    gc_string > gc_tokenizer;
  gc_tokenizer tok(line);
  for (gc_tokenizer::iterator beg=tok.begin();
       beg!=tok.end();
       ++beg){
    tokens.push_back( *beg );
  }
  return tokens;
}

bool CSVLoader::open(const char* fn){
  fn_ = fn;
  fs_.open(fn_, std::ios::in);
  return fs_.is_open();
}


bool CSVLoader::eof(){
  assert(fs_.is_open());
  return fs_.eof();
}

string_vector CSVLoader::next(){
  if (eof())
    throw std::runtime_error("end of the csv file");

  gc_string line;
  std::getline(fs_,line);
  return csv_parser(line);
}

void CSVLoader::close(){
  fs_.close();
}
