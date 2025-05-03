#include "util/TJson.h"
#include "util/TParse.h"
#include "util/TStringConversion.h"
#include "util/TCharFmt.h"
#include "exception/TGenericException.h"
#include <cctype>
#include <cstdio>
#include <algorithm>
#include <string>
#include <iostream>

namespace BlackT {


TJson::TJson()
  : type(type_none) { }

TJson::TJson(const std::string& strVal)
  : type(type_string),
    content_string(strVal) { }

TJson::TJson(long int intVal)
  : type(type_number),
    content_number(intVal) { }

void TJson::read(TStream& ifs) {
  *this = getNextNode(ifs);
}

void TJson::write(std::ostream& ofs) const {
  outputNextNode(ofs, *this, 0);
}

TJson TJson::getNextNode(TStream& ifs) {
//  TJson result;
//  result.type = type_none;
  
  TParse::skipSpace(ifs);
  switch (ifs.peek()) {
  case '[':
    return getArray(ifs);
    break;
  case '{':
    return getObject(ifs);
    break;
  case '"':
    return getString(ifs);
    break;
  default:
    return getValue(ifs);
/*      throw TGenericException(T_SRCANDLINE,
                            "TJson::read()",
                            "Failed to parse input");*/
    break;
  }
}

TJson TJson::getArray(TStream& ifs) {
  TJson result;
  result.type = type_array;
  
/*  {
    int pos = ifs.tell();
    std::string temp;
    while (!ifs.eof()) temp += ifs.get();
    ifs.seek(pos);
    fprintf(stderr, "ARRAY: %s\n", temp.c_str());
  }*/
  
  TParse::matchChar(ifs, '[');
  
  while (!ifs.eof()) {
    TParse::skipSpace(ifs);
    if (ifs.eof()) {
      throw TGenericException(T_SRCANDLINE,
                              "TJson::getArray()",
                              "Unexpected end of input");
    }
    
    if (TParse::checkChar(ifs, ']')) break;
    
    result.content_array.push_back(getNextNode(ifs));
    
    if (!TParse::checkChar(ifs, ',')) break;
    TParse::matchChar(ifs, ',');
  }
  
  TParse::matchChar(ifs, ']');
  
/*  {
    int pos = ifs.tell();
    std::string temp;
    while (!ifs.eof()) temp += ifs.get();
    ifs.seek(pos);
    fprintf(stderr, "ARRAY DONE: %s\n", temp.c_str());
  }*/
  
  return result;
}

TJson TJson::getObject(TStream& ifs) {
  TJson result;
  result.type = type_object;
  
/*  {
    int pos = ifs.tell();
    std::string temp;
    while (!ifs.eof()) temp += ifs.get();
    ifs.seek(pos);
    fprintf(stderr, "OBJ: %s\n", temp.c_str());
  }*/
  
  TParse::matchChar(ifs, '{');
  
  while (!ifs.eof()) {
    TParse::skipSpace(ifs);
    if (ifs.eof()) {
      throw TGenericException(T_SRCANDLINE,
                              "TJson::getObject()",
                              "Unexpected end of input");
    }
    
    if (TParse::checkChar(ifs, '}')) break;
    
    std::string name = TParse::matchString(ifs);
    TParse::matchChar(ifs, ':');
    result.content_object[name] = getNextNode(ifs);
    
    if (!TParse::checkChar(ifs, ',')) break;
    TParse::matchChar(ifs, ',');
  }
  
  TParse::matchChar(ifs, '}');
  
/*  {
    int pos = ifs.tell();
    std::string temp;
    while (!ifs.eof()) temp += ifs.get();
    ifs.seek(pos);
    fprintf(stderr, "OBJ DONE: %s\n", temp.c_str());
  }*/
  
  return result;
}

TJson TJson::getString(TStream& ifs) {
  TJson result;
  result.type = type_string;
  
//  result.content_string = TParse::matchString(ifs);
  TParse::matchChar(ifs, '"');
  while (true) {
    if (ifs.eof()) {
      throw TGenericException(T_SRCANDLINE,
                              "TJson::getString()",
                              "Unexpected end of input");
    }
    
    if (ifs.peek() == '"') break;
    
    char next = ifs.get();
    if (next == '\\') {
      next = ifs.get();
      switch (next) {
      case '"':
        result.content_string += "\"";
        break;
      case '\\':
        result.content_string += "\\";
        break;
      case '/':
        result.content_string += "/";
        break;
      case 'b':
        result.content_string += "\b";
        break;
      case 'f':
        result.content_string += "\f";
        break;
      case 'n':
        result.content_string += "\n";
        break;
      case 'r':
        result.content_string += "\r";
        break;
      case 't':
        result.content_string += "\t";
        break;
      case 'u':
      {
        std::string digitStr = "0x";
        for (int i = 0; i < 4; i++) {
          digitStr += ifs.get();
        }
        unsigned int val = TStringConversion::stringToInt(digitStr);
/*        result.content_string += (char)((val & 0xFF00) >> 8);
        result.content_string += (char)((val & 0xFF));*/
        
        TUtf16Chars conv;
        conv.push_back(val);
        std::string convDst;
        TCharFmt::utf16To8(conv, convDst);
        result.content_string += convDst;
      }
        break;
      default:
        throw TGenericException(T_SRCANDLINE,
                                "TJson::getString()",
                                std::string("Unknown escaped character: ")
                                  + next);
        break;
      }
    }
    else {
      result.content_string += next;
    }
  }
  TParse::matchChar(ifs, '"');
  
  return result;
}

TJson TJson::getValue(TStream& ifs) {
  if (TParse::checkInt(ifs)) return getNumber(ifs);
  
  TJson result;
  std::string next = TParse::matchName(ifs);
  if (next.compare("true") == 0) {
    result.type = type_true;
  }
  else if (next.compare("false") == 0) {
    result.type = type_false;
  }
  else if (next.compare("null") == 0) {
    result.type = type_null;
  }
  else {
    throw TGenericException(T_SRCANDLINE,
                            "TJson::getValue()",
                            std::string("Unknown value: ")
                              + next);
  }
  return result;
}

TJson TJson::getNumber(TStream& ifs) {
  TJson result;
  result.type = type_number;
  
  result.content_number = TParse::matchInt(ifs);
  
  return result;
}

void TJson::outputNextNode(std::ostream& ofs, const TJson& obj, int indent) {
  switch (obj.type) {
  case type_array:
    outputArray(ofs, obj, indent);
    break;
  case type_object:
    outputObject(ofs, obj, indent);
    break;
  case type_string:
    outputString(ofs, obj, indent);
    break;
  case type_number:
    outputNumber(ofs, obj, indent);
    break;
  case type_null:
    ofs << "null";
    break;
  case type_true:
    ofs << "true";
    break;
  case type_false:
    ofs << "false";
    break;
  default:
    throw TGenericException(T_SRCANDLINE,
                          "TJson::outputNextNode()",
                          "Bad node type");
    break;
  }
}

void TJson::outputArray(std::ostream& ofs, const TJson& obj, int indent) {
  ofs << std::endl;
  for (int i = 0; i < indent; i++) ofs << " ";
  ofs << "[" << std::endl;
  
  for (unsigned int i = 0; i < obj.content_array.size(); i++) {
    for (int i = 0; i < indent + indentSize; i++) ofs << " ";
    
    outputNextNode(ofs, obj.content_array[i], indent + indentSize);
    
    if (i != obj.content_array.size() - 1) ofs << "," << std::endl;
  }
  
  ofs << std::endl;
  for (int i = 0; i < indent; i++) ofs << " ";
  ofs << "]";
}

void TJson::outputObject(std::ostream& ofs, const TJson& obj, int indent) {
  ofs << std::endl;
  for (int i = 0; i < indent; i++) ofs << " ";
  ofs << "{" << std::endl;
  
  unsigned int itemNum = 0;
  for (const auto& item: obj.content_object) {
    for (int i = 0; i < indent + indentSize; i++) ofs << " ";
    
    ofs << "\"" << item.first << "\"" << ": ";
    outputNextNode(ofs, item.second, indent + indentSize);
    
    ++itemNum;
    if (itemNum != obj.content_object.size()) ofs << "," << std::endl;
  }
  
  ofs << std::endl;
  for (int i = 0; i < indent; i++) ofs << " ";
  ofs << "}";
}

void TJson::outputString(std::ostream& ofs, const TJson& obj, int indent) {
//  ofs << "\"" << obj.content_string << "\"";
  ofs << "\"";
  for (const auto c: obj.content_string) {
    switch (c) {
    case '"':
      ofs << "\\\"";
      break;
    case '\\':
      ofs << "\\\\";
      break;
    case '/':
      ofs << "\\/";
      break;
    case '\b':
      ofs << "\\b";
      break;
    case '\f':
      ofs << "\\f";
      break;
    case '\n':
      ofs << "\\n";
      break;
    case '\r':
      ofs << "\\r";
      break;
    case '\t':
      ofs << "\\t";
      break;
    default:
      ofs << c;
      break;
    }
  }
  ofs << "\"";
}

void TJson::outputNumber(std::ostream& ofs, const TJson& obj, int indent) {
  ofs << obj.content_number;
}


}
