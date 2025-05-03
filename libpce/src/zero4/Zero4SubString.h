#ifndef ZERO4SUBSTRING_H
#define ZERO4SUBSTRING_H


#include "util/TStream.h"
#include "util/TThingyTable.h"
#include <string>
#include <vector>
#include <map>

namespace Pce {


class Zero4SubString {
public:
  Zero4SubString();
  
  std::string content;
  std::string prefix;
  std::string suffix;
  std::string translationPlaceholder;
  std::map<std::string,std::string> properties;
  bool visible;
  int portrait;
  
protected:
  
};

typedef std::vector<Zero4SubString> Zero4SubStringCollection;


}


#endif
