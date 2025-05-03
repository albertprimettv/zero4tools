#ifndef ZERO4SCRIPTGENSTREAM_H
#define ZERO4SCRIPTGENSTREAM_H


#include "zero4/Zero4TranslationSheet.h"
#include "zero4/Zero4SubString.h"
#include "zero4/Zero4MsgDism.h"
#include "util/TStream.h"
#include <string>
#include <vector>
#include <map>
#include <iostream>

namespace Pce {


class Zero4ScriptGenStreamEntry {
public:
  Zero4ScriptGenStreamEntry();
  
  enum Type {
    type_none,
    type_comment,
    type_smallComment,
    type_string,
    type_setRegion,
    type_setMetaRegion,
    type_marker,
    type_genericLine,
    type_setRegionProp,
    type_setStringProp,
    type_setSize,
    type_addFreeSpace,
    type_setBook,
    type_setChapter,
    type_setPage,
    type_addFont,
    type_setFont,
    type_setFailOnBoxOverflow
  };
  
  Type type;
  
  std::string content;
  int intContent;
  // HACK: if this is not empty, its strings are dumped in split mode
  // and content string is ignored
  Zero4SubStringCollection subStrings;
  std::map<std::string, std::string> propertyMap;
  
  std::string translationPlaceholder;
  
  int offset;
  int size;
  
//  int regionId;
  bool mayNotExist;
  
  std::string idOverride;
  
  std::string propName;
  std::string propValue;
  
  std::string name;
  
  int w;
  int h;
  
protected:
  
};

typedef std::vector<Zero4ScriptGenStreamEntry>
    Zero4ScriptGenStreamEntryCollection;

class Zero4ScriptGenStream {
public:
  enum MapDumpFlags {
    flag_null            = 0,
    flag_isSmall         = (1 << 0),
    flag_hasLargeSpacing = (1 << 1)
  };  
  
  Zero4ScriptGenStream();
  
  Zero4ScriptGenStreamEntryCollection entries;
  
  void exportToSheet(
      Zero4TranslationSheet& dst,
      std::ostream& ofs,
      std::string idPrefix) const;
  
  void addRawTerminatedString(BlackT::TStream& ifs,
                              int terminator,
                              const BlackT::TThingyTable& table,
                              std::string name = "");
  void addRawUnterminatedString(BlackT::TStream& ifs,
                         int length,
                         const BlackT::TThingyTable& table,
                         std::string name = "");
  void addGenericString(BlackT::TStream& ifs,
                         const BlackT::TThingyTable& table,
                         std::string name = "");
  // for pulling a string from a target position in ifs, using the load address
  // of the content in conjunction with the address of the target within the
  // loaded address space
  void addGenericStringRelToBase(BlackT::TStream& ifs,
                         int loadAddr, int targetAddr,
                         const BlackT::TThingyTable& table,
                         std::string name = "");
  void addScriptString(BlackT::TStream& ifs,
                       const BlackT::TThingyTable& table,
                       std::string name = "");
  
  void addString(std::string str, int offset, int size, std::string id,
    std::map<std::string, std::string> propertyMap
      = std::map<std::string, std::string>());
  void addSplitString(Zero4SubStringCollection& subStrings,
    int offset, int size, std::string id,
    std::map<std::string, std::string> propertyMap
      = std::map<std::string, std::string>());
  
  void addPlaceholderString(std::string id, bool noTerminator = false);
  void addPlaceholderStringSet(std::string id, int count,
    bool noTerminator = false);
  
  void addComment(std::string comment);
  void addSmallComment(std::string comment);
  void addSetRegion(std::string name);
  void addSetMetaRegion(std::string name);
  void addGenericLine(std::string content);
  void addSetRegionProp(std::string name, std::string value);
  void addSetStringProp(std::string name, std::string value);
  void addSetSize(int w, int h);
  void addFreeSpace(int offset, int size);
  void addSetBook(std::string id, std::string name = "");
  void addSetChapter(std::string id, std::string name = "");
  void addSetPage(std::string id, std::string name = "");
  void addAddFont(std::string font, std::string outputTableName);
  void addSetFont(std::string font);
  void addSetFailOnBoxOverflow(bool enabled);
  
  // lower case and katakana text
  BlackT::TThingyTable frontTable;
  // upper case and hiragana text
  BlackT::TThingyTable backTable;
  bool usingFrontPage;
protected:
  static std::string getHexByte(BlackT::TStream& ifs);
//  std::map<int, int> codepointWidthMap;
};


}


#endif
