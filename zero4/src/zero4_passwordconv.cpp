#include "zero4/Zero4MsgConsts.h"
#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TGraphic.h"
#include "util/TStringConversion.h"
#include "util/TFileManip.h"
#include "util/TPngConversion.h"
#include "util/TFreeSpace.h"
#include "util/TBitmapFont.h"
#include <cctype>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;
using namespace BlackT;
using namespace Pce;

//TThingyTable tableStd;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cout << "Zero Yon Champ password converter" << endl;
    cout << "Usage: " << argv[0] << " <oldtable> <newtable> <password>"
      << endl;
    return 0;
  }
  
//  string outPrefix(argv[1]);
  string tableOldName(argv[1]);
  string tableNewName(argv[2]);
  string content(argv[3]);
  
  TThingyTable tableOld;
  tableOld.readUtf8(tableOldName);
  TThingyTable tableNew;
  tableNew.readUtf8(tableNewName);
  
  TBufStream ifs;
  ifs.writeString(content);
  ifs.seek(0);
  
  while (!ifs.eof()) {
    TThingyTable::MatchResult result = tableOld.matchTableEntry(ifs);
    
    int id = result.id;
    
    if (result.id == -1) {
      std::cerr << "Symbol not in old table" << std::endl;
      return 1;
    }
    
    // the japanese version allows inputting symbols that are not actually used
    // for generating valid passwords. these are apparently just remapped to the
    // normal range of valid characters.
    if (id < 0xB0) id += 0x40;
    
    if (!tableNew.hasEntry(id)) {
      std::cerr << "Symbol not in new table" << std::endl;
      return 1;
    }
    
    std::cout << tableNew.getEntry(id);
  }
  
  std::cout << std::endl;
  
  return 0;
}
