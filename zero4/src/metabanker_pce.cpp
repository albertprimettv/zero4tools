#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TStringConversion.h"
#include "util/TFileManip.h"
#include "util/TParse.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace BlackT;
        
/*void outputIncludeAndOverwrite(int refBankNum, int refPtrLo, std::string strId,
                               std::string strOutputBase,
                               std::ostream& ofsStringInclude,
                               std::ostream& ofsStringOverwrite) {
  ofsStringOverwrite << ".bank " << refBankNum << " slot 0" << std::endl;
  ofsStringOverwrite << ".orga " << getHexWordNumStr(refPtrLo) << std::endl;
  ofsStringOverwrite << ".section \"str ref overwrite "
      << strId << "\" overwrite" << std::endl;
    ofsStringOverwrite << "  .dl " << labelNameSubstitute(strId) << std::endl;
  ofsStringOverwrite << ".ends" << std::endl << std::endl;
  
  ofsStringInclude << ".slot 0" << std::endl;
  ofsStringInclude << ".section \"str include "
      << strId << "\" superfree" << std::endl;
    ofsStringInclude << "  " << labelNameSubstitute(strId) << ":" << std::endl;
    ofsStringInclude << "    .incbin \"" << strOutputBase << strId
      << ".bin\"" << std::endl;
  ofsStringInclude << ".ends" << std::endl << std::endl;
}*/

const static int metabankSize = 0x10000;

struct MetabankCommand {
  enum Type {
    type_importBank,
    type_addEmptyBank,
    type_outputInclude
  };
  
  Type type;
  std::string name;
  std::string srcFile;
  std::string dstFile;
  int srcOffset;
  int size;
  int loadAddr;
};

enum Mode {
  mode_none,
  mode_split,
  mode_merge
};

std::string asDollarHex(int value) {
  std::string str = TStringConversion::intToString(value,
    TStringConversion::baseHex);
  str = str.substr(2, std::string::npos);
  str = std::string("$") + str;
  return str;
}

void splitMetabankFile(
    const std::vector<MetabankCommand>& commands, std::string inName) {
  TBufStream ifs;
  ifs.open(inName.c_str());
  for (const auto& command: commands) {
    switch (command.type) {
    case MetabankCommand::type_importBank:
    case MetabankCommand::type_addEmptyBank:
    {
      int nextPos = ifs.tell() + metabankSize;
      
      TBufStream ofs;
      if (TFileManip::fileExists(command.dstFile))
        ofs.open(command.dstFile.c_str());
      else
        TFileManip::createDirectoryForFile(command.dstFile);
      
      // TODO: we always patch dst at the src offset.
      // fine for ROMs, but may not be the general behavior we want.
      ofs.seek(command.srcOffset);
      
      ifs.seekoff(command.loadAddr);
      ofs.writeFrom(ifs, command.size);
      ofs.save(command.dstFile.c_str());
      
      ifs.seek(nextPos);
    }
      break;
/*    case MetabankCommand::type_addEmptyBank:
    {
      int nextPos = ifs.tell() + metabankSize;
      
      TBufStream ofs;
      if (TFileManip::fileExists(command.dstFile))
        ofs.open(command.dstFile.c_str());
      else
        TFileManip::createDirectoryForFile(command.dstFile);
      
      ofs.seek(command.srcOffset);
      ofs.writeFrom(ifs, command.size);
      
      ofs.save(command.dstFile.c_str());
      ifs.seek(nextPos);
    }
      break;*/
    case MetabankCommand::type_outputInclude:
    {
      
    }
      break;
    default:
      break;
    }
  }
}

