#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TStringConversion.h"
#include "util/TCsv.h"
#include "util/TFileManip.h"
#include "util/TParse.h"
#include <string>
#include <iostream>

using namespace std;
using namespace BlackT;

std::string escapeStr(std::string input) {
  std::string output;
  for (auto c: input) {
    if (c == '"') output += "\"\"";
    else output += c;
  }
  return output;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cout << "CSV duplication formula resolver" << endl;
    cout << "Usage: " << argv[0] << " <infile> <outfile>" << endl;
    return 0;
  }
  
  TBufStream ifs;
  ifs.open(argv[1]);
  
  TCsv csv;
  csv.readUtf8(ifs);
  
  for (int j = 0; j < csv.numRows(); j++) {
    for (int i = 0; i < csv.numCols(); i++) {
      std::string content = csv.cell(i, j);
      if ((content.size() >= 5)
          && (content[0] == '=')
          && (content[1] == 'T')
          && (content[2] == '(')) {
        char colLetter = content[3];
        int colNum = (colLetter - (char)'A');
        
        std::string value;
        int index = 4;
        while (index < content.size()) {
          if (content[index] == ')') break;
          value += content[index++];
        }
        
        int rowNum = TStringConversion::stringToInt(value) - 1;
        
//        std::cerr << "substituting: " << i << "," << j << "->" << colNum << "," << rowNum << " -- content = " << csv.cell(colNum, rowNum) << std::endl;
        csv.cell(i, j) = csv.cell(colNum, rowNum);
      }
    }
  }
  
  TBufStream ofs;
  
  for (int j = 0; j < csv.numRows(); j++) {
    for (int i = 0; i < csv.numCols(); i++) {
      std::string str = csv.cell(i, j);
      
      if (str.empty()) {
        ofs.writeString(escapeStr(str));
      }
      else {
        ofs.writeString(std::string("\""));
        ofs.writeString(escapeStr(str));
        ofs.writeString(std::string("\""));
      }
      
      if (i != csv.numCols() - 1)
        ofs.writeString(std::string(","));
    }
    ofs.writeString(std::string("\n"));
  }
  
  ofs.save(argv[2]);
  
  return 0;
}
