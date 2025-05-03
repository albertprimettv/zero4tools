#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TBufStream.h"
#include "util/TFileManip.h"
#include "util/TStringConversion.h"
#include "util/TOpt.h"
#include "pce/PceSpriteId.h"
#include <iostream>
#include <string>

using namespace std;
using namespace BlackT;
using namespace Pce;

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cout << "PCE raw sprite -> Zero Yon Champ sprite object converter" << endl;
    cout << "Usage: " << argv[0]
      << " <infile> <outfile> [options]"
      << endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --xoff   Offset sprites horizontally" << std::endl;
    std::cout << "  --yoff   Offset sprites vertically" << std::endl;
    return 0;
  }
  
  std::string inFileName(argv[1]);
  std::string outFileName(argv[2]);
  
  int xOffset = 0;
  int yOffset = 0;
  
  TOpt::readNumericOpt(argc, argv, "--xoff", &xOffset);
  TOpt::readNumericOpt(argc, argv, "--yoff", &yOffset);
  
  TBufStream ifs;
  ifs.open(inFileName.c_str());
  
  TBufStream ofs;
  
  int numSprites = ifs.size() / PceSpriteId::size;
  ofs.writeu8(numSprites);
  
  for (int i = 0; i < numSprites; i++) {
    PceSpriteId sprite;
    sprite.read(ifs);
    
    // TODO: this is probably the flags byte
    ofs.writeu8(0x00);
    ofs.writes8(sprite.y);
    ofs.writes8(sprite.x);
    ofs.writeu8(sprite.pattern);
  }
  
  ofs.save(outFileName.c_str());
  
  return 0;
}