void makeMetabankFile(
    const std::vector<MetabankCommand>& commands, std::string outName) {
  TBufStream ofs;
  for (const auto& command: commands) {
    switch (command.type) {
    case MetabankCommand::type_importBank:
    {
      TBufStream ifs;
      ifs.open(command.srcFile.c_str());
      ifs.seek(command.srcOffset);
      
//      std::cerr << command.srcFile << " " << ifs.size() << " " << command.srcOffset << " " << command.size << std::endl;
      
      // pre-padding
      for (int i = 0; i < command.loadAddr; i++) ofs.put(0xFF);
      
      // content
      ofs.writeFrom(ifs, command.size);
      
      // post-padding
      int remainder = metabankSize - (command.loadAddr + command.size);
      for (int i = 0; i < remainder; i++) ofs.put(0xFF);
    }
      break;
    case MetabankCommand::type_addEmptyBank:
    {
      for (int i = 0; i < metabankSize; i++) ofs.put(0xFF);
    }
      break;
    case MetabankCommand::type_outputInclude:
    {
      std::ostringstream oss;
      int metabankNum = 0;
      for (const auto& command: commands) {
        switch (command.type) {
        case MetabankCommand::type_importBank:
        case MetabankCommand::type_addEmptyBank:
          oss << ".define metabank_" << command.name
            << " " << asDollarHex(metabankNum) << std::endl;
          oss << "  .define metabank_" << command.name
            << "_loadAddr " << asDollarHex(command.loadAddr) << std::endl;
          oss << "  .define metabank_" << command.name
            << "_size " << asDollarHex(command.size) << std::endl;
          ++metabankNum;
          break;
        default:
          break;
        }
      }
      
      oss << std::endl;
      oss << ".define numMetabanks"
        << " " << asDollarHex(metabankNum) << std::endl;
      
      TBufStream ofs;
      ofs.writeString(oss.str());
      TFileManip::createDirectoryForFile(command.dstFile);
      ofs.save(command.dstFile.c_str());
    }
      break;
    default:
      break;
    }
  }
  
//  std::cerr << outName << std::endl;
  TFileManip::createDirectoryForFile(outName);
  ofs.save(outName.c_str());
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    std::cout << "Metabank splitter/merger" << std::endl;
    std::cout << "Usage: " << argv[0]
      << " <mode> <scriptfile> <metabankfile> [args]"
      << std::endl;
    std::cout << "Mode can be \"merge\" or \"split\"." << std::endl;
    return 0;
  }
  
  Mode mode = mode_none;
  
  std::string modeStr(argv[1]);
  
  if (modeStr.compare("split") == 0) {
    mode = mode_split;
  }
  else if (modeStr.compare("merge") == 0) {
    mode = mode_merge;
  }
  else {
    std::cerr << "Unknown mode: " << modeStr << std::endl;
    return 1;
  }
  
  std::string scriptFileStr(argv[2]);
  std::string metabankFileStr(argv[3]);
  
  std::vector<MetabankCommand> commands;
  
  TBufStream scriptIfs;
  scriptIfs.open(scriptFileStr.c_str());
  while (!scriptIfs.eof()) {
    std::string line;
    scriptIfs.getLine(line);
    
    TBufStream ifs;
    ifs.writeString(line);
    ifs.seek(0);
    
    TParse::skipSpace(ifs);
    
    // ignore empty or whitespace-only lines
    if (ifs.eof()) continue;
    
    // ignore comment lines
    if ((ifs.remaining() >= 2)) {
      if (ifs.peek() == '/') {
        ifs.get();
        if (ifs.peek() == '/') continue;
        ifs.seekoff(-1);
      }
    }
    
    MetabankCommand command;
    
    std::string commandName = TParse::getSpacedSymbol(ifs);
    if (commandName.compare("importbank") == 0) {
      command.type = MetabankCommand::type_importBank;
      command.name = TParse::matchString(ifs);
      command.srcFile = TParse::matchString(ifs);
      command.dstFile = TParse::matchString(ifs);
      command.srcOffset = TParse::matchInt(ifs);
      command.size = TParse::matchInt(ifs);
      command.loadAddr = TParse::matchInt(ifs);
    }
    else if (commandName.compare("addemptybank") == 0) {
      command.type = MetabankCommand::type_addEmptyBank;
      command.name = TParse::matchString(ifs);
      command.dstFile = TParse::matchString(ifs);
      command.srcOffset = TParse::matchInt(ifs);
      command.size = TParse::matchInt(ifs);
      command.loadAddr = TParse::matchInt(ifs);
    }
    else if (commandName.compare("outputinclude") == 0) {
      command.type = MetabankCommand::type_outputInclude;
      command.dstFile = TParse::matchString(ifs);
    }
    else {
      std::cerr << "Unknown command: " << commandName << std::endl;
      return 1;
    }
    
    commands.push_back(command);
  }
  
  switch (mode) {
  case mode_split:
    splitMetabankFile(commands, metabankFileStr);
    break;
  case mode_merge:
    makeMetabankFile(commands, metabankFileStr);
    break;
  default:
    
    break;
  }
  
  return 0;
}
