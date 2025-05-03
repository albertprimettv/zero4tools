#include "zero4/Zero4ScriptReader.h"
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

//#define BUILD_COMPRESSION_DICTIONARY 1

using namespace std;
using namespace BlackT;
using namespace Pce;

const static int textCharsStart = 0x20;
const static int textCharsEnd = textCharsStart + 0xE0;
const static int textEncodingMax = 0x100;
const static int maxDictionarySymbols = textEncodingMax - textCharsEnd;

//TThingyTable tableStd;

string as2bHex(int num) {
  string str = TStringConversion::intToString(num,
                  TStringConversion::baseHex).substr(2, string::npos);
  while (str.size() < 2) str = string("0") + str;
  
//  return "<$" + str + ">";
  return str;
}

string as2bHexPrefix(int num) {
  return "$" + as2bHex(num) + "";
}

std::string getNumStr(int num) {
  std::string str = TStringConversion::intToString(num);
  while (str.size() < 2) str = string("0") + str;
  return str;
}

std::string getHexByteNumStr(int num) {
  std::string str = TStringConversion::intToString(num,
    TStringConversion::baseHex).substr(2, string::npos);
  while (str.size() < 2) str = string("0") + str;
  return string("$") + str;
}

std::string getHexWordNumStr(int num) {
  std::string str = TStringConversion::intToString(num,
    TStringConversion::baseHex).substr(2, string::npos);
  while (str.size() < 4) str = string("0") + str;
  return string("$") + str;
}
                      

void binToDcb(TStream& ifs, std::ostream& ofs) {
  int constsPerLine = 16;
  
  while (true) {
    if (ifs.eof()) break;
    
    ofs << "  .db ";
    
    for (int i = 0; i < constsPerLine; i++) {
      if (ifs.eof()) break;
      
      TByte next = ifs.get();
      ofs << as2bHexPrefix(next);
      if (!ifs.eof() && (i != constsPerLine - 1)) ofs << ",";
    }
    
    ofs << std::endl;
  }
}

//==============================================================
// compressor
//==============================================================

typedef std::map<std::string, int> UseCountTable;
//typedef std::map<std::string, double> EfficiencyTable;
typedef std::map<double, std::string> EfficiencyTable;

bool isCompressible(std::string& str) {
  for (int i = 0; i < str.size(); i++) {
    if ((unsigned char)str[i] < textCharsStart) return false;
    if ((unsigned char)str[i] >= textCharsEnd) return false;
//    if ((unsigned char)str[i] == fontEmphToggleOp) return false;
  }
  
  return true;
}

void addStringToUseCountTable(std::string& input,
                        UseCountTable& useCountTable,
                        int minLength, int maxLength) {
  int total = input.size() - minLength;
  if (total <= 0) return;
  
  for (int i = 0; i < total; ) {
    int basePos = i;
    for (int j = minLength; j < maxLength; j++) {
      int length = j;
      if (basePos + length >= input.size()) break;
      
      std::string str = input.substr(basePos, length);
      
      // HACK: avoid analyzing parameters of control sequences
      // the ops themselves are already ignored in the isCompressible check;
      // we just check when an op enters into the first byte of the string,
      // then advance the check position so the parameter byte will
      // never be considered
/*      if ((str.size() > 0) && ((unsigned char)str[0] < textCharsStart)) {
        unsigned char value = str[0];
        if ((value == 0x02) // "L"
            || (value == 0x05) // "P"
            || (value == 0x06)) { // "W"
          // skip the argument byte
          i += 1;
        }
        break;
      }*/
      if (str.size() > 0) {
        unsigned char value = str[0];
        if (Zero4MsgConsts::isOp(value)) {
          // skip the arguments
          i += Zero4MsgConsts::getOpArgsSize(value);
          break;
        }
      }
      
      if (!isCompressible(str)) break;
      
      ++(useCountTable[str]);
    }
    
    // skip literal arguments to ops
/*    if ((unsigned char)input[i] < textCharsStart) {
      ++i;
      int opSize = numOpParamWords((unsigned char)input[i]);
      i += opSize;
    }
    else {
      ++i;
    } */
    ++i;
  }
}

