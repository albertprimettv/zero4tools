#include "zero4/Zero4MsgDism.h"
#include "zero4/Zero4MsgDismException.h"
#include "zero4/Zero4MsgConsts.h"
#include "util/TStringConversion.h"
#include "util/TBufStream.h"
#include "util/TParse.h"
#include "util/TSjis.h"
#include "exception/TGenericException.h"
#include <list>
#include <iostream>

using namespace BlackT;

namespace Pce {


std::string Zero4MsgDismEntry
  ::getStringForm(const TThingyTable& table) const {
  std::string result;
  
  switch (type) {
  case type_string:
  {
    TBufStream ifs;
    ifs.writeString(content);
    ifs.seek(0);
    while (!ifs.eof()) {
      TThingyTable::MatchResult matchResult = table.matchId(ifs);
      if (matchResult.id == -1) {
        throw TGenericException(T_SRCANDLINE,
                                    "Zero4MsgDismEntry::getStringForm()",
                                    "Could not match symbol");
      }
      result += table.getEntry(matchResult.id);
    }
  }
    break;
  case type_literalString:
  {
    result += content;
  }
    break;
  case type_op:
  {
    // op
    if (!table.hasEntry(opNum)) {
      throw TGenericException(T_SRCANDLINE,
                                  "Zero4MsgDismEntry::getStringForm()",
                                  std::string("Could not match op: ")
                                  + TStringConversion::intToString(opNum,
                                      TStringConversion::baseHex));
    }
    result += table.getEntry(opNum);
    // args
    for (int i = 0; i < content.size(); i++) {
      int value = (unsigned char)content[i];
      result += (std::string("<$")
        + TStringConversion::intToFixedWHexString(value, 2)
        + ">");
    }
  }
    break;
  case type_null:
  {
    result += table.getEntry(0x00);
  }
    break;
  default:
    throw TGenericException(T_SRCANDLINE,
                                "Zero4MsgDismEntry::getStringForm()",
                                "Illegal type");
    break;
  }
  
  return result;
}

Zero4MsgDism::Zero4MsgDism()
  : msgType(0),
    msgTypeArg(0),
    msgStartOffset(0),
    msgContentStartOffset(0),
    msgFirstStringStartOffset(-1),
    msgEndOffset(0),
    frontTable(NULL),
    backTable(NULL),
    usingFrontPage(NULL) { }

void Zero4MsgDism::dism(BlackT::TStream& ifs,
          const BlackT::TThingyTable& table) {
  if (ifs.eof()) {
    throw Zero4MsgDismException(T_SRCANDLINE,
                                "Zero4MsgDism::dism()",
                                "out of space");
  }
  
  msgStartOffset = ifs.tell();
  
  msgContentStartOffset = ifs.tell();
  
  // try to read data
  while (true) {
    if (ifs.eof()) {
      throw Zero4MsgDismException(T_SRCANDLINE,
                                  "Zero4MsgDism::dism()",
                                  "out of space");
    }
    
    int basePos = ifs.tell();
    Zero4MsgDismEntry entry;
    
    bool nullCharsAllowed = false;
//    if (dismMode == dismMode_raw) nullCharsAllowed = true;

    const TThingyTable* srcTable = &table;
/*    if (usingFrontPage != NULL) {
      if (*usingFrontPage) srcTable = frontTable;
      else srcTable = backTable;
    }*/
    
    std::string strContent;
    // must also check if content was actually read to string
    // (null strings don't make sense here)
    if (tryStringRead(ifs, *srcTable, strContent)
        && (strContent.size() > 0)) {
      entry.type = Zero4MsgDismEntry::type_string;
      entry.content = strContent;
      entries.push_back(entry);
      
      if (msgFirstStringStartOffset == -1)
        msgFirstStringStartOffset = basePos;
      
      continue;
    }
    else {
      ifs.seek(basePos);
    }
    
    // not a string: check for ops
    
    // terminator
    int opNum = ifs.readu8();
//    ifs.seekoff(-2);
//    int opNumCorrected = ifs.readu16le();

    // op 1B takes a 1b subparam, so if we encounter it, go back and read 2
    // bytes so we handle the appropriate subop
    if ((opNum == Zero4MsgConsts::opcode_subop1B)
        && (ifs.remaining() > 0)) {
      ifs.seekoff(-1);
      opNum = ifs.readu16be();
    }
    
    if (opNum == Zero4MsgConsts::opcode_end) break;
      
/*    if ((msgFirstStringStartOffset == -1)
        && (opNum == Zero4MsgConsts::opcode_color))
      msgFirstStringStartOffset = basePos;*/
    
/*    if (nullCharsAllowed && (opNum == 0x00)) {
//      std::cerr << std::hex << opNum << std::endl;
      entry.type = Zero4MsgDismEntry::type_null;
      entries.push_back(entry);
      continue;
    }*/
    
/*    if (opNum < Zero4MsgConsts::opcodeBase) {
//      std::cout << basePos << " " << opNum << std::endl;
      throw Zero4MsgDismException(T_SRCANDLINE,
                                  "Zero4MsgDism::dism()",
                                  "not valid string");
    }
    
    if (opNum >= Zero4MsgConsts::opcodeRangeEnd) {
//      std::cout << basePos << " " << opNum << std::endl;
      throw Zero4MsgDismException(T_SRCANDLINE,
                                  "Zero4MsgDism::dism()",
                                  "not valid string");
    }*/
    
    if (!Zero4MsgConsts::isOp(opNum)) {
      throw Zero4MsgDismException(T_SRCANDLINE,
                                  "Zero4MsgDism::dism()",
                                  "not valid string");
    }
    
    int opArgsSize = Zero4MsgConsts::getOpArgsSize(opNum);
    if (ifs.remaining() < opArgsSize) {
      throw Zero4MsgDismException(T_SRCANDLINE,
                                  "Zero4MsgDism::dism()",
                                  "not enough space for op args");
    }
    
    entry.type = Zero4MsgDismEntry::type_op;
    entry.opNum = opNum;
    for (int i = 0; i < opArgsSize; i++) entry.content += ifs.get();
    
    // validate op arguments
    // TODO?
    
    if (usingFrontPage != NULL) {
      if (opNum == Zero4MsgConsts::opcode_selectFrontPal) {
        *usingFrontPage = true;
//        continue;
      }
      
      if (opNum == Zero4MsgConsts::opcode_selectBackPal) {
        *usingFrontPage = false;
//        continue;
      }
    }
    
    entries.push_back(entry);
    
    // check for special terminators
    // TODO?
    if (opNum == Zero4MsgConsts::opcode_jump) break;
    if (opNum == Zero4MsgConsts::opcode_subop1B_end) break;
    
//    continue;
  }
  
  msgEndOffset = ifs.tell();
}

std::string Zero4MsgDism::getHexByte(BlackT::TStream& ifs) {
  std::string str = TStringConversion::intToString(ifs.readu8(),
    TStringConversion::baseHex).substr(2, std::string::npos);
  while (str.size() < 2) str = std::string("0") + str;
  return std::string("<$") + str + ">";
}

bool Zero4MsgDism::hasStringContent() const {
  for (Zero4MsgDismEntryCollection::const_iterator it = entries.cbegin();
       it != entries.cend();
       ++it) {
    if (it->type == Zero4MsgDismEntry::type_string) return true;
  }
  
  return false;
}

void Zero4MsgDism::stripInitialNonStringContent() {
  Zero4MsgDismEntryCollection oldEntries = entries;
  entries.clear();
  
  Zero4MsgDismEntryCollection::iterator it = oldEntries.begin();
  while (it != oldEntries.end()) {
    if (it->type == Zero4MsgDismEntry::type_string) break;
//    if ((it->type == Zero4MsgDismEntry::type_op)
//        && (it->opNum == Zero4MsgConsts::)) break;
    
    ++it;
  }
  
  while (it != oldEntries.end()) {
    entries.push_back(*it);
    ++it;
  }
  
  msgContentStartOffset = msgFirstStringStartOffset;
}

std::string Zero4MsgDism::getStringForm(const TThingyTable& table,
                            bool noAutoLinebreak) const {
  std::string result;
  
  int lineNum = 0;
  for (Zero4MsgDismEntryCollection::const_iterator it = entries.cbegin();
       it != entries.cend();
       ++it) {
    const Zero4MsgDismEntry& item = *it;
    
    if (item.type == Zero4MsgDismEntry::type_op) {
      int linebreakCount
        = Zero4MsgConsts::numDismPreOpLinebreaks(item.opNum);
      for (int i = 0; i < linebreakCount; i++) {
        result += "\n";
      }
    }
    
    // FIXME?
    if (!noAutoLinebreak
        && (item.type == Zero4MsgDismEntry::type_op)
        && (item.opNum == Zero4MsgConsts::opcode_br)
        && (lineNum == 4)) {
      // HACK: replace box-breaking linebreaks with an alternate sequence
      // to explicitly notate that a box break should occur here
      // (for use in the translation)
//      result += "\\p";
      result += "#BOXPAUSE()";
    }
    else {
      result += item.getStringForm(table);
    }
    
    if (item.type == Zero4MsgDismEntry::type_op) {
      int linebreakCount
        = Zero4MsgConsts::numDismPostOpLinebreaks(item.opNum);
      for (int i = 0; i < linebreakCount; i++) {
        result += "\n";
      }
      
      if (item.opNum == Zero4MsgConsts::opcode_br) {
//        result += "\n";
        ++lineNum;
        
        // text boxes are automatically broken every three lines
/*        if (!noAutoLinebreak
            && (lineNum >= 3)) {
          result += "\n";
          lineNum = 0;
        }*/
      }
    }
  }
  
  // strip trailing linebreaks
  while (result.back() == '\n') result = result.substr(0, result.size() - 1);
  
  return result;
}

Zero4SubStringCollection Zero4MsgDism::getSplitStringForm(
    const BlackT::TThingyTable& table) const {
  Zero4SubStringCollection result;
  
  Zero4MsgDismEntryCollection::const_iterator it = entries.cbegin();
  int lineNum = 0;
  int portraitNum = -1;
  while (it != entries.cend()) {
//    result.push_back(generateNextSubString(entries, it, table));
    
    const TThingyTable* srcTable = &table;
    if (usingFrontPage != NULL) {
      if (*usingFrontPage) srcTable = frontTable;
      else srcTable = backTable;
    }
    
    generateNextSubStrings(entries, it, *srcTable, result, lineNum, portraitNum);
  }
  
  return result;
}

bool Zero4MsgDism::tryStringRead(BlackT::TStream& ifs,
                                 const BlackT::TThingyTable& table,
                                 std::string& dst) {
  if (ifs.eof()) return false;
  
  while (!ifs.eof()) {
    unsigned int basePos = ifs.tell();
    
    // any special validity checks go here...
    
    ifs.seek(basePos);
    
    // check for valid terminating content
    // (any op, including terminator)
//    unsigned char first = ifs.peek();
    int first = ifs.readu8();
    ifs.seekoff(-1);
    
    if ((first == Zero4MsgConsts::opcode_subop1B)
        && (ifs.remaining() >= 2)) {
      first = ifs.readu16be();
      ifs.seekoff(-2);
    }
    
//    if ((first >= Zero4MsgConsts::opcodeBase)
//        && (first < Zero4MsgConsts::opcodeRangeEnd)) {
    if (Zero4MsgConsts::isOp(first)) {
      bool ignore = false;
      
      // HACK: specially ignore the sequence 190322, ("call 0x2203"), used for
      // printing the protagonist's name.
      // this is given a hardcoded abbreviation to a more easily typed sequence.
      if (((first == 0x19) || (first == 0x05))
          && (ifs.remaining() >= 3)) {
        ifs.seekoff(1);
        int param = ifs.readu16le();
        ifs.seekoff(-3);
        if (param == 0x2203) {
          ignore = true;
        }
        else {
          
        }
      }
      
      if (!ignore)
        return true;
    }
    
    // try to match next character
    TThingyTable::MatchResult matchResult = table.matchId(ifs);
    if (matchResult.id == -1) {
      return false;
    }
    
    // if matched, add raw content
    ifs.seekoff(-matchResult.size);
    for (int i = 0; i < matchResult.size; i++)
      dst += ifs.get();
    
    // if EOF, string is not terminated and therefore invalid
//    if (ifs.eof()) return false;
  }
  
  return false;
}

void Zero4MsgDism::generateNextSubStrings(
    const Zero4MsgDismEntryCollection& input,
    Zero4MsgDismEntryCollection::const_iterator& it,
    const BlackT::TThingyTable& table,
    Zero4SubStringCollection& dst,
    int& lineNum,
    int& portraitNum) const {
  Zero4SubString result;
//  int lineNum = 0;
  bool typeSet = false;
  bool needsAutoPause = false;
  int pendingNumLinebreaks = 0;
  bool fullyTerminated = false;
  bool cleared = false;
  int startingLineNum = lineNum;
  while (it != input.cend()) {
    const Zero4MsgDismEntry& item = *it;
    const TThingyTable* srcTable = &table;
    
    // HACK: wait commands followed by a linebreak start a new box of text
    // (the linebreak is necessary to keep text from continuing on the
    // current line).
    // we don't want to have to deal with those extra linebreaks, so we
    // detect this combination and avoid inlineing it.
/*    bool previousWasWait = false;
    if (it != input.cbegin()) {
      --it;
      previousWasWait = ((it->type == Zero4MsgDismEntry::type_op)
          && (it->opNum == Zero4MsgConsts::opcode_wait));
      ++it;
    }*/
    
    if (usingFrontPage != NULL) {
      if (*usingFrontPage) srcTable = frontTable;
      else srcTable = backTable;
    }
    
    if (!typeSet) {
      if (item.type == Zero4MsgDismEntry::type_op) {
        
        // linebreaks that IMMEDIATELY FOLLOW a wait command are not inlineable
/*        if (previousWasWait && (item.opNum == Zero4MsgConsts::opcode_br)) {
          result.visible = false;
        }
        else*/ if (Zero4MsgConsts::isInlineableOp(item.opNum)) {
          result.visible = true;
        }
        else {
          result.visible = false;
        }
      }
      else {
        // literal
        result.visible = true;
      }
      
      typeSet = true;
      continue;
    }
    else {
      if (item.type == Zero4MsgDismEntry::type_op) {
        bool isInlineable = Zero4MsgConsts::isInlineableOp(item.opNum);
        
/*        if (previousWasWait && (item.opNum == Zero4MsgConsts::opcode_br)) {
          isInlineable = false;
        }*/
        
        if ((item.opNum == Zero4MsgConsts::opcode_end)) {
          fullyTerminated = true;
        }
//        else if (item.opNum == Zero4MsgConsts::opcode_clear) {
//          cleared = true;
//        }
        
        if (result.visible && !isInlineable) {
          break;
        }
        else if (!result.visible && isInlineable) {
          break;
        }
      }
      else {
        // literal
        if (!result.visible) {
          break;
        }
      }
    }
    
    ++it;
    
    if (item.type == Zero4MsgDismEntry::type_op) {
//      if (item.opNum == Zero4MsgConsts::opcode_wait)
//        lastWasWait = true;
//      else
//        lastWasWait = false;
      
      if (usingFrontPage != NULL) {
        if (item.opNum == Zero4MsgConsts::opcode_selectFrontPal) {
          *usingFrontPage = true;
        }
        else if (item.opNum == Zero4MsgConsts::opcode_selectBackPal) {
          *usingFrontPage = false;
        }
        // box clear resets page to back
        else if (item.opNum == Zero4MsgConsts::opcode_clear) {
          *usingFrontPage = false;
        }
      }
      
      int linebreakCount
        = Zero4MsgConsts::numDismPreOpLinebreaks(item.opNum);
      for (int i = 0; i < linebreakCount; i++) {
        result.content += "\n";
      }
    }
//    else {
//      lastWasWait = false;
//    }
    
    if ((item.type == Zero4MsgDismEntry::type_op)
        && ((item.opNum == Zero4MsgConsts::opcode_selectFrontPal)
            || (item.opNum == Zero4MsgConsts::opcode_selectBackPal)
            )) {
      
    }
    else if ((item.type == Zero4MsgDismEntry::type_op)
        && (item.opNum == Zero4MsgConsts::opcode_br)) {
      result.content += item.getStringForm(*srcTable);
    }
    else {
      result.content += item.getStringForm(*srcTable);
    }
    
    if (item.type == Zero4MsgDismEntry::type_op) {
      if (item.opNum != Zero4MsgConsts::opcode_br) {
        int linebreakCount
          = Zero4MsgConsts::numDismPostOpLinebreaks(item.opNum);
        
        for (int i = 0; i < linebreakCount; i++) {
          result.content += "\n";
        }
      }
      
      if (item.opNum == Zero4MsgConsts::opcode_br) {
        result.content += "\n";
        ++lineNum;
        ++pendingNumLinebreaks;
      }
    }
  }
  
  if (it == input.cend())
    fullyTerminated = true;
  
  // strip trailing linebreaks
  while (result.content.back() == '\n')
    result.content = result.content.substr(0, result.content.size() - 1);
  
  // - set flag if first line is indented
  // - set flag if all lines are intented (or empty!).
  //   don't use this if there is only one line.
  // - if any line is indented by more than 2 spaces, don't use any special
  //   modes -- these require manual handling
  
/*  {
    SymbolList rawContent;
    reverseDismLiteralString(result.content, table, rawContent);
    SymbolListSequence splitSeq;
    // split by literal newline ("\n", not "\\n")
    splitSymbolList(rawContent, "\n", splitSeq);
    
    std::vector<int> intendationPerLine;
    std::vector<bool> lineIsEmpty;
    std::vector<int> widthOfLine;
    for (const auto& line: splitSeq) {
      intendationPerLine.push_back(0);
      widthOfLine.push_back(0);
      lineIsEmpty.push_back(false);
      
      if (line.size() < 2) {
        lineIsEmpty.back() = true;
        continue;
      }
      
      for (const auto& sym: line) {
        if (sym.compare("　") == 0) {
          ++intendationPerLine.back();
        }
        
        int codepoint = table.matchTableEntry(sym).id;
        
        int width = 0;
        const auto findIt = codepointWidthMap.find(codepoint);
        if (findIt != codepointWidthMap.end()) {
          widthOfLine.back() += findIt->second;
        }
      }
    }
    
    bool firstLineIsIndented = false;
    bool allLinesAreIndented = true;
    bool allNonFirstLinesAreIndented = false;
    bool deepIndentationExists = false;
    
    if ((intendationPerLine.size() > 0)
        && !lineIsEmpty[0]
        && (intendationPerLine[0] > 0)
        && (intendationPerLine[0] <= 2)) {
      firstLineIsIndented = true;
    }
    
    for (int i = 1; i < intendationPerLine.size(); i++) {
      if (!lineIsEmpty[i]
          && (intendationPerLine[i] <= 0)
          ) {
        allLinesAreIndented = false;
        break;
      }
    }
    
    // if there is more than one line and all lines other than the first are
    // indented, set flags accordingly
    if ((intendationPerLine.size() > 1) && allLinesAreIndented
        && !firstLineIsIndented) {
      allLinesAreIndented = false;
      allNonFirstLinesAreIndented = true;
    }
    
    // all lines are not indented if first line is not indented
    if (!firstLineIsIndented) allLinesAreIndented = false;
    
    for (int i = 0; i < intendationPerLine.size(); i++) {
      if (intendationPerLine[i] > 2) {
        deepIndentationExists = true;
        break;
      }
    }
    
    // if any line is long enough to wrap automatically, the wrapped text
    // on the next line will not be indented, so it doesn't count as an
    // indented line.
    // therefore, all lines can't be indented in this case.
    for (int i = 0; i < intendationPerLine.size(); i++) {
      if (widthOfLine[i] >= 190) {
        allLinesAreIndented = false;
        break;
      }
    }
    
    if (deepIndentationExists) {
    
    }
    else if (allLinesAreIndented
             && (intendationPerLine.size() > 1)) {
      result.properties["indentation"] = "allLines";
    }
    else if (allNonFirstLinesAreIndented) {
      result.properties["indentation"] = "allNonFirstLines";
    }
    else if (firstLineIsIndented) {
      result.properties["indentation"] = "firstLine";
    }
  }
  
  {
    SymbolList rawContent;
    reverseDismLiteralString(result.content, table, rawContent);
    SymbolListSequence splitSeq;
    splitNametag(rawContent, table, splitSeq, table.getEntry(0xED00), 8);
    
    if (splitSeq.size() == 2) {
      std::string nametag;
      std::string content;
      for (const auto& item: splitSeq[0]) nametag += item;
      nametag += table.getEntry(0xED00);
      for (const auto& item: splitSeq[1]) content += item;
      
      result.prefix = nametag;
      result.content = content;
    }
  }
  
  // trailing quote detection
  // (quoted text that is split across multiple boxes will end with a
  // closing quote in a box with no open quote, which is why this is separate
  // from nametag detection)
  {
    SymbolList rawContent;
    reverseDismLiteralString(result.content, table, rawContent);
    SymbolListSequence splitSeq;
    splitSymbolList(rawContent, table.getEntry(0xEE00), splitSeq);
    
    if ((splitSeq.size() == 2)
        && (!containsNonWhitespace(splitSeq[1], table))) {
      std::string content;
      for (const auto& item: splitSeq[0]) content += item;
      
      std::string suffix;
      suffix += table.getEntry(0xEE00);
      for (const auto& item: splitSeq[1]) suffix += item;
      
      result.content = content;
      result.suffix = suffix;
    }
  }
  
  // open "double quote" detection (the protagonist's dialogue is formatted
  // by using these, with no nametag)
  {
    std::string initialPrefix = result.prefix;
    std::string initialContent = result.content;
    std::string initialSuffix = result.suffix;
    
    SymbolList rawContent;
    reverseDismLiteralString(result.content, table, rawContent);
    SymbolListSequence splitSeq;
    splitNametag(rawContent, table, splitSeq, table.getEntry(0xEF00), 3);
    
    if (splitSeq.size() == 2) {
      std::string nametag;
      std::string content;
      for (const auto& item: splitSeq[0]) nametag += item;
      nametag += table.getEntry(0xEF00);
      for (const auto& item: splitSeq[1]) content += item;
      
      result.prefix = nametag;
      result.content = content;
    
      // trailing "double quote" detection
      // (only done if an open quote was found; gil's dialogue generally
      // isn't more than one box long)
      {
        SymbolList rawContent;
        reverseDismLiteralString(result.content, table, rawContent);
        SymbolListSequence splitSeq;
        splitSymbolList(rawContent, table.getEntry(0xF000), splitSeq);
        
        if ((splitSeq.size() == 2)
            && (!containsNonWhitespace(splitSeq[1], table))) {
          std::string content;
          for (const auto& item: splitSeq[0]) content += item;
          
          std::string suffix;
          suffix += table.getEntry(0xF000);
          for (const auto& item: splitSeq[1]) suffix += item;
          
          result.content = content;
          result.suffix = suffix;
        }
        else {
          // if no closing tag found, revert all changes
          result.prefix = initialPrefix;
          result.content = initialContent;
          result.suffix = initialSuffix;
        }
      }
    }
  }*/
  
  if (result.content.size() > 0) {
    dst.push_back(result);
  }
  
  if (cleared) lineNum = 0;
}

void Zero4MsgDism::outputLinebreaks(Zero4SubString& dst,
    const BlackT::TThingyTable& table, int count) {
  Zero4MsgDismEntry brEntry;
  brEntry.type = Zero4MsgDismEntry::type_op;
  brEntry.opNum = Zero4MsgConsts::opcode_br;
  std::string output = brEntry.getStringForm(table);
  for (int i = 0; i < count; i++) {
    dst.content += output;
    dst.content += "\n";
  }
}

void Zero4MsgDism::reverseDismLiteralString(
    const std::string& src, const BlackT::TThingyTable& table,
    SymbolList& dstSymbolList) {
  TBufStream stringIfs;
  stringIfs.writeString(src);
  stringIfs.seek(0);
  while (!stringIfs.eof()) {
    std::string line;
    stringIfs.getLine(line);
    
    TBufStream ifs;
    ifs.writeString(line);
    ifs.seek(0);
    
    while (!ifs.eof()) {
      TThingyTable::MatchResult result = table.matchTableEntry(ifs);
      if (result.id == -1) {
        throw TGenericException(T_SRCANDLINE,
                                "Zero4MsgDism::reverseDismLiteralString()",
                                "Unable to match content");
      }
      
//      dstCodeList.push_back(result.id);
      dstSymbolList.push_back(table.getEntry(result.id));
    }
    
    // add linebreak if not last line
    if (!stringIfs.eof()) dstSymbolList.push_back("\n");
  }
}

void Zero4MsgDism::splitSymbolList(
    const SymbolList& src, std::string splitSym, SymbolListSequence& dst) {
  SymbolList currentList;
  bool lastWasSplit = false;
  for (const auto& item: src) {
    if (item.compare(splitSym) == 0) {
      dst.push_back(currentList);
      currentList.clear();
      lastWasSplit = true;
    }
    else {
      currentList.push_back(item);
      lastWasSplit = false;
    }
  }
  
  // add final list
  if (!currentList.empty() || lastWasSplit) dst.push_back(currentList);
}

bool Zero4MsgDism::containsSymbol(const SymbolList& src, std::string sym) {
  for (const auto& item: src) {
    if (item.compare(sym) == 0) return true;
  }
  return false;
}

bool Zero4MsgDism::containsNonWhitespace(
    const SymbolList& src, const BlackT::TThingyTable& table) {
  for (const auto& item: src) {
    if (!(
        // space
        (item.compare(table.getEntry(0x20)) == 0)
        // end
          || (item.compare(table.getEntry(Zero4MsgConsts::opcode_end)) == 0)
        // linebreak
          || (item.compare(table.getEntry(Zero4MsgConsts::opcode_br)) == 0)
        // wait
          || (item.compare(table.getEntry(Zero4MsgConsts::opcode_wait)) == 0)
        // actual linebreaks
          || (item.compare("\n") == 0)
        )
      ) {
      return true;
    }
  }
  
  return false;
}

/*void Zero4MsgDism::splitNametag(
    const SymbolList& src, const BlackT::TThingyTable& table,
    SymbolListSequence& dst, std::string splitSym, int tagSizeLimit) {
  SymbolListSequence splitContent;
  splitSymbolList(src, splitSym, splitContent);
  
  if ((splitContent.size() != 2)
      || (splitContent[0].size() > tagSizeLimit)
      // japanese comma
      || (containsSymbol(splitContent[0], table.getEntry(0xE200)))
      // linebreak
      || (containsSymbol(splitContent[0], table.getEntry(0xF1FF)))
      ) {
    dst.push_back(src);
    return;
  }
  
  dst = splitContent;
}*/


}
