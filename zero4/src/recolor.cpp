#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TFileManip.h"
#include "util/TStringConversion.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "util/TOpt.h"
#include "util/TArray.h"
#include <iostream>
#include <string>
#include <map>

using namespace std;
using namespace BlackT;

std::map<TColor, TColor> colorMap;

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
  if (argc < 3) {
    cout << "Color replacement tool" << endl;
    cout << "Usage: " << argv[0] << " <infile> <outfile> [options]" << endl;
    
    cout << "Options: " << endl;
    cout << "  -c <oldcolor> <newcolor>   " << "Replace all pixels of color "
      << " 'oldcolor' with 'newcolor'" << endl;
    cout << "Colors are specified in the format RRGGBB."
      << endl;
    cout << "Pixels that are not fully opaque are ignored."
      << endl;
    
    return 0;
  }
  
  char* infile = argv[1];
  char* outfile = argv[2];
  
  TGraphic dst;
  TPngConversion::RGBAPngToGraphic(string(infile), dst);
  
  for (int i = 0; i < argc - 2; i++) {
    if (std::string(argv[i]).compare("-c") == 0) {
      string oldColor(argv[i + 1]);
      string newColor(argv[i + 2]);
      colorMap[decodeColor(oldColor)] = decodeColor(newColor);
    }
  }
  
  for (int j = 0; j < dst.h(); j++) {
    for (int i = 0; i < dst.w(); i++) {
      TColor color = dst.getPixel(i, j);
      auto it = colorMap.find(color);
      if (it != colorMap.end()) {
        dst.setPixel(i, j, it->second);
      }
    }
  }
  
  TPngConversion::graphicToRGBAPng(string(outfile), dst);
  
  return 0;
}
