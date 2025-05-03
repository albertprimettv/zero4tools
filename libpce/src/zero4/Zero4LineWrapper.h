#ifndef ZERO4LINEWRAPPER_H
#define ZERO4LINEWRAPPER_H


#include "util/TStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TThingyTable.h"
#include "util/TLineWrapper.h"
#include "util/TTwoDArray.h"
#include "util/TBitmapFont.h"
#include <string>
#include <vector>
#include <map>
#include <fstream>

namespace Pce {


class Zero4LineWrapper : public BlackT::TLineWrapper {
public:
  typedef std::map<int, int> CharSizeTable;
  typedef BlackT::TTwoDArray<char> KerningMatrix;

  Zero4LineWrapper(BlackT::TStream& src__,
                  ResultCollection& dst__);
  ~Zero4LineWrapper();
  
  /**
   * Return width of a given symbol ID in "units" --
   * pixels, characters, whatever is compatible with the specified xSize.
   */
  virtual int widthOfKey(int key);
  
  virtual int advanceWidthOfKey(int key);
  
  /**
   * Return true if a given symbol ID is considered a word boundary.
   * For English text, this will usually be whitespace characters.
   * Linebreaks can and should be included in this category.
   */
  virtual bool isWordDivider(int key);
  
  virtual bool isVisibleWordDivider(int key, int nextKey);
  
  /**
   * Return true if a given symbol ID constitutes a linebreak.
   * A linebreak is, by default, considered to do the following:
   *   - increment the yPos
   *   - reset the xPos to zero
   */
  virtual bool isLinebreak(int key);
  
  /**
   * Return true if a given symbol ID constitutes a box clear.
   * A box clear is, by default, considered to do the following:
   *   - reset the xPos to zero
   *   - reset the yPos to zero
   */
  virtual bool isBoxClear(int key);
  
  /**
   * This function is called immediately before the next word would normally
   * be output when the following conditions are met:
   *   a.) yPos == ySize, and
   *   b.) the current word's computed width will, when added to xPos, exceed
   *       xSize (necessitating a linebreak)
   * The implementation should handle this as appropriate for the target,
   * such as by outputting a wait/clear command, emitting an error, etc.
   */
  virtual void onBoxFull();
  
  /**
   * Returns the string used for linebreaks symbol.
   */
  virtual std::string linebreakString() const;
  
//  virtual int linebreakHeight() const;
  virtual void handleManualLinebreak(TLineWrapper::Symbol result, int key);
  
  virtual void onSymbolAdded(BlackT::TStream& ifs, int key);
  virtual void afterSymbolAdded(BlackT::TStream& ifs, int key);
  virtual void onLineContentStarted();
  virtual void beforeLinebreak(LinebreakSource clearSrc,
                             int key);
  virtual void afterLinebreak(LinebreakSource clearSrc,
                             int key);
  virtual void beforeBoxClear(BoxClearSource clearSrc,
                             int key);
  virtual void afterBoxClear(BoxClearSource clearSrc,
                             int key);
  virtual void beforeWordFlushed();
  virtual char literalOpenSymbol() const;
  virtual char literalCloseSymbol() const;
  
  virtual void flushActiveScript();
  
  void setFailOnBoxOverflow(bool failOnBoxOverflow__);
  bool didYOverflow() const;
  
  struct LoadedFont {
    BlackT::TBitmapFont font;
    BlackT::TThingyTable glyphIndexTable;
    std::string outputCodepointTableName;
  };
  
  void addActiveFont(std::string name, const LoadedFont& loadedFont);
  void addActiveOutputTable(std::string name, const BlackT::TThingyTable& table);
  void setActiveFont(std::string fontName);
  
protected:

//  enum ClearMode {
//    clearMode_default,
//    clearMode_messageSplit
//  };

  const static char myLiteralOpenSymbol = '<';
  const static char myLiteralCloseSymbol = '>';

  enum BreakMode {
    breakMode_single,
    breakMode_double
  };
  
  enum IndentMode {
    indentMode_none,
    indentMode_firstLine,
    indentMode_allLines,
    indentMode_allNonFirstLines
  };
  
  BreakMode breakMode;
  IndentMode indentMode;
  bool failOnBoxOverflow;
  bool doubleWidthModeOn;
  bool doubleWidthCharsInWord;
  bool padAndCenter_on;
  int padAndCenter_leftPad;
  int padAndCenter_width;
  // list of starting positions in current script buffer of each line
  std::vector<int> lineContentStartOffsets;
  int longestLineLen;
  bool emphModeOn;
  int lastChar;
  bool overflowedY;
  bool currentMessageExceededBox;
  std::string fontBaseDir;
  std::string tableBaseDir;
  std::string activeFontName;
  
  int getLocalBrId() const;
  int getLocalSpaceId() const;
//  int getLocalEspId() const;
  
  virtual bool processUserDirective(BlackT::TStream& ifs);
  
  std::string getPadString(int width, int mode = 0);
  void doLineEndPadChecks();
  void applyPadAndCenterToCurrentLine();
  void applyPadAndCenterToCurrentBlock();
  
  static int swapWordEnd(int value);
  
  typedef std::map<std::string, LoadedFont*> LoadedFontMap;
  typedef std::map<std::string, const LoadedFont*> ActiveFontMap;
  typedef std::map<std::string, BlackT::TThingyTable*> LoadedTableMap;
  typedef std::map<std::string, const BlackT::TThingyTable*> ActiveTableMap;
  
  // contains pointers to any fonts loaded via ADDFONT directive.
  // these are stored internally to the wrapper instance, and destroyed
  // in its destructor.
  LoadedFontMap loadedFonts;
  // contains pointers to any fonts loaded by whatever means, whether
  // that's an ADDFONT directive or a manual load done in code.
  // these may reference storage external to the class, and are not deleted
  // on destruction of the class instance.
  ActiveFontMap activeFonts;
  
  LoadedTableMap loadedOutputTables;
  ActiveTableMap activeOutputTables;
  
  void loadOutputTable(std::string name);
  std::string convertOutputCodepointToSymbol(
    const LoadedFont& font, int key) const;
  int convertOutputCodepointToActiveFontIndex(
    const LoadedFont& font, int key) const;
  const LoadedFont& getCurrentActiveFont() const;
  // returns true if the codepoint given by key has the symbol given by
  // symbol in the current output table
  bool outputCodepointMatchesSymbol(int key, std::string symbol);
};


}


#endif
