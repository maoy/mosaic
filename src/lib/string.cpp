/* $Id: string.cpp 210 2009-04-14 22:14:22Z maoy $ */

#include <vector>
#include <string>
#include<boost/tokenizer.hpp>
#include<string>

#include "types.hpp"

string_vector string_split(gc_string str)
{
  string_vector tokens;
  typedef boost::tokenizer< boost::char_separator<char>,
    gc_string::const_iterator,
    gc_string
    > gc_tokenizer;
  boost::char_separator<char> sep(" ");
  gc_tokenizer tok(str, sep);
  for (gc_tokenizer::iterator beg=tok.begin();
       beg!=tok.end();
       ++beg){
    tokens.push_back( *beg );
  }
  return tokens;
}

