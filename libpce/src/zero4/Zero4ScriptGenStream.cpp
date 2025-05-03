#include "zero4/Zero4ScriptGenStream.h"
#include "zero4/Zero4MsgConsts.h"
#include "zero4/Zero4MsgDism.h"
#include "zero4/Zero4MsgDismException.h"
#include "util/TStringConversion.h"
#include "util/TBufStream.h"
#include "util/TOfstream.h"
#include "util/TParse.h"
#include "util/TStringSearch.h"
#include "util/TJson.h"
#include "util/TFileManip.h"
#include <map>
#include <vector>
#include <iostream>

using namespace BlackT;

namespace Pce {


Zero4ScriptGenStreamEntry::Zero4ScriptGenStreamEntry()
  : type(type_none),
    offset(-1),
    size(-1),
    mayNotExist(false) { }

Zero4ScriptGenStream::Zero4ScriptGenStream()
  : usingFrontPage(false) {
  // HACK
  if (TFileManip::fileExists("table/orig/zero4_front.tbl")) {
    frontTable.readUtf8("table/orig/zero4_front.tbl");
  }
  if (TFileManip::fileExists("table/orig/zero4_back.tbl")) {
    backTable.readUtf8("table/orig/zero4_back.tbl");
  }
}

void Zero4ScriptGenStream::exportToSheet(
    Zero4TranslationSheet& dst,
    std::ostream& ofs,
    std::string idPrefix) const {
  
//  int strNum = 0;
  int currentW = -1;
  int currentH = -1;
  std::string currentBook = "";
  std::string currentChapter = "";
  std::string currentPage = "";
  std::string currentFont = "";
  bool failOnYOverflow = false;
  for (unsigned int i = 0; i < entries.size(); i++) {
    const Zero4ScriptGenStreamEntry& item = entries[i];
    
    if ((item.type == Zero4ScriptGenStreamEntry::type_string)
        ) {
      std::string idString = idPrefix
//          + TStringConversion::intToString(strNum)
//          + "-"
        + TStringConversion::intToString(entries[i].offset,
            TStringConversion::baseHex);
      if (!item.idOverride.empty()) idString = item.idOverride;
      
      BlackT::TJson extraParams;
      extraParams.type = BlackT::TJson::type_object;
      
      ofs << "#STARTSTRING("
        << "\"" << idString << "\""
        << ")" << std::endl;
      
      if (item.offset != -1) {
        ofs << "#SETORIGINALPOS("
          << TStringConversion::intToString(item.offset,
              TStringConversion::baseHex)
          << ", "
          << TStringConversion::intToString(item.size,
              TStringConversion::baseHex)
          << ")" << std::endl;
      }
      
      extraParams.content_object["stringProperties"].type
        = BlackT::TJson::type_object;
      for (const auto& propertyPair: item.propertyMap) {
        ofs << "#SETSTRINGPROPERTY("
          << "\"" << propertyPair.first << "\""
          << ", \"" << propertyPair.second << "\""
          << ")"
          << std::endl;
        
        extraParams.content_object["stringProperties"]
          .content_object[propertyPair.first] = propertyPair.second;
      }
      
      std::string notes = "";
      {
        auto findIt = item.propertyMap.find("autonotes");
        if (findIt != item.propertyMap.end()) {
          notes = findIt->second;
        }
      }
      
      BlackT::TJson extraData;
      extraData.type = BlackT::TJson::type_object;
      
      extraData.content_object["book"]
        = BlackT::TJson(currentBook);
      extraData.content_object["chapter"]
        = BlackT::TJson(currentChapter);
      extraData.content_object["page"]
        = BlackT::TJson(currentPage);
      extraData.content_object["canon_width"]
        = BlackT::TJson(currentW);
      extraData.content_object["canon_height"]
        = BlackT::TJson(currentH);
      extraData.content_object["canon_font"]
        = BlackT::TJson(currentFont);
      extraData.content_object["canon_fail_on_y_overflow"]
        = BlackT::TJson(failOnYOverflow ? 1 : 0);
      extraData.content_object["extra_params"]
        = extraParams;
      
      if (item.subStrings.empty()) {
        dst.addStringEntry(
          idString, item.content, "", "", item.translationPlaceholder,
          notes, extraData);
        if (item.mayNotExist)
          ofs << "#IMPORTIFEXISTS(\"";
        else
          ofs << "#IMPORT(\"";
        
        ofs << idString << "\")" << std::endl;
      }
      else {
        for (int i = 0; i < item.subStrings.size(); i++) {
          const Zero4SubString& subItem = item.subStrings[i];
          
          if (!subItem.visible) {
            ofs << subItem.content << std::endl;
            continue;
          }
          
          // TODO: it may be better to just add an [img] tag or something to the
          // notes and separately upload the images rather than go through the
          // whole backend with the raw portrait IDs
/*          if (subItem.portrait != -1) {
            extraData.content_object["portrait"]
              = BlackT::TJson(subItem.portrait);
            dst.addInlineCommentEntry(
              std::string("[img]proj/zero4/images/portrait_")
              + TStringConversion::intToString(subItem.portrait,
                  TStringConversion::baseHex)
              + ".png[/img]");
          }
          else {
//            auto findIt = extraData.content_object.find("portrait");
//            if (findIt != extraData.content_object.end())
//              extraData.content_object.erase(findIt);
            extraData.content_object.erase("portrait");
          }*/
          
          BlackT::TJson localExtraData = extraData;
          for (const auto& item: subItem.properties) {
            localExtraData.content_object["extra_params"]
              .content_object["stringProperties"]
              .content_object[item.first] = item.second;
          }
          
          {
            auto findIt = subItem.properties.find("indentation");
            if (findIt != subItem.properties.end()) {
              if (findIt->second.compare("firstLine") == 0) {
                ofs << "#SETAUTOINDENT(\"firstLine\")" << std::endl;
              }
              else if (findIt->second.compare("allLines") == 0) {
                ofs << "#SETAUTOINDENT(\"allLines\")" << std::endl;
              }
              else if (findIt->second.compare("allNonFirstLines") == 0) {
                ofs << "#SETAUTOINDENT(\"allNonFirstLines\")" << std::endl;
              }
              else {
                throw TGenericException(T_SRCANDLINE,
                                        "Zero4ScriptGenStream::exportToSheet()",
                                        std::string("Unknown indentation mode: ")
                                          + findIt->second);
              }
            }
            else {
              ofs << "#SETAUTOINDENT(\"none\")" << std::endl;
            }
          }
          
          std::string subId = idString
            + "_" + TStringConversion::intToString(i);
          dst.addStringEntry(
            subId, subItem.content, subItem.prefix, subItem.suffix,
            subItem.translationPlaceholder,
            notes, localExtraData);
          
          if (item.mayNotExist)
            ofs << "#IMPORTIFEXISTS(\"";
          else
            ofs << "#IMPORT(\"";
          ofs << subId << "\")" << std::endl;
        }
      }
      
      ofs << "#ENDSTRING()" << std::endl;
      ofs << std::endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_setRegion) {
      ofs << "#STARTREGION("
        << "\"" << item.content << "\""
        << ")" << std::endl;
      ofs << std::endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_setMetaRegion) {
      ofs << "#STARTMETAREGION("
        << "\"" << item.content << "\""
        << ")" << std::endl;
      ofs << std::endl;
    }
/*    else if (item.type == Zero4ScriptGenStreamEntry::type_setNotCompressible) {
      ofs << "#SETNOTCOMPRESSIBLE("
        << (item.notCompressible ? 1 : 0)
        << ")" << endl;
      ofs << endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_addOverwrite) {
      ofs << "#ADDOVERWRITE("
        << TStringConversion::intToString(item.offset,
          TStringConversion::baseHex)
        << ")" << endl;
      ofs << endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_genericLine) {
      ofs << item.content << endl;
      ofs << endl;
    } */
    else if (item.type == Zero4ScriptGenStreamEntry::type_comment) {
      dst.addCommentEntry(item.content);
      
      ofs << "//===================================" << std::endl;
      ofs << "// " << item.content << std::endl;
      ofs << "//===================================" << std::endl;
      ofs << std::endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_smallComment) {
      dst.addSmallCommentEntry(item.content);
      
      ofs << "//=====" << std::endl;
      ofs << "// " << item.content << std::endl;
      ofs << "//=====" << std::endl;
      ofs << std::endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_marker) {
      dst.addMarkerEntry(item.content);
      
      ofs << "// === MARKER: " << item.content << std::endl;
      ofs << std::endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_genericLine) {
      ofs << item.content << std::endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_setRegionProp) {
      ofs << "#SETREGIONPROPERTY("
        << "\"" << item.propName << "\""
        << ", \"" << item.propValue << "\""
        << ")"
        << std::endl << std::endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_setStringProp) {
      ofs << "#SETSTRINGPROPERTY("
        << "\"" << item.propName << "\""
        << ", \"" << item.propValue << "\""
        << ")"
        << std::endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_setSize) {
      ofs << "#SETSIZE("
        << item.w
        << ", "
        << item.h
        << ")"
        << std::endl;
      currentW = item.w;
      currentH = item.h;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_addFreeSpace) {
      ofs << "#ADDFREESPACE("
        << TStringConversion::intToString(item.offset,
            TStringConversion::baseHex)
        << ", "
        << TStringConversion::intToString(item.size,
            TStringConversion::baseHex)
        << ")"
        << std::endl;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_setBook) {
      dst.addSetBookEntry(item.content, item.name);
      
      currentBook = item.content;
      currentChapter = "";
      currentPage = "";
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_setChapter) {
      if (currentBook.empty()) {
        throw TGenericException(T_SRCANDLINE,
                                    "Zero4ScriptGenStream::exportToSheet()",
                                    "Set chapter of null book");
      }
      
//      dst.addCommentEntry(item.name);
      dst.addSetChapterEntry(item.content, item.name);
      
      ofs << "//===================================" << std::endl;
      ofs << "// " << item.name << std::endl;
      ofs << "//===================================" << std::endl;
      ofs << std::endl;
      
      currentChapter = item.content;
      currentPage = "";
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_setPage) {
      if (currentChapter.empty()) {
        throw TGenericException(T_SRCANDLINE,
                                    "Zero4ScriptGenStream::exportToSheet()",
                                    "Set page of null chapter");
      }
      else if (currentBook.empty()) {
        throw TGenericException(T_SRCANDLINE,
                                    "Zero4ScriptGenStream::exportToSheet()",
                                    "Set page of null book");
      }
      
//      dst.addSmallCommentEntry(item.name);
      dst.addSetPageEntry(item.content, item.name);
      
      ofs << "//=====" << std::endl;
      ofs << "// " << item.name << std::endl;
      ofs << "//=====" << std::endl;
      ofs << std::endl;
      
      currentPage = item.content;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_addFont) {
      ofs << "#ADDFONT(\"" << item.content << "\", \""
        << item.name
        << "\")"
        << std::endl;
      currentFont = item.content;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_setFont) {
      ofs << "#SETFONT(\"" << item.content << "\")"
        << std::endl;
      currentFont = item.content;
    }
    else if (item.type == Zero4ScriptGenStreamEntry::type_setFailOnBoxOverflow) {
      ofs << "#SETFAILONBOXOVERFLOW(" << item.intContent << ")"
        << std::endl;
      failOnYOverflow = (item.intContent != 0) ? true : false;
    }
  }
}

void Zero4ScriptGenStream::addRawTerminatedString(BlackT::TStream& ifs,
                         int terminator,
                         const BlackT::TThingyTable& table,
                         std::string name) {
  int startPos = ifs.tell();
  std::string result;
  while (!ifs.eof()) {
//    std::cerr << std::hex << ifs.tell() << std::endl;
    TThingyTable::MatchResult matchResult = table.matchId(ifs);
    
    if (matchResult.id == -1) {
      throw TGenericException(T_SRCANDLINE,
                                  "Zero4MsgDismEntry::addRawTerminatedString()",
                                  "Could not match symbol");
    }
    
    if (matchResult.id == terminator) break;
    
    result += table.getEntry(matchResult.id);
  }
  
//    std::cerr << "done: " << std::hex << ifs.tell() << std::endl;
  int length = ifs.tell() - startPos;
  
  if (name.empty()) {
    name = std::string("rawstr-")
          + TStringConversion::intToString(startPos,
              TStringConversion::baseHex);
  }
  
  std::map<std::string, std::string> propertyMap;
//  propertyMap["noTerminator"] = "1";
  
  addString(result,
          startPos,
          length,
          name,
          propertyMap);
}

void Zero4ScriptGenStream::addRawUnterminatedString(BlackT::TStream& ifs,
                         int length,
                         const BlackT::TThingyTable& table,
                         std::string name) {
  int startPos = ifs.tell();
  int endPos = startPos + length;
  std::string result;
  while (ifs.tell() < endPos) {
    TThingyTable::MatchResult matchResult = table.matchId(ifs);
    if (matchResult.id == -1) {
      throw TGenericException(T_SRCANDLINE,
                                  "Zero4MsgDismEntry::addRawUnterminatedString()",
                                  "Could not match symbol");
    }
    result += table.getEntry(matchResult.id);
  }
  
  if (name.empty()) {
    name = std::string("rawstr-")
          + TStringConversion::intToString(startPos,
              TStringConversion::baseHex);
  }
  
  std::map<std::string, std::string> propertyMap;
  propertyMap["noTerminator"] = "1";
  
  addString(result,
          startPos,
          length,
          name,
          propertyMap);
}

void Zero4ScriptGenStream::addGenericString(BlackT::TStream& ifs,
                         const BlackT::TThingyTable& table,
                         std::string name) {
  Zero4MsgDism dism;
  try {
    dism.dism(ifs, table);
  }
  catch (Zero4MsgDismException& e) {
    std::cerr << "exception at " << std::hex << ifs.tell()
      << ": " << e.problem() << std::endl;
    throw e;
  }
  
  if (name.empty()) {
    name = std::string("str-")
          + TStringConversion::intToString(dism.msgContentStartOffset,
              TStringConversion::baseHex);
  }
  
  std::map<std::string, std::string> propertyMap;
/*  if (dism.terminatedByPrompt) {
    propertyMap["terminatedByPrompt"] = "1";
  }*/
  
  addString(dism.getStringForm(table),
          dism.msgContentStartOffset,
          dism.msgEndOffset - dism.msgContentStartOffset,
          name,
          propertyMap);
}

void Zero4ScriptGenStream::addGenericStringRelToBase(BlackT::TStream& ifs,
                       int loadAddr, int targetAddr,
                       const BlackT::TThingyTable& table,
                       std::string name) {
  ifs.seek(targetAddr - loadAddr);
  addGenericString(ifs, table, name);
}

void Zero4ScriptGenStream::addScriptString(BlackT::TStream& ifs,
                 const BlackT::TThingyTable& table,
                 std::string name) {
  Zero4MsgDism dism;
//  dism.codepointWidthMap = codepointWidthMap;
  dism.frontTable = &frontTable;
  dism.backTable = &backTable;
  dism.usingFrontPage = &usingFrontPage;
  try {
    dism.dism(ifs, table);
  }
  catch (Zero4MsgDismException& e) {
    std::cerr << "exception at " << std::hex << ifs.tell()
      << ": " << e.problem() << std::endl;
    throw e;
  }
  
  if (name.empty()) {
    name = std::string("str-")
          + TStringConversion::intToString(dism.msgContentStartOffset,
              TStringConversion::baseHex);
  }
  
  std::map<std::string, std::string> propertyMap;
  
  Zero4SubStringCollection subStrings
    = dism.getSplitStringForm(table);
  addSmallComment(std::string("String start: ") + name);
  addSplitString(subStrings,
          dism.msgContentStartOffset,
          dism.msgEndOffset - dism.msgContentStartOffset,
          name,
          propertyMap);
}

void Zero4ScriptGenStream::addString(
    std::string str, int offset, int size, std::string id,
    std::map<std::string, std::string> propertyMap) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_string;
  result.content = str;
  result.offset = offset;
  result.size = size;
  result.idOverride = id;
  result.propertyMap = propertyMap;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSplitString(
    Zero4SubStringCollection& subStrings,
    int offset, int size, std::string id,
    std::map<std::string, std::string> propertyMap) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_string;
//  result.content = str;
  result.subStrings = subStrings;
  result.offset = offset;
  result.size = size;
  result.idOverride = id;
  result.propertyMap = propertyMap;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addPlaceholderString(std::string id,
    bool noTerminator) {
//  addString("", 0, 0, id);
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_string;
  result.content = "";
  result.offset = 0;
  result.size = 0;
  result.idOverride = id;

  std::map<std::string, std::string> propertyMap;
  if (noTerminator) {
    propertyMap["noTerminator"] = "1";
  }
  result.propertyMap = propertyMap;
  
  result.mayNotExist = true;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addPlaceholderStringSet(std::string id, int count,
    bool noTerminator) {
  for (int i = 0; i < count; i++) {
    addPlaceholderString(id + "_" + TStringConversion::intToString(i),
      noTerminator);
  }
}

void Zero4ScriptGenStream::addComment(std::string comment) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_comment;
  result.content = comment;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSmallComment(std::string comment) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_smallComment;
  result.content = comment;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSetRegion(std::string name) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_setRegion;
  result.content = name;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSetMetaRegion(std::string name) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_setMetaRegion;
  result.content = name;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addGenericLine(std::string content) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_genericLine;
  result.content = content;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSetRegionProp(
    std::string name, std::string value) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_setRegionProp;
  result.propName = name;
  result.propValue = value;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSetStringProp(
    std::string name, std::string value) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_setStringProp;
  result.propName = name;
  result.propValue = value;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSetSize(int w, int h) {
/*  std::string str = std::string("#SETSIZE(")
    + TStringConversion::intToString(w)
    + ", "
    + TStringConversion::intToString(h)
    + ")";
  addGenericLine(str);*/
  
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_setSize;
  result.w = w;
  result.h = h;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addFreeSpace(int offset, int size) {
/*  std::string str = std::string("#ADDFREESPACE(")
    + TStringConversion::intToString(offset,
        TStringConversion::baseHex)
    + ", "
    + TStringConversion::intToString(size,
        TStringConversion::baseHex)
    + ")";
  addGenericLine(str);*/
  
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_addFreeSpace;
  result.offset = offset;
  result.size = size;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSetBook(std::string id, std::string name) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_setBook;
  result.content = id;
  result.name = name;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSetChapter(std::string id, std::string name) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_setChapter;
  result.content = id;
  result.name = name;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSetPage(std::string id, std::string name) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_setPage;
  result.content = id;
  result.name = name;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addAddFont(
    std::string font, std::string outputTableName) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_addFont;
  result.content = font;
  result.name = outputTableName;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSetFont(std::string font) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_setFont;
  result.content = font;
  entries.push_back(result);
}

void Zero4ScriptGenStream::addSetFailOnBoxOverflow(bool enabled) {
  Zero4ScriptGenStreamEntry result;
  result.type = Zero4ScriptGenStreamEntry::type_setFailOnBoxOverflow;
  result.intContent = (enabled ? 1 : 0);
  entries.push_back(result);
}

std::string Zero4ScriptGenStream::getHexByte(BlackT::TStream& ifs) {
  std::string str = TStringConversion::intToString(ifs.readu8(),
    TStringConversion::baseHex).substr(2, std::string::npos);
  while (str.size() < 2) str = std::string("0") + str;
  return std::string("<$") + str + ">";
}


}
