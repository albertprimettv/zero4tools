#include "zero4/Zero4LineWrapper.h"
#include "zero4/Zero4MsgConsts.h"
#include "util/TParse.h"
#include "util/TStringConversion.h"
#include "exception/TGenericException.h"
#include <iostream>

using namespace BlackT;

namespace Pce {

//const static int controlOpsStart = 0x40;

//const static int code_br4       = 0xEB;
//const static int code_forceBoxEnd = 0xEC;
//const static int code_forceBoxPause = 0xED;
//const static int code_end      = 0xF0FF;
//const static int code_br       = 0xF1FF;
//const static int code_wait     = 0xF2FF;

//const static int fontBase = 0x20;
//const static int fontDteStart = 0x80;

/*const static int code_period   = 0x5F00;
const static int code_exclaim  = 0x6100;
const static int code_question = 0x6200;
const static int code_rquotesing = 0x6300;
const static int code_rquote   = 0x6900;
const static int code_hyphen   = 0x6A00;
const static int code_ellipsis = 0x6B00;
const static int code_dash     = 0x6D00;*/
//const static int code_space1px = 0x7200;
//const static int code_space8px = 0x7300;
// TODO?
//const static int code_space8px = 0x68;
//const static int code_space4px = 0x6D;

Zero4LineWrapper::Zero4LineWrapper(BlackT::TStream& src__,
                ResultCollection& dst__)
  : TLineWrapper(src__, dst__, TThingyTable(), -1, -1),
    breakMode(breakMode_single),
    indentMode(indentMode_none),
    failOnBoxOverflow(false),
    doubleWidthModeOn(false),
    doubleWidthCharsInWord(false),
    padAndCenter_on(false),
    padAndCenter_leftPad(0),
    padAndCenter_width(0),
    longestLineLen(0),
    emphModeOn(false),
    lastChar(-1),
    overflowedY(false),
    currentMessageExceededBox(false),
    fontBaseDir("font/"),
    tableBaseDir("table/") {
//  if (kerningMatrix__ != NULL) kerningMatrix = *kerningMatrix__;
}

Zero4LineWrapper::~Zero4LineWrapper() {
  // destroy internally loaded fonts
  for (auto& item: loadedFonts) {
    delete item.second;
  }
  
  // destroy internally loaded tables
  for (auto& item: loadedOutputTables) {
    delete item.second;
  }
}

int Zero4LineWrapper::getLocalBrId() const {
  return thingy.matchTableEntry("\\n").id;
}

int Zero4LineWrapper::getLocalSpaceId() const {
  // HACK
  return thingy.matchTableEntry(" ").id;
}

int Zero4LineWrapper::widthOfKey(int key) {
  if ((key == getLocalBrId())) return 0;
  else if ((key == Zero4MsgConsts::opcode_end)
            || (key == Zero4MsgConsts::opcode_wait)) return 0;
  else if (Zero4MsgConsts::isOp(key)) return 0;
  // HACK: new font change op
  else if ((key & 0xFFFFFF00)
            == (Zero4MsgConsts::transOp_setFont_std & 0xFF00)) return 0;
  // protagonist's name.
  // assume 5 chars of max 8 pixels each = 40.
  else if ((key == 0x190322) || (key == 0x050322)) return 40;
  else if (key >= 0x10000) return 0;
  
  const LoadedFont& font = getCurrentActiveFont();
  int localGlyphIndex = convertOutputCodepointToActiveFontIndex(font, key);
  int result = font.font.fontChar(localGlyphIndex).advanceWidth;
  
  // apply kerning.
  // note that if fonts are switched and kerning is not reset (e.g. with a
  // linebreak), the kerning from the new font's table will be used, and applied
  // as though lastChar had been its equivalent glyph in the new font.
  if ((lastChar != -1)) {
    int lastCharLocalGlyphIndex
      = convertOutputCodepointToActiveFontIndex(font, lastChar);
    int kernValue
      = font.font.getKerning(lastCharLocalGlyphIndex, localGlyphIndex);
//    std::cerr << std::hex << key << " " << std::hex << lastChar << " " << std::dec << kernValue << std::endl;
    result += kernValue;
  }
  
  return result;
}

int Zero4LineWrapper::advanceWidthOfKey(int key) {
  return widthOfKey(key);
}

bool Zero4LineWrapper::isWordDivider(int key) {
  if ((key == getLocalBrId())
      || (key == getLocalSpaceId()
      )
     ) return true;
  
  return false;
}

bool Zero4LineWrapper::isVisibleWordDivider(int key, int nextKey) {
//  if (isNonstd) return false;
  
/*  if ((key == code_hyphen)
      || (key == code_ellipsis)
      || (key == code_dash)) {
    // if followed by punctuation, do not treat as divider
    if ((nextKey == code_exclaim)
        || (nextKey == code_question)
        || (nextKey == code_rquote)
        || (nextKey == code_rquotesing)) {
      return false;
    }
    
    return true;
  }*/
  
  if (outputCodepointMatchesSymbol(key, "...")
      // hyphen
      || outputCodepointMatchesSymbol(key, "-")
      // em dash
      || outputCodepointMatchesSymbol(key, "—")
      // en dash
//      || outputCodepointMatchesSymbol(key, "–")
      ) {
    // if followed by punctuation, do not treat as divider
    if (outputCodepointMatchesSymbol(nextKey, "!")
        || outputCodepointMatchesSymbol(nextKey, "?")
        || outputCodepointMatchesSymbol(nextKey, "}")
        || outputCodepointMatchesSymbol(nextKey, "'")) {
      return false;
    }
    
    return true;
  }
  
  return false;
}

bool Zero4LineWrapper::isLinebreak(int key) {
  if ((key == getLocalBrId())
      ) return true;
  
  return false;
}

bool Zero4LineWrapper::isBoxClear(int key) {
  // TODO
  if (
//      (key == Zero4MsgConsts::opcode_wait)
      (key == Zero4MsgConsts::opcode_clear)
      // treat setpos as a box clear.
      // we have absolutely no idea what the significance of the new
      // position is, and without this, order of e.g. auto-generated
      // line centering data will be disrupted.
//      || (key == Zero4MsgConsts::opcode_setPrintPos)
    ) return true;
  
  return false;
}

void Zero4LineWrapper::onBoxFull() {
  std::string content;
  if (lineHasContent) {
    // wait
    content += thingy.getEntry(Zero4MsgConsts::opcode_wait);
    // advance to next line after wait
//    content += linebreakString();
    
    currentScriptBuffer.write(content.c_str(), content.size());
    lineHasContent = false;
  }
  // linebreak
  stripCurrentPreDividers();
  
  currentScriptBuffer.put('\n');
  currentScriptBuffer.put('\n');
  xPos = 0;
  // FIXME?
  yPos = 0;
  currentMessageExceededBox = true;
}

std::string Zero4LineWrapper::linebreakString() const {
  std::string breakString = thingy.getEntry(getLocalBrId());
  if (breakMode == breakMode_single) {
    return breakString;
  }
  else {
    return breakString + breakString;
  }
}

void Zero4LineWrapper::onSymbolAdded(BlackT::TStream& ifs, int key) {
/*  if (isLinebreak(key)) {
    if ((yPos != -1) && (yPos >= ySize - 1)) {
      flushActiveWord();
      
    }
  } */
  
//  if (key == code_emphModeToggle) {
//    emphModeOn = !emphModeOn;
//  }
  
  // handle font switch ops
  switch (key) {
  case Zero4MsgConsts::transOp_setFont_std:
    setActiveFont("std");
    break;
  case Zero4MsgConsts::transOp_setFont_monospace:
    setActiveFont("monospace");
    break;
  case Zero4MsgConsts::transOp_setFont_monospaceIt:
    setActiveFont("monospace_it");
    break;
  case Zero4MsgConsts::transOp_setFont_bold:
    setActiveFont("bold");
    break;
  case Zero4MsgConsts::transOp_setFont_italic:
    setActiveFont("italic");
    break;
  case Zero4MsgConsts::transOp_setFont_ascii:
    setActiveFont("asciistrict");
    break;
  default:
    break;
  }
}

void Zero4LineWrapper::afterSymbolAdded(BlackT::TStream& ifs, int key) {
  if ((key < 0x10000)
      && ((key & 0xFFFFFF00) != (Zero4MsgConsts::transOp_setFont_std & 0xFF00))
      && !Zero4MsgConsts::isOp(key))
    lastChar = key;
}

void Zero4LineWrapper
    ::handleManualLinebreak(TLineWrapper::Symbol result, int key) {
  if ((key != getLocalBrId()) || (breakMode == breakMode_single)) {
    TLineWrapper::handleManualLinebreak(result, key);
  }
  else {
    outputLinebreak(linebreakString());
  }
}

void Zero4LineWrapper::onLineContentStarted() {
  lineContentStartOffsets.push_back(currentScriptBuffer.tell());
  
  // ypos will be zero if we encountered a wait command
  // and this linebreak is advancing to the first line of the next
  // text box (because we reset it to a negative value in afterBoxClear
  // specifically to ensure the line count will be correct for the next box).
  // if this is the case, do not apply automatic indentation.
/*  if (
      ((yPos >= 0) && (indentMode == indentMode_allLines))
       || ((yPos >= 1) && (indentMode == indentMode_allNonFirstLines))
       // special case: if the box overflowed and we started a new one,
       // and all non-first lines are being indented, then the first line
       // of then new box should also be indented
       || (currentMessageExceededBox && (yPos >= 0)
            && (indentMode == indentMode_allNonFirstLines))
       || ((yPos == 0) && (indentMode == indentMode_firstLine))
        ) {
    // HACK
    // this will not behave as desired if a word is used that is too long to
    // fit on one line only when the indentation is added to it
    // (no warning, and an overflow in the output).
    // i don't anticipate that being an issue for any situation where
    // we'd actually use indentation, though.
    
    // insert indentation symbol into output
//    std::string indentationSymbol = "\\t";
    // japanese full-width space (same as original character)
    std::string indentationSymbol = "　";
    currentScriptBuffer.writeString(indentationSymbol);
    const LoadedFont& loadedFont = getCurrentActiveFont();
    xPos += loadedFont.font.fontChar(convertOutputCodepointToActiveFontIndex(
      loadedFont, thingy.matchTableEntry(indentationSymbol).id)).advanceWidth;
  }*/
/*  else {
    if (indentMode != indentMode_none) {
      std::string str;
      int pos = currentWordBuffer.tell();
      currentWordBuffer.seek(0);
      while (!currentWordBuffer.eof()) str += currentWordBuffer.get();
      currentWordBuffer.seek(pos);
      
      std::cerr << lineNum << ": " << str << " " << yPos << " " << indentMode << std::endl;
    }
  }*/
}

void Zero4LineWrapper::beforeLinebreak(
    LinebreakSource clearSrc, int key) {
  if (clearSrc == linebreakManual) {
    applyPadAndCenterToCurrentLine();
    doLineEndPadChecks();
  }
}

void Zero4LineWrapper::afterLinebreak(
    LinebreakSource clearSrc, int key) {
/*  if (clearSrc != linebreakBoxEnd) {
    if (spkrOn) {
      xPos = spkrLineInitialX;
    }
  } */
  
/*  if (clearSrc == linebreakManual) {
    if (breakMode == breakMode_double) {
      --yPos;
    }
  } */
  
  // ypos will be zero if we encountered a wait command
  // and this linebreak is advancing to the first line of the next
  // text box (because we reset it to a negative value in afterBoxClear
  // specifically to ensure the line count will be correct for the next box).
  // if this is the case, do not apply automatic indentation.
/*  if (yPos >= 1) {
    if ((indentMode == indentMode_allLines)
        || (indentMode == indentMode_allNonFirstLines)
//        || ((yPos == 0) && (indentMode == indentMode_firstLine))
        ) {
      // HACK
      // insert indentation symbol into output
      std::string indentationSymbol = "\\t";
      currentScriptBuffer.writeString(indentationSymbol);
      const LoadedFont& loadedFont = getCurrentActiveFont();
      xPos += loadedFont.font.fontChar(convertOutputCodepointToActiveFontIndex(
        loadedFont, thingy.matchTableEntry(indentationSymbol).id)).advanceWidth;
    }
  }*/
  
  // linebreak resets kerning
  lastChar = -1;
}

void Zero4LineWrapper::beforeBoxClear(
    BoxClearSource clearSrc, int key) {
//  if (((clearSrc == boxClearManual) && (key == Zero4MsgConsts::opcode_wait))) {
//    xBeforeWait = xPos;
//  }
  if (failOnBoxOverflow && (clearSrc != boxClearManual)) {
    overflowedY = true;
    throw TGenericException(T_SRCANDLINE,
                            "Zero4LineWrapper::beforeBoxClear()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ": box overflow");
  }
  
  doLineEndPadChecks();
  
//  currentScriptBuffer.put('\n');
//  currentScriptBuffer.put('\n');
}

void Zero4LineWrapper::afterBoxClear(
  BoxClearSource clearSrc, int key) {
  // wait pauses but does not automatically break the line
//  if (((clearSrc == boxClearManual) && (key == Zero4MsgConsts::opcode_wait))) {
//    xPos = xBeforeWait;
//    yPos = -1;
/*    if (breakMode == breakMode_single) {
      yPos = -1;
    }
    else {
      yPos = -2;
    } */
//  }
  
  if (((clearSrc == boxClearManual) && (key == Zero4MsgConsts::opcode_wait))) {
    if (breakMode == breakMode_single) {
      yPos = -1;
    }
    else {
      yPos = -2;
    }
  }
  
  // box clear resets kerning
  lastChar = -1;
}

void Zero4LineWrapper::beforeWordFlushed() {
  doubleWidthCharsInWord = false;
}

char Zero4LineWrapper::literalOpenSymbol() const {
  return myLiteralOpenSymbol;
}

char Zero4LineWrapper::literalCloseSymbol() const {
  return myLiteralCloseSymbol;
}

void Zero4LineWrapper::flushActiveScript() {
  TLineWrapper::flushActiveScript();
  lastChar = -1;
  currentMessageExceededBox = false;
}

void Zero4LineWrapper::setFailOnBoxOverflow(bool failOnBoxOverflow__) {
  failOnBoxOverflow = failOnBoxOverflow__;
}

bool Zero4LineWrapper::didYOverflow() const {
  return overflowedY;
}

void Zero4LineWrapper::addActiveFont(
    std::string name, const LoadedFont& loadedFont) {
  activeFonts[name] = &loadedFont;
}

void Zero4LineWrapper::addActiveOutputTable(
    std::string name, const BlackT::TThingyTable& table) {
  activeOutputTables[name] = &table;
}

void Zero4LineWrapper::setActiveFont(std::string fontName) {
  auto findIt = activeFonts.find(fontName);
  if (findIt == activeFonts.end()) {
    throw TGenericException(T_SRCANDLINE,
                            "Zero4LineWrapper::setActiveFont()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ": tried to activate unknown font '"
                              + fontName
                              + "'");
  }
  
  // look up symbol for lastChar in current font
  std::string lastCharSymbol;
  if (lastChar != -1) {
    lastCharSymbol = convertOutputCodepointToSymbol(
      *activeFonts.at(activeFontName), lastChar);
  }
  
  activeFontName = fontName;
  const LoadedFont& font = *findIt->second;
  
  // switch to corresponding output table for font
  std::string outputTableName = font.outputCodepointTableName;
  auto tableFindIt = activeOutputTables.find(outputTableName);
  if (tableFindIt == activeOutputTables.end()) {
    throw TGenericException(T_SRCANDLINE,
                            "Zero4LineWrapper::processUserDirective()",
                            "Line "
                              + TStringConversion::intToString(lineNum)
                              + ": tried to activate unknown output table '"
                              + outputTableName
                              + "'");
  }
  thingy = *tableFindIt->second;
  
  // attempt to convert lastChar to its equivalent symbol in the new text
  // encoding; if none, reset it
  if (lastChar != -1) {
    TThingyTable::MatchResult result = thingy.matchTableEntry(lastCharSymbol);
    if (result.id != -1) {
      lastChar = result.id;
    }
    else {
      lastChar = -1;
    }
  }
}

bool Zero4LineWrapper::processUserDirective(BlackT::TStream& ifs) {
  TParse::skipSpace(ifs);
  
  std::string name = TParse::matchName(ifs);
  TParse::matchChar(ifs, '(');
  
  for (std::string::size_type i = 0; i < name.size(); i++) {
    name[i] = toupper(name[i]);
  }
  
  if (name.compare("STARTSTRING") == 0) {
/*    // don't care
    TParse::matchInt(ifs);
    // don't care
    TParse::matchChar(ifs, ',');
    TParse::matchInt(ifs);
    // don't care
    TParse::matchChar(ifs, ',');
    TParse::matchInt(ifs);
    // isLiteral
    TParse::matchChar(ifs, ',');
    
    // turn on literal mode if literal
    copyEverythingLiterally = (TParse::matchInt(ifs) != 0); */
    
    return false;
  }
  else if (name.compare("ENDSTRING") == 0) {
    // HACK: for pad+center.
    // i don't remember how much of this is actually necessary,
    // this is pure copy and paste from another file
    // FIXME
      // output current word
      flushActiveWord();
      
      doLineEndPadChecks();
      applyPadAndCenterToCurrentLine();
//      applyPadAndCenterToCurrentBlock();
//      padAndCenter_on = false;
      
      beforeBoxClear(boxClearManual, Zero4MsgConsts::opcode_end);
        
      xPos = 0;
      yPos = 0;
      pendingAdvanceWidth = 0;
      
      lineContentStartOffsets.clear();
      longestLineLen = 0;
      
      afterBoxClear(boxClearManual, Zero4MsgConsts::opcode_end);
      
      lineHasContent = false;
      currentLineContentStartInBuffer = -1;
    
    processEndMsg(ifs);
    return true;
  }
  else if (name.compare("SETBREAKMODE") == 0) {
    std::string type = TParse::matchName(ifs);
    
    if (type.compare("SINGLE") == 0) {
      breakMode = breakMode_single;
    }
    else if (type.compare("DOUBLE") == 0) {
      breakMode = breakMode_double;
    }
    else {
      throw TGenericException(T_SRCANDLINE,
                              "Zero4LineWrapper::processUserDirective()",
                              "Line "
                                + TStringConversion::intToString(lineNum)
                                + ": unknown break mode '"
                                + type
                                + "'");
    }
    
    return true;
  }
  else if (name.compare("SETFAILONBOXOVERFLOW") == 0) {
    int value = TParse::matchInt(ifs);
    failOnBoxOverflow = (value != 0);
    
    return true;
  }
  else if (name.compare("SETPADANDCENTER") == 0) {
    padAndCenter_leftPad = TParse::matchInt(ifs);
    TParse::matchChar(ifs, ',');
    padAndCenter_width = TParse::matchInt(ifs);
    
    padAndCenter_on = true;
    
    return true;
  }
  else if (name.compare("DISABLEPADANDCENTER") == 0) {
    // output current word
    flushActiveWord();
    applyPadAndCenterToCurrentLine();
    padAndCenter_on = false;
    
    return true;
  }
  else if ((name.compare("BREAKBOX") == 0)) {
/*    flushActiveWord();
    int count = ySize - yPos;
    for (int i = 0; i < count; i++) {
      outputLinebreak();
    } */
    
    int targetSize = ySize - 1;
    
    if (ySize == -1) {
      throw TGenericException(T_SRCANDLINE,
                              "Zero4LineWrapper::processUserDirective()",
                              "Line "
                                + TStringConversion::intToString(lineNum)
                                + ": tried to break infinitely-high box");
    }
    
    flushActiveWord();
    std::string breakstr = thingy.getEntry(getLocalBrId());
    while (yPos < targetSize) {
      currentScriptBuffer.write(breakstr.c_str(), breakstr.size());
      ++yPos;
    }
    
    onBoxFull();
    
    return true;
  }
/*  else if ((name.compare("BOXPAUSE") == 0)
           || (name.compare("BOXEND") == 0)) {
    if (ySize == -1) {
      // for autowrap, where we don't know how many lines are needed
      // to reach the end of the box
      flushActiveWord();
      applyPadAndCenterToCurrentLine();
      std::string str;
      if (name.compare("BOXPAUSE") == 0) {
        str = thingy.getEntry(code_forceBoxPause);
      }
      else {
        str = thingy.getEntry(code_forceBoxEnd);
      }
      currentScriptBuffer.write(str.c_str(), str.size());
      stripCurrentPreDividers();
      currentScriptBuffer.put('\n');
      currentScriptBuffer.put('\n');
      xPos = 0;
      yPos = 0;
      lineHasContent = false;
      br4ModeOn = false;
      return true;
    }
    
//    std::cerr << "initial: " << yPos << std::endl;
    
    int targetSize = ySize - 1;
    if (name.compare("BOXPAUSE") == 0)
      targetSize = ySize - 0;
    
    flushActiveWord();
    applyPadAndCenterToCurrentLine();
    std::string breakstr = thingy.getEntry(getLocalBrId());
    while (yPos < targetSize) {
      currentScriptBuffer.write(breakstr.c_str(), breakstr.size());
      ++yPos;
    }
    
//    std::cerr << "semifinal: " << yPos << std::endl;
    
    if (name.compare("BOXPAUSE") == 0) {
//      onBoxFull();
      stripCurrentPreDividers();
      
      currentScriptBuffer.put('\n');
      currentScriptBuffer.put('\n');
      xPos = 0;
      yPos = 0;
    }
    
    lineHasContent = false;
//    std::cerr << "final: " << yPos << std::endl;
    return true;
  }*/
  else if (name.compare("BREAKGROUP") == 0) {
    if (yPos > ((ySize/2) - 1)) {
      throw TGenericException(T_SRCANDLINE,
                              "Zero4LineWrapper::processUserDirective()",
                              "Line "
                                + TStringConversion::intToString(lineNum)
                                + ": tried to break overflowed group");
    }
      
    std::string breakstr = thingy.getEntry(getLocalBrId());
  
//    if (currentWordBuffer.size() <= 0) {
//      currentScriptBuffer.write(breakstr.c_str(), breakstr.size());
//    }
    
//    if (yPos != ((ySize/2) - 1)) {
//      std::cerr << "1: " << ySize << " " << yPos << std::endl;
    flushActiveWord();
//      std::cerr << "2: " << ySize << " " << yPos << std::endl;
    while (yPos < ((ySize/2) - 1)) {
//      while (yPos < ((ySize/2))) {
      currentScriptBuffer.write(breakstr.c_str(), breakstr.size());
      ++yPos;
    }
//      std::cerr << "3: " << ySize << " " << yPos << std::endl;
  
    // HACK to correctly handle empty groups
    lineHasContent = true;
    
    onBoxFull();
//      std::cerr << "4: " << ySize << " " << yPos << std::endl;
//    }
    
    return true;
  }
  else if (name.compare("ADDFONT") == 0) {
    if (secureModeOn) return true;
    
    std::string fontName = TParse::matchString(ifs);
    TParse::matchChar(ifs, ',');
    std::string tableName = TParse::matchString(ifs);
    
    // if an internally loaded font by this name exists, destroy it
    auto findIt = loadedFonts.find(fontName);
    if (findIt != loadedFonts.end()) {
      // remove pointer from active font table
      activeFonts.erase(activeFonts.find(fontName));
      // destroy object
      delete findIt->second;
      findIt->second = NULL;
    }
    
    // instantiate new font
    loadedFonts[fontName] = new LoadedFont();
    LoadedFont& loadedFont = *loadedFonts.at(fontName);
    
    // load font
    std::string fontBase = fontBaseDir + localeId + "/" + fontName + "/";
    loadedFont.font.load(fontBase);
    loadedFont.glyphIndexTable.readUtf8(fontBase + "table.tbl");
    loadedFont.outputCodepointTableName = tableName;
    
    // add to active font table
    addActiveFont(fontName, loadedFont);
    
    // load output codepoint table if not already done
    loadOutputTable(tableName);
  }
  else if (name.compare("SETFONT") == 0) {
    std::string fontName = TParse::matchString(ifs);
    
    // switch to new font
    setActiveFont(fontName);
  }
  else if (name.compare("SETAUTOINDENT") == 0) {
    std::string modeName = TParse::matchString(ifs);
    
    if (modeName.compare("none") == 0)
      indentMode = indentMode_none;
    else if (modeName.compare("firstLine") == 0)
      indentMode = indentMode_firstLine;
    else if (modeName.compare("allLines") == 0)
      indentMode = indentMode_allLines;
    else if (modeName.compare("allNonFirstLines") == 0)
      indentMode = indentMode_allNonFirstLines;
    else {
      throw TGenericException(T_SRCANDLINE,
                              "Zero4LineWrapper::processUserDirective()",
                              std::string("Line ")
                                + TStringConversion::intToString(lineNum)
                                + ": unknown indent mode '"
                                + modeName
                                + "'");
    }
    
    return true;
  }
/*  else if (name.compare("OPENFONTKERNINGMATRIX") == 0) {
    if (secureModeOn) return true;
    
    std::string filename = TParse::matchString(ifs);
    TBufStream tableIfs;
    tableIfs.open(filename.c_str());
    
    int dimension = sizeTable.size();
    kerningMatrix = KerningMatrix(dimension, dimension);
    for (int j = 0; j < kerningMatrix.h(); j++) {
      for (int i = 0; i < kerningMatrix.w(); i++) {
        kerningMatrix.data(i, j) = tableIfs.reads8();
      }
    }
    
    return true;
  }
  else if (name.compare("OPENFONTWIDTHTABLE") == 0) {
    if (secureModeOn) return true;
    
    std::string filename = TParse::matchString(ifs);
//    TParse::matchChar(ifs, ',');
//    int textCharsStart = TParse::matchInt(ifs);
    
    sizeTable = CharSizeTable();
    TBufStream tableIfs;
    tableIfs.open(filename.c_str());
    int index = 0;
    while (!tableIfs.eof()) {
      sizeTable[swapWordEnd(index++)] = tableIfs.readu8();
    }
    
    return true;
  }
  else if (name.compare("OPENFONTWIDTHTABLE_EMPH") == 0) {
    if (secureModeOn) return true;
    
    std::string filename = TParse::matchString(ifs);
//    TParse::matchChar(ifs, ',');
//    int textCharsStart = TParse::matchInt(ifs);
    
    sizeTableEmph = CharSizeTable();
    TBufStream tableIfs;
    tableIfs.open(filename.c_str());
    int index = 0;
    while (!tableIfs.eof()) {
      sizeTableEmph[swapWordEnd(index++)] = tableIfs.readu8();
    }
    
    return true;
  }*/
//  else if (name.compare("ENDMSG") == 0) {
//    processEndMsg(ifs);
//    return true;
//  }
  
  return false;
}

std::string Zero4LineWrapper::getPadString(int width, int mode) {
  std::string output;
  if (width <= 0) return output;
  
//  std::cerr << "getPadString: " << xPos << std::endl;
  
  while (width > 0) {
    // HACK: only generate 8px spaces if we're at an 8px boundary
    // (and correctly apply for right-pad mode).
    // the text generation code is optimized not to bother outputting patterns
    // for pattern-aligned 8px spaces, so this saves vram.
//    if ((xPos % 8 == 0) && (width >= 8)) {
    if (
        ((mode >= 0) && (width >= 8))
        || ((mode == -1) && (xPos % 8 == 0) && (width >= 8))
        ) {
//      output += thingy.getEntry(code_space8px);
      output += "[8px]";
      width -= 8;
      // HACK: these xpos modifications won't work in block mode.
      // but we're not using it right now.
      xPos += 8;
    }
    else if (width >= 4) {
//      output += thingy.getEntry(code_space4px);
      output += "[4px]";
      width -= 4;
      xPos += 4;
    }
    else {
//      output += thingy.getEntry(code_space1px);
      output += "[1px]";
      width -= 1;
      xPos += 1;
    }
  }
  
/*  output += thingy.getEntry(TenmaTextScript::op_spaces);
  output += "<$";
  std::string valstr
    = TStringConversion::intToString(width, TStringConversion::baseHex)
        .substr(2, std::string::npos);
  while (valstr.size() < 2) valstr = std::string("0") + valstr;
  output += valstr;
  output += ">";*/
  return output;
}

void Zero4LineWrapper::doLineEndPadChecks() {
  if (xPos > longestLineLen) longestLineLen = xPos;
}

void Zero4LineWrapper::applyPadAndCenterToCurrentLine() {
  if (!padAndCenter_on) return;
  if (!lineHasContent) return;

  TBufStream temp;
  
  // copy script buffer up to start of current line content
  currentScriptBuffer.seek(0);
  temp.writeFrom(currentScriptBuffer, currentLineContentStartInBuffer);
  
  // insert padding at start of current line
//  std::cerr << "x: " << xPos << std::endl;
//  std::cerr << "padAndCenter_width: " << padAndCenter_width << std::endl;
  int centerOffset = 0;
  if (padAndCenter_width > 0) {
    centerOffset = (padAndCenter_width - xPos) / 2;
//    if (centerOffset < padAndCenter_leftPad) {
    if (centerOffset < 0) {
      throw TGenericException(T_SRCANDLINE,
                              "Zero4LineWrapper::applyPadAndCenterToCurrentLine()",
                              "Line "
                                + TStringConversion::intToString(lineNum)
                                + ": cannot fit on line");
    }
  }
//  std::cerr << "centerOffset: " << centerOffset << std::endl;
//  char c;
//  std::cin >> c;
//  temp.writeString(getPadString(padAndCenter_leftPad + centerOffset));
  // HACK: left pad of -1 means "don't center, but pad to target width on right"
  if (padAndCenter_leftPad == -1) {
    int offset = padAndCenter_width - xPos;
    
    if (offset < 0) {
      throw TGenericException(T_SRCANDLINE,
                              "Zero4LineWrapper::applyPadAndCenterToCurrentLine()",
                              "Line "
                                + TStringConversion::intToString(lineNum)
                                + ": cannot fit on line (left pad == -1)");
    }
    
    if (currentScriptBuffer.remaining() > 0)
      temp.writeFrom(currentScriptBuffer, currentScriptBuffer.remaining());
    temp.writeString(
      getPadString(offset, padAndCenter_leftPad));
  }
  else {
    temp.writeString(
      getPadString(padAndCenter_leftPad + centerOffset, padAndCenter_leftPad));
  
    // copy remaining content from original script buffer
    if (currentScriptBuffer.remaining() > 0)
      temp.writeFrom(currentScriptBuffer, currentScriptBuffer.remaining());
  }
  
  currentScriptBuffer = temp;
}

void Zero4LineWrapper::applyPadAndCenterToCurrentBlock() {
  if (!padAndCenter_on) return;
  if (longestLineLen <= 0) return;
  if (lineContentStartOffsets.size() <= 0) return;

  TBufStream temp;
  
//  std::cerr << longestLineLen << std::endl;
  
  int centerOffset = 0;
  if (padAndCenter_width > 0) {
    centerOffset = (padAndCenter_width - longestLineLen) / 2;
    if ((centerOffset < 0)
        || (longestLineLen > padAndCenter_width)) {
      throw TGenericException(T_SRCANDLINE,
                              "Zero4LineWrapper::applyPadAndCenterToCurrentBlock()",
                              "Line "
                                + TStringConversion::intToString(lineNum)
                                + ": cannot fit on line");
    }
  }
  
  // copy script buffer up to start of first line
  currentScriptBuffer.seek(0);
  temp.writeFrom(currentScriptBuffer, lineContentStartOffsets[0]);
  
  std::string padString = getPadString(padAndCenter_leftPad + centerOffset);
//  std::cerr << padString << std::endl;
  // awkwardly splice all subsequent lines together with padding applied
  for (std::vector<int>::size_type i = 0;
       i < lineContentStartOffsets.size() - 1;
       i++) {
    temp.writeString(padString);
    temp.writeFrom(currentScriptBuffer,
      lineContentStartOffsets[i + 1] - lineContentStartOffsets[i]);
  }
  
  // remaining content
  temp.writeString(padString);
  if (currentScriptBuffer.remaining() > 0)
    temp.writeFrom(currentScriptBuffer, currentScriptBuffer.remaining());
  
  currentScriptBuffer = temp;
}

int Zero4LineWrapper::swapWordEnd(int value) {
  return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
}

void Zero4LineWrapper::loadOutputTable(std::string name) {
  // if an internally loaded table by this name exists, destroy it
  auto findIt = loadedOutputTables.find(name);
  if (findIt != loadedOutputTables.end()) {
    // remove pointer from active table
    activeOutputTables.erase(activeOutputTables.find(name));
    // destroy object
    delete findIt->second;
    findIt->second = NULL;
  }
  
  // instantiate new table
  loadedOutputTables[name] = new BlackT::TThingyTable();
  BlackT::TThingyTable& loadedTable = *loadedOutputTables.at(name);
  
  // load content
  std::string tableBase = tableBaseDir + localeId + "/" + name;
  loadedTable.readUtf8(tableBase);
  
  // add to active table
  addActiveOutputTable(name, loadedTable);
}

std::string Zero4LineWrapper::convertOutputCodepointToSymbol(
    const LoadedFont& font, int key) const {
  std::string symbol = thingy.getEntry(key);
  return symbol;
}

int Zero4LineWrapper::convertOutputCodepointToActiveFontIndex(
    const LoadedFont& font, int key) const {
  std::string symbol = thingy.getEntry(key);
//  std::cerr << std::hex << key << " " << symbol << std::endl;
  TThingyTable::MatchResult matchResult
    = font.glyphIndexTable.matchTableEntry(symbol);
  if (matchResult.id == -1) {
    throw TGenericException(T_SRCANDLINE,
         "Zero4LineWrapper::convertOutputCodepointToActiveFontIndex()",
                            std::string("No match for symbol in font: ")
                            + symbol
//                            + " ("
//                            + TStringConversion::intToString(font.glyphIndexTable.entries.size())
//                            + ")"
                            );
  }
  
  return matchResult.id;
}

const Zero4LineWrapper::LoadedFont&
    Zero4LineWrapper::getCurrentActiveFont() const {
  const LoadedFont& font = *activeFonts.at(activeFontName);
  return font;
}

bool Zero4LineWrapper::outputCodepointMatchesSymbol(
    int key, std::string symbol) {
  if (!thingy.hasEntry(key)) return false;
  return (thingy.getEntry(key).compare(symbol) == 0);
}

}
