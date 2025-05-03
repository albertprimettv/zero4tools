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
#include "pce/PcePattern.h"
#include "pce/PcePalette.h"
#include "pce/PcePaletteLine.h"
#include <iostream>
#include <string>

using namespace std;
using namespace BlackT;
using namespace Pce;

int patternsPerRow = 16;

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cout << "PC Engine-format raw tile graphics merger" << endl;
    cout << "Usage: " << argv[0] << " <file1> <file2> <outfile>" << endl;
    return 0;
  }
  
  std::string inFile1(argv[1]);
  std::string inFile2(argv[2]);
  std::string outFile(argv[3]);
  
  TBufStream ifs1;
  ifs1.open(inFile1.c_str());
  TBufStream ifs2;
  ifs2.open(inFile2.c_str());
  
  TBufStream ofs;
  
  TBufStream* first = &ifs1;
  TBufStream* second = &ifs2;
  // first is the larger file
  if (first->size() < second->size()) std::swap(first, second);
  
  while (!second->eof()) {
    ofs.put(first->get() | second->get());
  }
  
  while (!first->eof()) {
    ofs.put(first->get());
  }
  
  ofs.save(outFile.c_str());
  
  return 0;
}
