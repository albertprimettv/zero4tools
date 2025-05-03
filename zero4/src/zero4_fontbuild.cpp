#include "util/TGraphic.h"
#include "util/TPngConversion.h"
#include "util/TIniFile.h"
#include "util/TBufStream.h"
#include "util/TOfstream.h"
#include "util/TIfstream.h"
#include "util/TStringConversion.h"
#include "util/TBitmapFont.h"
#include "util/TFileManip.h"
#include <iostream>
#include <vector>

using namespace std;
using namespace BlackT;

const static int charW = 8;
const static int charH = 8;

static int kerningBaseOffset = 0;
const static int minRunLength = 3;
const static int startingFontSlot = 0x20;
const static int slotsPerFont = 0x60;

void charToData(const TGraphic& src,
                int xOffset, int yOffset,
                TStream& ofs) {
  for (int j = 0; j < charH; j++) {
    int output = 0;
    
    int mask = 0x80;
    for (int i = 0; i < charW; i++) {
      int x = xOffset + i;
      int y = yOffset + j;
      TColor color = src.getPixel(x, y);
      
      if ((color.a() == TColor::fullAlphaTransparency)
          || (color.r() < 0x80)) {
        
      }
      else {
        output |= mask;
      }
      
      mask >>= 1;
    }
    
    ofs.writeu8(output);
  }
}

int getRunLength(const std::vector<int>& src, int baseIndex) {
  int length = 1;
  for (int i = baseIndex; i < src.size() - 1; i++) {
    if ((src[i] + 1) != src[i + 1]) break;
    ++length;
  }
  return length;
}

int getLocalGlyphIndex(TBitmapFont& font, const TThingyTable& symbolRemapTable,
    int outputCodepoint) {
  if (!symbolRemapTable.hasEntry(outputCodepoint)) return -1;
  
  std::string symbol = symbolRemapTable.getEntry(outputCodepoint);
  TThingyTable::MatchResult matchResult
    = font.getGlyphSymbolTable().matchTableEntry(symbol);
  if (matchResult.id == -1) return -1;
  
  return matchResult.id;
}

int swapWordEnd(int value) {
//  return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
  return value;
}

void makeKerningList(TBitmapFont& font, const TThingyTable& symbolRemapTable,
    TStream& indexOfs, TStream& dataOfs) {
  // converts the kerning data to a sparse matrix format.
  // format of output is as follows:
  // - index block, consisting of (numChars) little-endian halfwords.
  //   if an entry is -1 (0xFFFF), the character is not kerned.
  //   otherwise, it is an offset from the beginning of the data block
  //   giving the start position of the kerning entries for a character.
  // - data block, consisting of a series of sequential entries.
  //   at each entry's start, assume kerning to be -1.
  //   each following byte is the ID of a character for which this character,
  //   if preceded by the IDed character, should be kerned by -1.
  //   the following special values control processing:
  //   - 0xFX = add this value (signed byte) to the kerning.
  //            the entries that follow now use this new kerning.
  //   - 0xF0 = end of entry; stop processing
  //   - 0xEX = sequential incremental run:
  //            repeat the following byte (X+2) times, incrementing each time.
  //            e.g. "E2 01" means "01 02 03 04 05"
  //   if end of entry is reached without finding the target character,
  //   assume a kerning of 0.
  
//  TBufStream kerningIfs;
//  font.exportKerningMatrix(kerningIfs);
//  for (int j = 0; j < font.numFontChars(); j++) {
  for (int j = startingFontSlot; j < startingFontSlot + slotsPerFont; j++) {
    int indexOuter = getLocalGlyphIndex(font, symbolRemapTable, swapWordEnd(j));
    if (indexOuter == -1) {
      indexOfs.writeu16le(0xFFFF);
      continue;
    }
    
    std::map<int, std::vector<int> > kerningToPrevIdList;
    bool hasNonzeroEntry = false;
//    for (int i = 0; i < font.numFontChars(); i++) {
    for (int i = startingFontSlot; i < startingFontSlot + slotsPerFont; i++) {
      int indexInner = getLocalGlyphIndex(font, symbolRemapTable, swapWordEnd(i));
      if (indexInner == -1) {
        continue;
      }
      
      int second = indexOuter;
      int first = indexInner;
      
      int kerning = font.getKerning(first, second);
      if (kerning != 0) hasNonzeroEntry = true;
      
//      if (first == 0x3D && second == 0x42)
//        std::cerr << "test kerning: " << kerning << std::endl;
      
      // HACK: the way we're processing the characters means that
      // any character that doesn't have kerning totally disabled
      // will have a kerning of -1 or lower, with -1 the most common
      // value.
      // to make use of this, the numbers we input are offset by 1.
      // the expectation is that the code that uses this table will
      // have its own specific blacklist for the non-kerned characters
      // so that it ignores them, and for anything else, it will
      // subtract 1 from the returned kerning value.
      // (note: blacklist can probably be implemented by checking whether
      // previous character has a 0xFFFF entry in the index)
//      kerning += kerningBaseOffset;
//      kerningToPrevIdList[kerning].push_back(first);
      kerningToPrevIdList[kerning].push_back(i);
    }
    
    // if all entries zero, do not generate a data entry
    // and flag index entry as nonexistent
    if (!hasNonzeroEntry) {
      // mark as empty
      indexOfs.writeu16le(0xFFFF);
      continue;
    }
    
    indexOfs.writeu16le(dataOfs.tell());
    
//    int lastNonemptyKerning = -1;
    int lastNonemptyKerning = -kerningBaseOffset - 1;
    for (int i = -kerningBaseOffset - 1; i >= -(kerningBaseOffset + 16); --i) {
      auto findIt = kerningToPrevIdList.find(i);
      if (findIt == kerningToPrevIdList.end()) {
        continue;
      }
      
      int gap = lastNonemptyKerning - i;
      lastNonemptyKerning = i;
      // add "next kerning" commands until current target reached
//      for (int k = 0; k < gap; k++) {
//        dataOfs.writeu8(0xFE);
//      }
      if (gap > 0) {
        dataOfs.writes8(-gap);
      }
      
      std::vector<int>& prevIdList = findIt->second;
//      for (auto value: prevIdList) {
//        dataOfs.writeu8(value);
//      }
      
      for (int k = 0; k < prevIdList.size(); ) {
        int runLen = getRunLength(prevIdList, k);
        
//        if (j == 0x42) {
//          std::cerr << std::hex << k << ": " << (int)prevIdList[k]
//            << " " << runLen << std::endl;
//        }
        
        if (runLen >= minRunLength) {
          if (runLen >= (0xF + minRunLength)) runLen = (0xF + minRunLength);
          dataOfs.writeu8(0xE0 | (runLen - minRunLength));
          dataOfs.writeu8(prevIdList[k]);
          k += runLen;
        }
        else {
          dataOfs.writeu8(prevIdList[k]);
          ++k;
        }
      }
    }
    
    // write end-of-entry marker
    dataOfs.writeu8(0xF0);
  }
}