void addRegionsToUseCountTable(Zero4ScriptReader::NameToRegionMap& input,
                        UseCountTable& useCountTable,
                        int minLength, int maxLength) {
  for (Zero4ScriptReader::NameToRegionMap::iterator it = input.begin();
       it != input.end();
       ++it) {
    Zero4ScriptReader::ResultCollection& results = it->second.strings;
    for (Zero4ScriptReader::ResultCollection::iterator jt = results.begin();
         jt != results.end();
         ++jt) {
//      std::cerr << jt->srcOffset << std::endl;
      if (jt->isLiteral) continue;
      if (jt->isNotCompressible) continue;
      
      addStringToUseCountTable(jt->str, useCountTable,
                               minLength, maxLength);
    }
  }
}

void buildEfficiencyTable(UseCountTable& useCountTable,
                        EfficiencyTable& efficiencyTable) {
  for (UseCountTable::iterator it = useCountTable.begin();
       it != useCountTable.end();
       ++it) {
    std::string str = it->first;
    // penalize by 1 byte (length of the dictionary code)
    double strLen = str.size() - 1;
    double uses = it->second;
//    efficiencyTable[str] = strLen / uses;
    
    efficiencyTable[strLen / uses] = str;
  }
}

void applyDictionaryEntry(std::string entry,
                          Zero4ScriptReader::NameToRegionMap& input,
                          std::string replacement) {
  for (Zero4ScriptReader::NameToRegionMap::iterator it = input.begin();
       it != input.end();
       ++it) {
    Zero4ScriptReader::ResultCollection& results = it->second.strings;
    int index = -1;
    for (Zero4ScriptReader::ResultCollection::iterator jt = results.begin();
         jt != results.end();
         ++jt) {
      ++index;
      
      if (jt->isNotCompressible) continue;
      
      std::string str = jt->str;
      if (str.size() < entry.size()) continue;
      
      std::string newStr;
      int i;
      for (i = 0; i < str.size() - entry.size(); ) {
        if (Zero4MsgConsts::isOp((unsigned char)str[i])) {
/*          int numParams = numOpParamWords((unsigned char)str[i]);
          
          newStr += str[i];
          for (int j = 0; j < numParams; j++) {
            newStr += str[i + 1 + j];
          }
          
          ++i;
          i += numParams; */
          
/*          newStr += str[i];
          ++i;
          continue;*/
          
          int numParams = Zero4MsgConsts::getOpArgsSize((unsigned char)str[i]);
          newStr += str[i++];
          for (int j = 0; j < numParams; j++) {
            newStr += str[i + j];
          }
          i += numParams;
          
          continue;
        }
        
        if (entry.compare(str.substr(i, entry.size())) == 0) {
          newStr += replacement;
          i += entry.size();
        }
        else {
          newStr += str[i];
          ++i;
        }
      }
      
      while (i < str.size()) newStr += str[i++];
      
      jt->str = newStr;
    }
  }
}

void generateCompressionDictionary(
    Zero4ScriptReader::NameToRegionMap& results,
    std::string outputDictFileName) {
  TBufStream dictOfs;
  for (int i = 0; i < maxDictionarySymbols; i++) {
//    cerr << i << endl;
    UseCountTable useCountTable;
    addRegionsToUseCountTable(results, useCountTable, 2, 3);
    EfficiencyTable efficiencyTable;
    buildEfficiencyTable(useCountTable, efficiencyTable);
    
//    std::cout << efficiencyTable.begin()->first << std::endl;
    
    // if no compressions are possible, give up
    if (efficiencyTable.empty()) break;
    
    int symbol = i + textCharsEnd;
    applyDictionaryEntry(efficiencyTable.begin()->second,
                         results,
                         std::string() + (char)symbol);
    
    // debug
/*    TBufStream temp;
    temp.writeString(efficiencyTable.begin()->second);
    temp.seek(0);
//    binToDcb(temp, cout);
    std::cout << "\"";
    while (!temp.eof()) {
      std::cout << table.getEntry(temp.get());
    }
    std::cout << "\"" << std::endl; */
    
    dictOfs.writeString(efficiencyTable.begin()->second);
  }
  
//  dictOfs.save((outPrefix + "dictionary.bin").c_str());
  dictOfs.save(outputDictFileName.c_str());
}

