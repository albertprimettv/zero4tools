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
  if (argc < 4) {
    std::cout << "String -> script data converter" << std::endl;
    std::cout << "Usage: " << argv[0]
      << " <thingy> <msgfile> <outfile>"
      << std::endl;
    return 0;
  }
  
  std::string remapTableName(argv[1]);
  std::string msgFileName(argv[2]);
  std::string outFileName(argv[3]);
  
  TThingyTable remapTable;
  remapTable.readUtf8(remapTableName);
  
  TBufStream ifs;
  ifs.open(msgFileName.c_str());
  
  TBufStream ofs;
  while (!ifs.eof()) {
    TThingyTable::MatchResult matchResult = remapTable.matchTableEntry(ifs);
    if (matchResult.id == -1) {
      std::cerr << "Error: unable to match text in string at " << ifs.tell()
        << std::endl;
      return 1;
    }
    
    int id = matchResult.id;
//    std::cerr << std::hex << matchResult.id << " " << matchResult.size << std::endl;
//    ofs.writeInt(id, matchResult.size, EndiannessTypes::big, SignednessTypes::nosign);
    ofs.writeInt(id, remapTable.entries.at(id).byteCount,
        EndiannessTypes::big, SignednessTypes::nosign);
  }
  
  ofs.save(outFileName.c_str());
  
  return 0;
}
