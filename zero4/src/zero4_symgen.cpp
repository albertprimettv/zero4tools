#include "zero4/Zero4MsgConsts.h"
#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TGraphic.h"
#include "util/TStringConversion.h"
#include "util/TFileManip.h"
#include "util/TPngConversion.h"
#include "util/TFreeSpace.h"
#include "util/TBitmapFont.h"
#include <cctype>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

//#define BUILD_COMPRESSION_DICTIONARY 1

using namespace std;
using namespace BlackT;
using namespace Pce;

//TThingyTable tableStd;

string as2bHex(int num) {
  string str = TStringConversion::intToString(num,
                  TStringConversion::baseHex).substr(2, string::npos);
  while (str.size() < 2) str = string("0") + str;
  
//  return "<$" + str + ">";
  return str;
}

string as2bHexPrefix(int num) {
  return "$" + as2bHex(num) + "";
}

string asHexPrefix(int num) {
  string str = TStringConversion::intToString(num,
                  TStringConversion::baseHex).substr(2, string::npos);
  return "$" + str + "";
}

std::string getNumStr(int num) {
  std::string str = TStringConversion::intToString(num);
  while (str.size() < 2) str = string("0") + str;
  return str;
}

std::string getHexByteNumStr(int num) {
  std::string str = TStringConversion::intToString(num,
    TStringConversion::baseHex).substr(2, string::npos);
  while (str.size() < 2) str = string("0") + str;
  return string("$") + str;
}

std::string getHexWordNumStr(int num) {
  std::string str = TStringConversion::intToString(num,
    TStringConversion::baseHex).substr(2, string::npos);
  while (str.size() < 4) str = string("0") + str;
  return string("$") + str;
}
                      

void binToDcb(TStream& ifs, std::ostream& ofs) {
  int constsPerLine = 16;
  
  while (true) {
    if (ifs.eof()) break;
    
    ofs << "  .db ";
    
    for (int i = 0; i < constsPerLine; i++) {
      if (ifs.eof()) break;
      
      TByte next = ifs.get();
      ofs << as2bHexPrefix(next);
      if (!ifs.eof() && (i != constsPerLine - 1)) ofs << ",";
    }
    
    ofs << std::endl;
  }
}

void outputPrefixedStringInclude(std::ostream& includeOfs,
    std::string prefix, std::string name, std::string value) {
  includeOfs << ".define " << prefix << name
    << " \"" << value << "\""
    << std::endl;
}

void outputPrefixedNumInclude(std::ostream& includeOfs,
    std::string prefix, std::string name, int value) {
  if (value >= 0) {
    includeOfs << ".define " << prefix << name
      << " " << TStringConversion::intToString(value, TStringConversion::baseHex)
      << std::endl;
  }
  else {
    includeOfs << ".define " << prefix << name
      << " " << TStringConversion::intToString(value, TStringConversion::baseDec)
      << std::endl;
  }
}

void outputNamedSymbol(const TThingyTable& table, std::ostream& ofs,
    std::string symbol, std::string name) {
  TThingyTable::MatchResult result = table.matchTableEntry(symbol);
  if (result.id == -1) {
    throw TGenericException(T_SRCANDLINE,
                            "outputNamedSymbol()",
                            std::string("Unknown symbol: ")
                              + symbol);
  }
  
  ofs << ".define " << name << " " << asHexPrefix(result.id) << std::endl;
}

void outputSymConversionDb(const TThingyTable& tableOrig,
    const TThingyTable& tableConv,
    int symIdOrig,
    std::ostream& ofs, bool convSymMustExist = true) {
  if (!tableOrig.hasEntry(symIdOrig)) {
/*    throw TGenericException(T_SRCANDLINE,
                            "outputSymConversionDb()",
                            std::string("Unknown orig symbol ID: ")
                              + TStringConversion::intToString(symIdOrig));*/
    ofs << ".db " << as2bHexPrefix(0) << std::endl;
    return;
  }
  
  std::string symbol = tableOrig.getEntry(symIdOrig);
  TThingyTable::MatchResult result = tableConv.matchTableEntry(symbol);
  if (result.id == -1) {
    if (convSymMustExist) {
      throw TGenericException(T_SRCANDLINE,
                              "outputSymConversionDb()",
                              std::string("Unknown converted symbol: ")
                                + symbol);
    }
    else {
      ofs << ".db " << as2bHexPrefix(0) << std::endl;
    }
  }
  else {
    ofs << ".db " << as2bHexPrefix(result.id) << std::endl;
  }
  
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cout << "Zero Yon Champ symbol include generator" << endl;
    cout << "Usage: " << argv[0] << " <localeid>"
      << endl;
    return 0;
  }
  
//  string outPrefix(argv[1]);
  string localeId(argv[1]);
  
  TThingyTable tableStd;
  tableStd.readUtf8(std::string("table/") + localeId + "/zero4.tbl");
  TThingyTable tableName;
  tableName.readUtf8(std::string("table/") + localeId + "/zero4_name.tbl");
  
  {
    std::string outName("asm/gen/charmap.inc");
    TFileManip::createDirectoryForFile(outName);
    std::ofstream ofs(outName.c_str());
    
    outputNamedSymbol(tableStd, ofs, "[end]", "textop_end");
    outputNamedSymbol(tableStd, ofs, "\\n", "textop_br");
    outputNamedSymbol(tableStd, ofs, "[jump]", "textop_jump");
    outputNamedSymbol(tableStd, ofs, "[call]", "textop_call");
    outputNamedSymbol(tableStd, ofs, "[call19]", "textop_call19");
    outputNamedSymbol(tableStd, ofs, "[num3]", "textop_num3");
    outputNamedSymbol(tableStd, ofs, "[num3zero]", "textop_num3zero");
    outputNamedSymbol(tableStd, ofs, "[num3ff]", "textop_num3ff");
    outputNamedSymbol(tableStd, ofs, "[num5]", "textop_num5");
    outputNamedSymbol(tableStd, ofs, "[num5zero]", "textop_num5zero");
    outputNamedSymbol(tableStd, ofs, "[num5ff]", "textop_num5ff");
    // NEW FOR TRANSLATION
    outputNamedSymbol(tableStd, ofs, "[jumpNewText]", "textop_jumpNewText");
    outputNamedSymbol(tableStd, ofs, "[font]", "textop_setFont");
    
    outputNamedSymbol(tableStd, ofs, " ", "textchar_space");
    outputNamedSymbol(tableStd, ofs, "...", "textchar_ellipse");
    outputNamedSymbol(tableStd, ofs, "-", "textchar_hyphen");
    outputNamedSymbol(tableStd, ofs, "—", "textchar_emdash");
    outputNamedSymbol(tableStd, ofs, "!", "textchar_exclaim");
    outputNamedSymbol(tableStd, ofs, "?", "textchar_question");
    
    outputNamedSymbol(tableName, ofs, " ", "namechar_space");
  }
  
  {
    std::string outName("asm/gen/name2std.inc");
    TFileManip::createDirectoryForFile(outName);
    std::ofstream ofs(outName.c_str());
    
    for (int i = 0x80; i < 0xF6; i++) {
      outputSymConversionDb(tableName, tableStd, i, ofs);
    }
  }
  
  {
    std::string outName("asm/gen/std2name.inc");
    TFileManip::createDirectoryForFile(outName);
    std::ofstream ofs(outName.c_str());
    
    // TODO: range?
    for (int i = 0x20; i < 0x80; i++) {
      outputSymConversionDb(tableStd, tableName, i, ofs, false);
    }
  }
  
  return 0;
}