struct PreppedFontData {
  TBufStream fontOfs;
  TBufStream widthOfs;
  TBufStream kerningIndexOfs;
  TBufStream kerningDataOfs;
  // HACK: we need to be able to look up the width of the space character
  // quickly for the word wrapping algorithm, so output that specially
  TBufStream widthOfSpaceOfs;
  TBufStream widthOfZeroOfs;
  
  void output(std::string outBase) {
    string outFontFileName = outBase + "font.bin";
    string outWidthFileName = outBase + "width.bin";
    string outKerningIndexFileName = outBase + "kerning_index.bin";
    string outKerningDataFileName = outBase + "kerning_data.bin";
//    string outKerningMatrixFileName = outBase + "kerning_matrix.bin";
    string outWidthOfSpaceFileName = outBase + "width_of_space.bin";
    string outWidthOfZeroFileName = outBase + "width_of_zero.bin";
    
    TFileManip::createDirectoryForFile(outFontFileName);
    
    fontOfs.seek(0);
    fontOfs.save(outFontFileName.c_str());
    widthOfs.seek(0);
    widthOfs.save(outWidthFileName.c_str());
    kerningIndexOfs.seek(0);
    kerningIndexOfs.save(outKerningIndexFileName.c_str());
    kerningDataOfs.seek(0);
    kerningDataOfs.save(outKerningDataFileName.c_str());
    widthOfSpaceOfs.seek(0);
    widthOfSpaceOfs.save(outWidthOfSpaceFileName.c_str());
    widthOfZeroOfs.seek(0);
    widthOfZeroOfs.save(outWidthOfZeroFileName.c_str());
  }
  
  void offsetKerningIndex(int amount) {
    kerningIndexOfs.seek(0);
    while (!kerningIndexOfs.eof()) {
      int value = kerningIndexOfs.readu16le();
      if (value == 0xFFFF) continue;
      
      kerningIndexOfs.seekoff(-2);
      kerningIndexOfs.writeu16le(value + amount);
    }
  }
};

