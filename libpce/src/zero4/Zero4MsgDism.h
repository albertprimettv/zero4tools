#ifndef ZERO4MSGDISM_H
#define ZERO4MSGDISM_H


#include "zero4/Zero4SubString.h"
#include "util/TStream.h"
#include "util/TThingyTable.h"
#include <string>
#include <vector>
#include <map>

namespace Pce {


class Zero4MsgDismEntry {
public:
//  Zero4MsgDismEntry();
  
  enum Type {
    type_none,
    type_string,
    type_op,
    type_null,
    type_literalString
  };
  
  Type type;
  std::string content;
  int opNum;
  
  std::string getStringForm(const BlackT::TThingyTable& table) const;
  
protected:
  
};

typedef std::vector<Zero4MsgDismEntry> Zero4MsgDismEntryCollection;

class Zero4MsgDism {
public:
  Zero4MsgDism();
  
  enum DismMode {
    dismMode_full,
    dismMode_raw
  };
  
  enum MsgTypeMode {
    msgTypeMode_check,
    msgTypeMode_noCheck
  };
  
  enum MenuTypeMode {
    menuTypeMode_params,
    menuTypeMode_noParams
  };
  
//  void save(BlackT::TStream& ofs) const;
//  void load(BlackT::TStream& ifs);
  
  void dism(BlackT::TStream& ifs,
            const BlackT::TThingyTable& table);
  
  bool hasStringContent() const;
  void stripInitialNonStringContent();
  
  std::string getStringForm(const BlackT::TThingyTable& table,
                            bool noAutoLinebreak = true) const;
  
  Zero4SubStringCollection getSplitStringForm(
    const BlackT::TThingyTable& table) const;
  
  int msgType;
  int msgTypeArg;
  int msgStartOffset;
  int msgContentStartOffset;
  int msgFirstStringStartOffset;
  int msgEndOffset;
  Zero4MsgDismEntryCollection entries;
//  std::map<int, int> codepointWidthMap;
  // lower case and katakana text
  const BlackT::TThingyTable* frontTable;
  // upper case and hiragana text
  const BlackT::TThingyTable* backTable;
  bool* usingFrontPage;
  
protected:
  bool tryStringRead(BlackT::TStream& ifs,
                     const BlackT::TThingyTable& table,
                     std::string& dst);
  
  void generateNextSubStrings(
    const Zero4MsgDismEntryCollection& input,
    Zero4MsgDismEntryCollection::const_iterator& it,
    const BlackT::TThingyTable& table,
    Zero4SubStringCollection& dst,
    int& lineNum,
    int& portraitNum) const;
  static void outputLinebreaks(Zero4SubString& dst,
    const BlackT::TThingyTable& table, int count);
  static std::string getHexByte(BlackT::TStream& ifs);
  
  typedef std::vector<std::string> SymbolList;
  typedef std::vector<SymbolList> SymbolListSequence;
  
  // convert a disassembled literal sequence to encoded symbols
  static void reverseDismLiteralString(
    const std::string& src, const BlackT::TThingyTable& table,
    SymbolList& dstSymbolList);
  static void splitSymbolList(
    const SymbolList& src, std::string splitSym,
    SymbolListSequence& dst);
  static bool containsSymbol(
    const SymbolList& src, std::string sym);
  static bool containsNonWhitespace(
    const SymbolList& src, const BlackT::TThingyTable& table);
  
//  static void splitNametag(
//    const SymbolList& src, const BlackT::TThingyTable& table,
//    SymbolListSequence& dst, std::string splitSym, int tagSizeLimit = 8);
};


}


#endif
