#ifndef TJSON_H
#define TJSON_H


#include "util/TStream.h"
#include <string>
#include <map>
#include <vector>
#include <iostream>

namespace BlackT {

// this is all very lazy and doesn't support anything beyond what i immediately
// need. get a real library if you want real functionality.
// in particular, all numbers must be plain integers (and hex prefixes will be
// interpreted, which violates the standard).
// utf8 conversion is attempted for \u sequences, but i'm not sure if it'll
// actually work for everything.

class TJson {
public:
  
  enum Type {
    type_none,
    type_string,
    type_number,
    type_array,
    type_object,
    type_null,
    type_true,
    type_false
  };
  
  Type type;
  
  TJson();
  TJson(const std::string& strVal);
  TJson(long int intVal);
  
  std::string content_string;
  long int content_number;
  std::vector<TJson> content_array;
  std::map<std::string, TJson> content_object;
  
  void read(TStream& ifs);
  void write(std::ostream& ofs) const;
  
protected:
  static TJson getNextNode(TStream& ifs);
  static TJson getArray(TStream& ifs);
  static TJson getObject(TStream& ifs);
  static TJson getString(TStream& ifs);
  static TJson getValue(TStream& ifs);
  static TJson getNumber(TStream& ifs);
  
  static void outputNextNode(std::ostream& ofs, const TJson& obj, int indent = 0);
  static void outputArray(std::ostream& ofs, const TJson& obj, int indent = 0);
  static void outputObject(std::ostream& ofs, const TJson& obj, int indent = 0);
  static void outputString(std::ostream& ofs, const TJson& obj, int indent = 0);
  static void outputValue(std::ostream& ofs, const TJson& obj, int indent = 0);
  static void outputNumber(std::ostream& ofs, const TJson& obj, int indent = 0);
  
  const static int indentSize = 2;
};

}


#endif
