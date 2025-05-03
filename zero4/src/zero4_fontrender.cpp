#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TFileManip.h"
#include "util/TStringConversion.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "util/TOpt.h"
#include "util/TArray.h"
#include "util/TByte.h"
#include "util/TFileManip.h"
#include "util/TBitmapFont.h"
#include "util/TThingyTable.h"
#include <iostream>
#include <string>

using namespace std;
using namespace BlackT;

TColor decodeColor(std::string colorStr) {
  TColor result;
  result.setA(TColor::fullAlphaOpacity);
  std::string rStr = std::string("0x") + colorStr.substr(0, 2);
  std::string gStr = std::string("0x") + colorStr.substr(2, 2);
  std::string bStr = std::string("0x") + colorStr.substr(4, 2);
  result.setR(TStringConversion::stringToInt(rStr));
  result.setG(TStringConversion::stringToInt(gStr));
  result.setB(TStringConversion::stringToInt(bStr));
  return result;
}

int main(int argc, char* argv[]) {
  if (argc < 5) {
    cout << "Font renderer" << endl;
    cout << "Usage: " << argv[0]
      << " <fontprefix> <stringfile> <table> <outfile> [options]"
      << endl;
    cout << "Options:" << endl;
    cout << "  --bottomshadow <color>   Add a bottom-pixel shadow to the output"
      << endl;
    cout << "Colors are specified in the form RRGGBB." << endl;
    cout << "  --unmap        <table>   Unmap characters using a table before processing"
      << endl;
    
    return 0;
  }
  
  char* fontPrefix = argv[1];
  char* stringfile = argv[2];
  char* tablefile = argv[3];
  char* outfile = argv[4];
  
  bool bottomShadow = false;
  TColor bottomShadowColor;
  
  bool unmapInput = false;
  TThingyTable unmappingTable;
  
  for (int i = 0; i < argc - 1; i++) {
    if (string(argv[i]).compare("--bottomshadow") == 0) {
      bottomShadow = true;
      bottomShadowColor = decodeColor(string(argv[i + 1]));
    }
    else if (string(argv[i]).compare("--unmap") == 0) {
      unmapInput = true;
      unmappingTable.readUtf8(string(argv[i + 1]));
    }
  }
  
  TBitmapFont font;
  font.load(std::string(fontPrefix));
  
  TBufStream rawInput;
  rawInput.open(stringfile);
  TBufStream mappedInput;
  if (unmapInput) {
    while (!rawInput.eof()) {
      TThingyTable::MatchResult result = unmappingTable.matchId(rawInput);
      if (result.id == -1) {
        throw TGenericException(T_SRCANDLINE,
                                "main()",
                                "Unable to match mapped input character");
      }
      
      std::string str = unmappingTable.getEntry(result.id);
      
      mappedInput.writeString(str);
//      std::cerr << unmappingTable.getEntry(result.id);
    }
//    std::cerr << std::endl;
  }
  else {
    mappedInput = rawInput;
  }
  
  mappedInput.seek(0);
  TBufStream ifs;
  {
//    TBufStream filterIfs;
//    filterIfs.open(stringfile);
    while (!mappedInput.eof()) {
      if ((mappedInput.peek() == '\r')
          || (mappedInput.peek() == '\n')) {
        break;
      }
      
      ifs.put(mappedInput.get());
    }
  }
  ifs.seek(0);
  
  TThingyTable table;
  table.readUtf8(tablefile);
  
  TGraphic grp;
  font.render(grp, ifs, table);
  
  if (bottomShadow) {
    for (int j = grp.h() - 1; j >= 0; j--) {
      for (int i = 0; i < grp.w(); i++) {
        if ((grp.getPixel(i, j).a() == TColor::fullAlphaOpacity)
            && (grp.getPixel(i, j + 1).a() == TColor::fullAlphaTransparency)) {
          grp.setPixel(i, j + 1, bottomShadowColor);
        }
      }
    }
  }
  
  TPngConversion::graphicToRGBAPng(std::string(outfile), grp);
  
  return 0;
}