// merge a set of NameToRegionMaps into a single NameToRegionMap
void mergeResultMaps(
    std::vector<Zero4ScriptReader::NameToRegionMap*>& allSrcPtrs,
    Zero4ScriptReader::NameToRegionMap& dst) {
  int targetOutputId = 0;
  for (std::vector<Zero4ScriptReader::NameToRegionMap*>::iterator it
        = allSrcPtrs.begin();
       it != allSrcPtrs.end();
       ++it) {
    Zero4ScriptReader::NameToRegionMap& src = **it;
    for (Zero4ScriptReader::NameToRegionMap::iterator jt = src.begin();
         jt != src.end();
         ++jt) {
      if (jt->second.propertyIsTrue("uncompressible")) continue;
      dst[TStringConversion::intToString(targetOutputId++)] = jt->second;
    }
  }
}

// undo the effect of mergeResultMaps(), applying any changes made to
// the merged maps back to the separate originals
void unmergeResultMaps(
    Zero4ScriptReader::NameToRegionMap& src,
    std::vector<Zero4ScriptReader::NameToRegionMap*>& allSrcPtrs) {
  int targetInputId = 0;
  for (std::vector<Zero4ScriptReader::NameToRegionMap*>::iterator it
        = allSrcPtrs.begin();
       it != allSrcPtrs.end();
       ++it) {
    Zero4ScriptReader::NameToRegionMap& dst = **it;
    for (Zero4ScriptReader::NameToRegionMap::iterator jt = dst.begin();
         jt != dst.end();
         ++jt) {
      if (jt->second.propertyIsTrue("uncompressible")) continue;
      jt->second = src[TStringConversion::intToString(targetInputId++)];
    }
  }
}

//==============================================================
// export functions
//==============================================================

void outputPrefixedStringInclude(std::ostream& includeOfs,
    std::string prefix, std::string name, std::string value) {
  includeOfs << ".define " << prefix << name
    << " \"" << value << "\""
    << std::endl;
}

void outputPrefixedNumInclude(std::ostream& includeOfs,
    std::string prefix, std::string name, int value) {
  if (value >= 0) {
    includeOfs << ".define " << prefix << name
      << " " << TStringConversion::intToString(value, TStringConversion::baseHex)
      << std::endl;
  }
  else {
    includeOfs << ".define " << prefix << name
      << " " << TStringConversion::intToString(value, TStringConversion::baseDec)
      << std::endl;
  }
}

void readScript(std::string filename, Zero4ScriptReader::NameToRegionMap& dst,
                std::string localeId) {
  TBufStream ifs;
  ifs.open(filename.c_str());
  Zero4ScriptReader(ifs, dst, localeId)();
}

void exportGenericString(Zero4ScriptReader::ResultString& str,
                         std::string prefix, bool omitEmpty = false) {
  if (omitEmpty && (str.str.size() <= 0)) return;
  
  std::string outName = prefix + str.id + ".bin";
  TFileManip::createDirectoryForFile(outName);
  
  TBufStream ofs;
  ofs.writeString(str.str);
  
  if (!str.propertyIsTrue("noTerminator")) {
    ofs.writeu8(Zero4MsgConsts::opcode_end);
  }
  
  if (ofs.size() <= 0) return;
  
  ofs.save(outName.c_str());
}

void exportGenericRegion(Zero4ScriptReader::ResultCollection& results,
                         std::string prefix, bool omitEmpty = false) {
  for (Zero4ScriptReader::ResultCollection::iterator it = results.begin();
       it != results.end();
       ++it) {
//    if (it->str.size() <= 0) continue;
    Zero4ScriptReader::ResultString& str = *it;
    exportGenericString(str, prefix, omitEmpty);
  }
}

