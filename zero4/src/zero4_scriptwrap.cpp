#include "zero4/Zero4ScriptReader.h"
#include "zero4/Zero4LineWrapper.h"
#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TStringConversion.h"
#include <cctype>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;
using namespace BlackT;
using namespace Pce;

TThingyTable table;

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cout << "Zero Yon Champ script wrapper" << endl;
    cout << "Usage: " << argv[0] << " <infile> <outfile> <locale>"
      << endl;
    return 0;
  }
  
  string infile = string(argv[1]);
  string outfile = string(argv[2]);
  string localeId = string(argv[3]);
    
  {
    TBufStream ifs;
    ifs.open((infile).c_str());
    
    TLineWrapper::ResultCollection results;
    Zero4LineWrapper wrapper(ifs, results);
    wrapper.setLocaleId(localeId);
    wrapper();
    
    if (results.size() > 0) {
      TOfstream ofs(outfile.c_str());
      for (TLineWrapper::ResultCollection::size_type i = 0;
           i < results.size();
           i++) {
        ofs.write(results[i].str.c_str(), results[i].str.size());
      }
    }
  }
  
  return 0;
}

