#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TStringConversion.h"
#include "util/TThingyTable.h"
#include "util/TStringSearch.h"
#include "util/TFileManip.h"
#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "util/TBitmapFont.h"
#include "util/TOpt.h"
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace BlackT;

int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cout << "Script data -> string converter" << std::endl;
    std::cout << "Usage: " << argv[0]
      << " <thingy> <infile> [outfile]"
      << std::endl;
    std::cout << "If outfile is not specified, result is printed to standard output."
      << std::endl;
    return 0;
  }
  
  std::string remapTableName(argv[1]);
  std::string inFileName(argv[2]);
  std::string outFileName;
  if (argc >= 4)
    outFileName = std::string(argv[3]);
  
  TThingyTable unmappingTable;
  unmappingTable.readUtf8(remapTableName);
  
//  TBufStream ifs;
//  ifs.open(inFileName.c_str());
  
  TBufStream rawInput;
  rawInput.open(inFileName.c_str());
  TBufStream mappedInput;
  while (!rawInput.eof()) {
    TThingyTable::MatchResult result = unmappingTable.matchId(rawInput);
    if (result.id == -1) {
      throw TGenericException(T_SRCANDLINE,
                              "main()",
                              "Unable to match mapped input character");
    }
    
    std::string str = unmappingTable.getEntry(result.id);
    
    mappedInput.writeString(str);
  }
  
  mappedInput.seek(0);
  TBufStream ifs;
  {
    while (!mappedInput.eof()) {
      // HACK
      if ((mappedInput.peek() == '\r')
          || (mappedInput.peek() == '\n')) {
        break;
      }
      
      ifs.put(mappedInput.get());
    }
  }
  
  if (outFileName.empty()) {
    // FIXME: this "conversion" is probably unnecessary, but just in case
    std::string result;
    ifs.seek(0);
    ifs.readFixedLenString(result, ifs.size());
    std::cout << result;
  }
  else {
    ifs.save(outFileName.c_str());
  }
  
  return 0;
}