/*void exportGenericSceneString(Zero4ScriptReader::ResultString& str,
                         std::string prefix, bool omitEmpty = false) {
  if (omitEmpty && (str.str.size() <= 0)) return;
  
  std::string outName = prefix + str.id + ".bin";
  TFileManip::createDirectoryForFile(outName);
  
  TBufStream ofs;
  ofs.writeString(str.str);
  
  if (!str.propertyIsTrue("noTerminator")) {
    ofs.writeu8(0xFF);
  }
  
  if (ofs.size() <= 0) return;
  
  ofs.save(outName.c_str());
}

void exportGenericSceneRegion(Zero4ScriptReader::ResultCollection& results,
                         std::string prefix, bool omitEmpty = false) {
  for (Zero4ScriptReader::ResultCollection::iterator it = results.begin();
       it != results.end();
       ++it) {
//    if (it->str.size() <= 0) continue;
    Zero4ScriptReader::ResultString& str = *it;
    exportGenericSceneString(str, prefix, omitEmpty);
  }
}*/

void exportGenericRegionWithIncludes(
    Zero4ScriptReader::ResultCollection& results,
    std::string prefix,
    std::ostream& includeOfs, std::string mapIncNameStr,
    std::string nameSuffix) {
  exportGenericRegion(results, prefix);
  
  int index = 0;
  for (auto& str: results) {
    outputPrefixedStringInclude(includeOfs, mapIncNameStr,
      nameSuffix + TStringConversion::intToString(index++),
      prefix + str.id + ".bin");
  }
}

void exportGenericRegionMap(Zero4ScriptReader::NameToRegionMap& results,
                         std::string prefix) {
  for (auto it: results) {
    exportGenericRegion(it.second.strings, prefix);
  }
}

std::string strIdToLabel(std::string str) {
  for (auto& c: str) if (c == '-') c = '_';
  return str;
}

void exportMainTextOverwriteInclude(Zero4ScriptReader::ResultCollection& results,
                         std::string outPrefix) {
  std::ostringstream overwriteOss;
  std::ostringstream dataOss;
  
  int overwriteSectionNum = 0;
  for (auto& str: results) {
//    if (it->str.size() <= 0) continue;
    
    int origPos = str.originalOffset;
    std::string labelName = strIdToLabel(str.id);
    
    TBufStream data;
    data.writeString(str.str);
    if (!str.propertyIsTrue("noTerminator")) {
      data.writeu8(Zero4MsgConsts::opcode_end);
    }
    data.seek(0);
    
    if (data.size() <= 0) continue;
    
    overwriteOss << ".section \"maintext overwrite include "
      << overwriteSectionNum
      << "\" BANK metabank_maintext SLOT 0"
      << " ORGA " << getHexWordNumStr(origPos + 0x4000)
      << " FORCE" << std::endl;
    overwriteOss << "  text_jumpToPossiblyNewLoc"
      << " " << labelName
      << std::endl;
    overwriteOss << ".ends" << std::endl;
    overwriteOss << std::endl;
    
/*    dataOss << ".section \"maintext data include "
      << overwriteSectionNum
      << "\" SLOT 0"
      << " SEMISUPERFREE BANKS 1-0" << std::endl;
    dataOss << "  " << labelName << ":" << std::endl;*/
    
    dataOss << "makeSemiSuperfreeSection \"maintext data include "
      << overwriteSectionNum
      << "\""
      << ",metabank_newtext,metabank_maintext"
      << std::endl;
    dataOss << "  " << labelName << ":" << std::endl;
    
    binToDcb(data, dataOss);
    dataOss << ".ends" << std::endl;
    dataOss << std::endl;
    
    ++overwriteSectionNum;
  }
  
  {
    TBufStream ofs;
    ofs.writeString(overwriteOss.str());
    std::string outName(outPrefix + "_overwrite.inc");
    TFileManip::createDirectoryForFile(outName);
    ofs.save(outName.c_str());
  }
  
  {
    TBufStream ofs;
    ofs.writeString(dataOss.str());
    std::string outName(outPrefix + "_data.inc");
    TFileManip::createDirectoryForFile(outName);
    ofs.save(outName.c_str());
  }
}

