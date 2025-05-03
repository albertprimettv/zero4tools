#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "util/TIniFile.h"
#include "util/TBufStream.h"
#include "util/TOfstream.h"
#include "util/TIfstream.h"
#include "util/TStringConversion.h"
#include "util/TBitmapFont.h"
#include "util/TOpt.h"
#include <iostream>
#include <vector>
#include <map>

using namespace std;
using namespace BlackT;

int spacingX = 16;
int spacingY = 16;

TColor colorFromString(std::string rawColorStr) {
  string rStr = string("0x") + rawColorStr.substr(0, 2);
  string gStr = string("0x") + rawColorStr.substr(2, 2);
  string bStr = string("0x") + rawColorStr.substr(4, 2);
  
  int r = TStringConversion::stringToInt(rStr);
  int g = TStringConversion::stringToInt(gStr);
  int b = TStringConversion::stringToInt(bStr);
  
  return TColor(r, g, b);
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cout << "Font kerning preview image generator" << endl;
    cout << "Usage: " << argv[0] << " <font> <outfile> [options]"
      << endl;
    cout << "Options:" << endl;
    cout << "  -r <start> <end>      Set codepoint start/end range"
      << endl;
    cout << "  -b RRGGBB             Set background color"
      << endl;
    
    return 0;
  }
  
  string fontName(argv[1]);
  string outFile(argv[2]);
  
  TBitmapFont font;
  font.load(fontName);
  
  bool useBgColor = false;
  const char* colorStrPtr = TOpt::getOpt(argc, argv, "-b");
  TColor bgColor;
  if (colorStrPtr != NULL) {
    bgColor = colorFromString(std::string(colorStrPtr));
    useBgColor = true;
  }
  
  int startCodepoint = 0;
  int endCodepoint = font.numFontChars();
  for (int i = 0; i < argc - 2; i++) {
    if (strcmp(argv[i], "-r") == 0) {
      startCodepoint = TStringConversion::stringToInt(string(argv[i + 1]));
      endCodepoint = TStringConversion::stringToInt(string(argv[i + 2]));
    }
  }
  
  int numFontChars = endCodepoint - startCodepoint;
  if (numFontChars < 0) numFontChars = 0;
  
  TGraphic grp(spacingX * numFontChars, spacingY * numFontChars);
//  grp.clear(TColor(0x00, 0x00, 0x00));
  grp.clearTransparent();
  
  if (useBgColor) {
    grp.clear(bgColor);
  }
  
  int outY = 0;
  for (int j = startCodepoint; j < endCodepoint; j++) {
    int outX = 0;
    for (int i = startCodepoint; i < endCodepoint; i++) {
      int firstCharIndex = j;
      int secondCharIndex = i;
      
      int x = outX * spacingX;
      int y = outY * spacingY;
      
      TBitmapFontChar& firstChar = font.fontChar(firstCharIndex);
      TBitmapFontChar& secondChar = font.fontChar(secondCharIndex);
      
      int kerning = font.getKerning(firstCharIndex, secondCharIndex);
//      int totalWidth = firstChar.advanceWidth + secondChar.advanceWidth
//        + kerning;

//      if ((firstCharIndex == 0x4E) && (secondCharIndex == 0x28)) {
//        std::cout << firstChar.advanceWidth << std::endl;
//        std::cout << kerning << std::endl;
//      }
      
      TGraphic combinedGrp(spacingX, spacingY);
      combinedGrp.clearTransparent();
      combinedGrp.blit(firstChar.grp, TRect(0, 0, 0, 0));
      combinedGrp.blit(secondChar.grp, TRect(firstChar.advanceWidth + kerning, 0, 0, 0));
      combinedGrp.regenerateTransparencyModel();
      
      grp.blit(combinedGrp, TRect(x, y, 0, 0));
      ++outX;
    }
    ++outY;
  }
    
  TPngConversion::graphicToRGBAPng(outFile, grp);
  
  return 0;
}