void prepFontData(std::string fontName, PreppedFontData& dst,
    const TThingyTable& symbolRemapTable) {
  TBitmapFont font;
  font.load(fontName);
  
//  const TThingyTable& glyphIndexTable
//    = font.getGlyphSymbolTable();
  
  for (int i = startingFontSlot; i < startingFontSlot + slotsPerFont; i++) {
    int localGlyphIndex
      = getLocalGlyphIndex(font, symbolRemapTable, swapWordEnd(i));
    if (localGlyphIndex == -1) {
      for (int i = 0; i < ((charW / 8) * charH); i++) dst.fontOfs.writeu8(0);
      dst.widthOfs.writeu8(0);
      continue;
    }
    
    const TBitmapFontChar& fontChar = font.fontChar(localGlyphIndex);
    int width = fontChar.advanceWidth;
    
    charToData(fontChar.grp, 0, 0, dst.fontOfs);
    dst.widthOfs.writeu8(width);
  }
  
  makeKerningList(font, symbolRemapTable, dst.kerningIndexOfs, dst.kerningDataOfs);
  
  // offset kerning index offsets by size of index,
  // so we can simply add to the base position
  dst.offsetKerningIndex(dst.kerningIndexOfs.size());
  
  // write space width
  {
    TThingyTable::MatchResult matchResult
      = font.getGlyphSymbolTable().matchTableEntry(" ");
    if (matchResult.id != -1) {
      const TBitmapFontChar& fontChar = font.fontChar(matchResult.id);
      dst.widthOfSpaceOfs.writeu8(fontChar.advanceWidth);
    }
  }
  
  // write zero width
  {
    TThingyTable::MatchResult matchResult
      = font.getGlyphSymbolTable().matchTableEntry("0");
    if (matchResult.id != -1) {
      const TBitmapFontChar& fontChar = font.fontChar(matchResult.id);
      dst.widthOfZeroOfs.writeu8(fontChar.advanceWidth);
    }
  }
}

void outputGenericFont(std::string name, std::string localeId,
    const TThingyTable& table_std, std::string outBase) {
  PreppedFontData fontData_std;
  prepFontData(std::string("font/") + localeId + "/" + name + "/", fontData_std,
    table_std);
  fontData_std.output(outBase + name + "/");
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cout << "Zero Yon Champ font builder" << endl;
    cout << "Usage: " << argv[0] << " <outbase> <locale>"
      << endl;
    
    return 0;
  }
  
  string outBase(argv[1]);
  string localeId(argv[2]);
  
  TThingyTable table_std;
  table_std.readUtf8(std::string("table/") + localeId + "/zero4.tbl");
  TThingyTable table_ascii;
  table_ascii.readUtf8(std::string("table/") + localeId + "/ascii.tbl");
  
/*  PreppedFontData fontData_std;
  prepFontData(std::string("font/") + localeId + "/std/", fontData_std,
    table_std);
  fontData_std.output(outBase + "std/");*/
  
  outputGenericFont("std", localeId, table_std, outBase);
  outputGenericFont("monospace", localeId, table_std, outBase);
  outputGenericFont("monospace_it", localeId, table_std, outBase);
  outputGenericFont("italic", localeId, table_std, outBase);
  outputGenericFont("bold", localeId, table_std, outBase);
  outputGenericFont("asciistrict", localeId, table_ascii, outBase);
  
/*  PreppedFontData fontData_std;
  prepFontData(std::string("font/") + localeId + "/std/", fontData_std,
    table_std);
  PreppedFontData fontData_narrow;
  prepFontData(std::string("font/") + localeId + "/narrow/", fontData_narrow,
    table_std);
  PreppedFontData fontData_bold;
  prepFontData(std::string("font/") + localeId + "/bold/", fontData_bold,
    table_std);
  PreppedFontData fontData_italic;
  prepFontData(std::string("font/") + localeId + "/italic/", fontData_italic,
    table_std);
  
  std::vector<PreppedFontData> preppedFonts;
  preppedFonts.push_back(fontData_std);
  preppedFonts.push_back(fontData_narrow);
  preppedFonts.push_back(fontData_bold);
  preppedFonts.push_back(fontData_italic);
  
  // offset index tables by size of preceding data tables
  for (int i = 0; i < preppedFonts.size(); i++) {
    PreppedFontData& fontData = preppedFonts[i];
    for (int j = 0; j < i; j++) {
      PreppedFontData& otherFontData = preppedFonts[j];
      fontData.offsetKerningIndex(otherFontData.kerningDataOfs.size());
    }
  }
  
  // merge data
  {
    PreppedFontData mergedFontData;
    
    TBufStream& fontOfs = mergedFontData.fontOfs;
    TBufStream& widthOfs = mergedFontData.widthOfs;
    TBufStream& kerningIndexOfs = mergedFontData.kerningIndexOfs;
    TBufStream& kerningDataOfs = mergedFontData.kerningDataOfs;
    
    for (auto& fontData: preppedFonts) {
      fontData.fontOfs.seek(0);
      fontOfs.writeFrom(
        fontData.fontOfs, fontData.fontOfs.size());
      
      fontData.widthOfs.seek(0);
      widthOfs.writeFrom(
        fontData.widthOfs, fontData.widthOfs.size());
      
      fontData.kerningIndexOfs.seek(0);
      kerningIndexOfs.writeFrom(
        fontData.kerningIndexOfs, fontData.kerningIndexOfs.size());
      
      fontData.kerningDataOfs.seek(0);
      kerningDataOfs.writeFrom(
        fontData.kerningDataOfs, fontData.kerningDataOfs.size());
    }
    
    mergedFontData.output(outBase + "universal/");
  }
  
  preppedFonts.at(0).output(outBase + "std/");
  preppedFonts.at(1).output(outBase + "narrow/");
  preppedFonts.at(2).output(outBase + "bold/");
  preppedFonts.at(3).output(outBase + "italic/");*/
  
  return 0;
}