void mergeIntoRegion(Zero4ScriptReader::ResultRegion& dst,
                     const Zero4ScriptReader::ResultRegion& src) {
  for (const auto& str: src.strings) {
    dst.strings.push_back(str);
  }
}

void mergeIntoRegionIfExists(Zero4ScriptReader::ResultRegion& dst,
                             const Zero4ScriptReader::NameToRegionMap& regions,
                             std::string regionName) {
  auto findIt = regions.find(regionName);
  if (findIt == regions.end()) return;
  mergeIntoRegion(dst, findIt->second);
}





//==============================================================
// main
//==============================================================

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cout << "Zero Yon Champ script builder" << endl;
    cout << "Usage: " << argv[0] << " [inprefix] [outprefix] [locale]"
      << endl;
    return 0;
  }
  
//  string infile(argv[1]);
  string inPrefix(argv[1]);
  string outPrefix(argv[2]);
  string localeId(argv[3]);

//  tableStd.readUtf8("table/zero4_en.tbl");
  
  //=====
  // read script
  //=====
  
  Zero4ScriptReader::NameToRegionMap scriptResults;
  
  readScript((inPrefix + "spec.txt"), scriptResults, localeId);
  
  //=====
  // read files to be updated
  //=====
  
/*  TBufStream isoIfs;
  isoIfs.open("tenma_02_build.iso");
  
  TBufStream battleIfs;
  battleIfs.open("out/base/battle_1D.bin");*/
  
  //=====
  // compress
  //=====
  
/*    {
      Zero4ScriptReader::NameToRegionMap allStrings;
      
      std::vector<Zero4ScriptReader::NameToRegionMap*> allSrcPtrs;
      allSrcPtrs.push_back(&scriptResults);
      
      // merge everything into one giant map for compression
      mergeResultMaps(allSrcPtrs, allStrings);
      
      #if BUILD_COMPRESSION_DICTIONARY
      {
        // compress
        generateCompressionDictionary(
          allStrings, outPrefix + "script_dictionary.bin");
      }
      #else
      {
        TBufStream dictIfs;
        dictIfs.open((outPrefix + "script_dictionary.bin").c_str());
        
        int index = 0;
        while (!dictIfs.eof()) {
          int symbol = index + textCharsEnd;
          std::string srcStr;
          srcStr += dictIfs.get();
          srcStr += dictIfs.get();
          applyDictionaryEntry(srcStr,
                               allStrings,
                               std::string() + (char)symbol);
          ++index;
        }
      }
      #endif
      
      // restore results from merge back to individual containers
      unmergeResultMaps(allStrings, allSrcPtrs);
    }*/
  
  //=====
  // export
  //=====
  
  exportMainTextOverwriteInclude(scriptResults.at("main").strings,
    "asm/gen/maintext");
  
  exportGenericRegion(scriptResults.at("bank01").strings,
    "out/script/bank01/");
  
  exportGenericRegion(scriptResults.at("specialname").strings,
    "out/script/specialname/");
  
  exportGenericRegion(scriptResults.at("names").strings,
    "out/script/names/");
  
  exportGenericRegion(scriptResults.at("credits").strings,
    "out/script/credits/");
  
  exportGenericRegion(scriptResults.at("new").strings,
    "out/script/new/");
  
/*  exportGenericRegion(scriptResults.at("main").strings,
    "out/script/main/");
  exportGenericRegion(scriptResults.at("areaname").strings,
    "out/script/areaname/");
  exportGenericSceneRegion(scriptResults.at("scene").strings,
    "out/script/scene/");
  
  exportGenericRegion(scriptResults.at("new_helplabel").strings,
    "out/script/new_helplabel/");
  exportGenericRegion(scriptResults.at("new_help").strings,
    "out/script/new_help/");
  exportGenericRegion(scriptResults.at("new_misc").strings,
    "out/script/new_misc/");
  
  exportEndingListStrings(
    scriptResults.at("new_endnames").strings,
    scriptResults.at("new_enddesc").strings,
    scriptResults.at("new_misc").strings.at(0),
    "out/script/new_endlist/");*/
  
  return 0;
}
