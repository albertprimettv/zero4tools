#include "tenma/TenmaMsgDism.h"
#include "tenma/TenmaMsgDismException.h"
#include "pce/PcePalette.h"
#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TGraphic.h"
#include "util/TStringConversion.h"
#include "util/TPngConversion.h"
#include "util/TCsv.h"
#include "util/TSoundFile.h"
#include <string>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace BlackT;
using namespace Pce;

std::string getNumStr(int num) {
  std::string str = TStringConversion::intToString(num);
  while (str.size() < 2) str = string("0") + str;
  return str;
}

std::string getHexWordNumStr(int num) {
  std::string str = TStringConversion::intToString(num,
    TStringConversion::baseHex).substr(2, string::npos);
  while (str.size() < 4) str = string("0") + str;
  return string("$") + str;
}

int main(int argc, char* argv[]) {
  for (int i = 0; i < 256; i++) {
    std::cout << "[char" << std::setiosflags(std::ios::uppercase) << std::setfill('0') << std::setw(4) << std::hex << i << "]" << std::endl;
    std::cout << "glyphWidth=8" << std::endl;
    std::cout << "advanceWidth=8" << std::endl;
  }
  
  return 0;
}
