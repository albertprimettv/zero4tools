#include "zero4/Zero4ScriptGenStream.h"
#include "zero4/Zero4TranslationSheet.h"
#include "zero4/Zero4MsgConsts.h"
#include "zero4/Zero4MsgDismException.h"
#include "util/TBufStream.h"
#include "util/TIfstream.h"
#include "util/TOfstream.h"
#include "util/TGraphic.h"
#include "util/TStringConversion.h"
#include "util/TPngConversion.h"
#include "util/TCsv.h"
#include "util/TSoundFile.h"
#include "util/TFileManip.h"
#include "util/TFreeSpace.h"
#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using namespace BlackT;
using namespace Pce;

struct StringTableIdentifier {
  enum Source {
    src_none,
    src_buttonInteraction,
    src_bank3Table,
    src_hardcoded
  };
  
  Source src;
  int tableNum;
  int subItemNum;
  
  StringTableIdentifier()
    : src(src_none),
      tableNum(-1),
      subItemNum(-1) { }
  
  StringTableIdentifier(Source src__, int subItemNum__ = -1)
    : src(src__),
      tableNum(-1),
      subItemNum(subItemNum__) { }
};

TThingyTable tableStd;
TThingyTable tableName;

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

void seekToLoromPos(TStream& rom, int targetAddr) {
  rom.seek((((targetAddr & 0xFF0000) >> 16) * 0x8000)
    + ((targetAddr & 0xFFFF) - 0x8000));
}

void seekToLorom16BitPos(TStream& rom, int bank, int addrLo) {
  seekToLoromPos(rom, (bank << 16) | addrLo);
}

std::string labelNameSubstitute(std::string input) {
  for (auto& c: input) if (c == '-') c = '_';
  return input;
}

void addGenericString(TStream& src, int offset, Zero4ScriptGenStream& dst,
                    TThingyTable& table,
                    std::string name = "") {
  src.seek(offset);
  dst.addGenericString(src, table, name);
}

void addGenericSeqStrings(TStream& src, int offset, int count,
                    Zero4ScriptGenStream& dst,
                    TThingyTable& table,
                    std::string name = "") {
  src.seek(offset);
  for (int i = 0; i < count; i++) {
//    std::cerr << i << std::endl;
    dst.addGenericString(src, table, name);
  }
}
        
void outputIncludeAndOverwrite(int refBankNum, int refPtrLo, std::string strId,
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
}

/*struct StringTableNumAndName {
  int num;
  std::string name;
};

StringTableNumAndName stringNumAndNameArray[] = {
  { 0x00, "Intro" },
  { 0x01, "Prologue" },
  { 0x02, "Start of the Tale" },
  { 0x03, "System" },
  { 0x04, "Tower of Druaga exterior, Babylim Harbor" },
  { 0x05, "Tower of Druaga: generic rooms" },
  { 0x06, "Tower of Druaga: Front" },
  { 0x07, "Tower of Druaga: Slime Room" },
  { 0x08, "Tower of Druaga: Entrance Hall" },
  { 0x09, "Tower of Druaga: Snake Room" },
  { 0x0A, "Tower of Druaga: Vampire Room" },
  { 0x0B, "Tower of Druaga: Roper Room" },
  { 0x0C, "Tower of Druaga: Bat Room" },
  { 0x0D, "Tower of Druaga: Treasure Room" },
  { 0x0E, "Tower of Druaga: Quox's Room" },
  { 0x0F, "Stormy Mountain cave: entrance" },
  { 0x10, "Stormy Mountain cave: interior" },
  { 0x11, "Stormy Mountain cave: exit" },
  { 0x12, "Succubus' Cave: front" },
  { 0x13, "Succubus' Cave: entrance" },
  { 0x14, "Succubus' Cave: interior" },
  { 0x15, "Raman's Temple: front" },
  { 0x16, "Raman's Temple: entrance" },
  { 0x17, "Raman's Temple: throne room" },
  { 0x18, "Stormy Mountain" },
  { 0x19, "Weather Man's Hut" },
  { 0x1A, "Underworld: common text, Desert of Death" },
  { 0x1B, "Underworld: Gate of Judgment" },
  { 0x1C, "Underworld: Blue Gate" },
  { 0x1D, "Underworld: Red Gate" },
  { 0x1E, "Underworld: Plains of Niavana" },
  { 0x1F, "Underworld: Nergal's Temple" },
  { 0x20, "Underworld: Plains of Gihenna" },
  { 0x21, "Underworld: Front of Devil's Manor" },
  { 0x22, "Underworld: Druaga's Room" },
  { 0x23, "Common text for Cliffs of Flame/Hanging Garden" },
  { 0x24, "Cliffs of Flame bridge" },
  { 0x25, "Cliffs of Flame clearing" },
  { 0x26, "Cliffs of Flame stone" },
  { 0x27, "Cliffs of Flame eagles" },
  { 0x28, "Hanging Garden: Klinta battles" },
  { 0x29, "Hanging Garden: Tarim battles" },
  { 0x2A, "Hanging Garden: Garru's throne room" },
  { 0x2B, "Sumar: Exterior" },
  { 0x2C, "Sumar: Market" },
  { 0x2D, "Sumar: Tavern" },
  { 0x2E, "Sumar: Hatari's shop" },
  { 0x2F, "Sumar: Central Plaza" },
  { 0x30, "Sumar: Worldly Man's House" },
  { 0x31, "Sumar: Castle walls" },
  { 0x32, "Sumar: Castle gate" },
  { 0x33, "Sumar: Coachyard" },
  { 0x34, "Arena: Antechamber" },
  { 0x35, "Arena" },
  { 0x36, "Sumar Castle: Entrance" },
  { 0x37, "Sumar Castle: Throne room" },
  { 0x38, "Babylim: Exterior" },
  { 0x39, "Mahaal's house (quiz game)" },
  { 0x3A, "Fortune-Telling House" },
  { 0x3B, "Ziggurat" },
  { 0x3C, "Governmental Room (Sargon conversations)" },
  { 0x3D, "Ishtar's Temple: Stone of the Heavens" },
  { 0x3E, "Ishtar's Temple" },
  { 0x3F, "Heaven: common text" },
  { 0x40, "Heaven: Throne of Anu" },
  { 0x41, "Wandering Woods" },
  { 0x42, "Credits" },
  { 0x43, "Endings 0x00-0x17: Gil becomes king + something else" },
  { 0x44, "Ending 0x18: Gil gets assassinated by Anshar" },
  { 0x45, "Ending 0x19: Gil somehow ends up in Isis with amnesia" },
  { 0x46, "Ending 0x1A: Gil and Ki are replaced by their children(?)" },
  { 0x47, "Ending 0x1B: Gil becomes god of peace" },
  { 0x48, "Ending 0x1C: Gil founds a democracy" },
  { 0x49, "Ending 0x1D: Gil becomes a sage" },
  { 0x4A, "Ending 0x1E: Gil becomes god of war" },
  { 0x4B, "Ending 0x1F: Gil and Ki swap bodies" },
  { 0x4C, "Ending 0x20: Gil becomes priest of gods and demons" },
  { 0x4D, "Ending 0x21: Gil explores the world" },
  { 0x4E, "Ending 0x22: Gil becomes emperor of Uruk" },
  { 0x4F, "Ending 0x23: Gil dies, the end" },
  { 0x50, "Ending 0x24: Gil and his many friends become the new divine peace squad" },
  { 0x51, "Ending 0x25: Gil appoints all his new friends as ministers" },
  { 0x52, "Ending 0x26: Gil becomes the god of battle" },
  { 0x53, "Ending 0x27: Gil is turned to stone for hubris" },
  { 0x54, "Ending 0x28: Gil becomes a prophet" },
  { 0x55, "Ending 0x29: Gil becomes emperor of Sumar" },
  { 0x56, "Ending 0x2A: Gil resurrects everybody killed by Druaga" },
  { 0x57, "Ending 0x2B: Gil gets the gods to screw off, then starts conquering everybody" },
  { 0x58, "Ending 0x2C: Gil and Ki elope" },
  { 0x59, "Ending 0x2D: Gil becomes a hermit in the Tower of Druaga" },
  { 0x5A, "Ending 0x2E: Gil goes mad with power and takes over the world" },
  { 0x5B, "Ending 0x2F: Gil forms a military dictatorship" },
  { 0x5C, "Rajaf Village" },
  { 0x5D, "Labyrinth" },
  { 0x5E, "Narak Village" },
  { 0x5F, "Cavern of Elbruz" },
  { 0x60, "Tutorial" },
};*/

struct StringSectionInfo {
  enum Type {
    type_none,
    type_chapter,
    type_page
  };
  
  StringSectionInfo()
    : firstStringPos(-1),
      type(type_none) { }
  StringSectionInfo(int firstStringPos__, std::string name__ = "",
      Type type__ = type_chapter)
    : firstStringPos(firstStringPos__),
      name(name__),
      type(type__) { }
  
  int firstStringPos;
  std::string name;
  Type type;
};

StringSectionInfo stringSectionInfoArray[] = {
  StringSectionInfo(0x0, "Intro"),
  StringSectionInfo(0x36D, "Magazine and some menus"),
//    StringSectionInfo(0x36D, "Miscellaneous", StringSectionInfo::type_page),
//    StringSectionInfo(0x36D, "Race schedule menu", StringSectionInfo::type_page),
//    StringSectionInfo(0x7B7, "Seminar", StringSectionInfo::type_page),
  StringSectionInfo(0xD77, "Amateur race"),
  StringSectionInfo(0xE6A, "Auto shop"),
    StringSectionInfo(0xE6A, "Basic interface", StringSectionInfo::type_page),
    StringSectionInfo(0xFA6, "Parts and tune-up descriptions", StringSectionInfo::type_page),
    StringSectionInfo(0x1334, "Final race prep", StringSectionInfo::type_page),
    StringSectionInfo(0x16FA, "Parts and tune-up menus", StringSectionInfo::type_page),
  StringSectionInfo(0x17A6, "Car dealership"),
  StringSectionInfo(0x1B89, "Car info menu"),
    StringSectionInfo(0x1B89, "Info window", StringSectionInfo::type_page),
    StringSectionInfo(0x23D1, "Parts and upgrades list", StringSectionInfo::type_page),
    StringSectionInfo(0x24DC, "Miscellaneous", StringSectionInfo::type_page),
  StringSectionInfo(0x2520, "Security guard job"),
  StringSectionInfo(0x2925, "Sound test"),
  StringSectionInfo(0x2B43, "Main menus and displays, common menu components"),
  StringSectionInfo(0x2C61, "Miscellaneous dialogue"),
  StringSectionInfo(0x2D33, "Short car names"),
  StringSectionInfo(0x2DE6, "Item names"),
  StringSectionInfo(0x2EAA, "Auto shop search scenes"),
  StringSectionInfo(0x33CD, "Ending cutscene"),
  StringSectionInfo(0x3911, "Game over cutscene"),
  StringSectionInfo(0x3A68, "Arcade job"),
  StringSectionInfo(0x3D60, "King introduction cutscene"),
  StringSectionInfo(0x43A9, "Takahiro (shitennou 1) race"),
  StringSectionInfo(0x488A, "Bank cutscene"),
  StringSectionInfo(0x4BF5, "Kayo/Mami at apartment 1"),
  StringSectionInfo(0x4DD9, "Kayo/Mami at apartment 2"),
  StringSectionInfo(0x5059, "Mami (shitennou 3) race"),
  StringSectionInfo(0x53CD, "Time limit cutscene 1"),
  StringSectionInfo(0x568A, "Driver's license renewal"),
    StringSectionInfo(0x568A, "Main dialogue", StringSectionInfo::type_page),
    StringSectionInfo(0x5961, "Quiz questions", StringSectionInfo::type_page),
  StringSectionInfo(0x617F, "Masaomi (shitennou 2) intro/race"),
  StringSectionInfo(0x6813, "Generic races"),
//  StringSectionInfo(0x6935, "Generic races"),
  StringSectionInfo(0x6ACF, "Imamura visits"),
  StringSectionInfo(0x6D91, "Ryouichi (shitennou 4) race"),
  StringSectionInfo(0x7012, "Ordered item delivery"),
  StringSectionInfo(0x7364, "Zero Yon agency calls"),
  StringSectionInfo(0x75B2, "Final race"),
  StringSectionInfo(0x7B34, "Miscellaneous"),
    StringSectionInfo(0x7B34, "Arcade race game results", StringSectionInfo::type_page),
    StringSectionInfo(0x7B65, "Name/password entry", StringSectionInfo::type_page),
    StringSectionInfo(0x7BFD, "Vs. mode menus, save management", StringSectionInfo::type_page),
    
};

struct StringSizeInfo {
  StringSizeInfo()
    : windowX(-1),
      windowY(-1),
      windowW(-1),
      windowH(-1) { }
  
  int windowX;
  int windowY;
  int windowW;
  int windowH;
};

int main(int argc, char* argv[]) {
  TFileManip::createDirectory("script/orig");
  TFileManip::createDirectory("asm/gen");
  
  tableStd.readUtf8("table/orig/zero4.tbl");
  tableName.readUtf8("table/orig/zero4_name.tbl");
  
  Zero4TranslationSheet sheetAll;
  
  TBufStream rom;
  rom.open("zero4.pce");
  
  TBufStream textIfs;
  rom.seek(0x10000);
  textIfs.writeFrom(rom, 0x8000);
  
//  std::ofstream ofsStringInclude("asm/gen/strings_include.inc");
//  std::ofstream ofsStringOverwrite("asm/gen/strings_overwrite.inc");
  
  std::map<int, std::vector<StringSectionInfo> > stringPosToSectionInfo;
  for (unsigned int i = 0;
       i < sizeof(stringSectionInfoArray) / sizeof(StringSectionInfo);
       i++) {
    StringSectionInfo& entry = stringSectionInfoArray[i];
    stringPosToSectionInfo[entry.firstStringPos].push_back(entry);
  }
  
  //=============================================
  // read window sizing info from gamescripts
  //=============================================
  
  std::map<int, StringSizeInfo> stringSizes;
  
  {
    TBufStream ifs;
    rom.seek(0x20000);
    ifs.writeFrom(rom, 0x8000);
    
    std::vector<StringSizeInfo> sizeInfo(8);
    for (int i = 0; i < ifs.size(); ) {
      ifs.seek(i);
      int op = ifs.readu8();
      
      // gamescript op 0x1C
      //
      // open a window
      // - 1b target slot
      // - 1b y
      // - 1b x
      // - 1b h
      // - 1b w
      if ((op == 0x1C) && (ifs.remaining() >= 5)) {
        int slot = ifs.readu8();
        int y = ifs.readu8();
        int x = ifs.readu8();
        int h = ifs.readu8();
        int w = ifs.readu8();
        
        // validate
        if (slot >= 8) goto not1C;
        // windows can occupy off-screen positions that are then shown via
        // scrolling, so allow that to an extent
        if (x >= 0x40) goto not1C;
        if (y >= 0x40) goto not1C;
        if (w > 32) goto not1C;
        if (h > 32) goto not1C;
        if (w < 4) goto not1C;
        if (h < 4) goto not1C;
        
        StringSizeInfo& currentInfo = sizeInfo.at(slot);
        
        currentInfo.windowX = x;
        currentInfo.windowY = y;
        currentInfo.windowW = w;
        currentInfo.windowH = h;
        
/*        if (i == 0x1296) {
          std::cerr << x << " " << y << " " << w << " " << h << std::endl;
          char c;
          std::cin >> c;
        }*/
        
        i = ifs.tell();
        continue;
      }
      not1C:
      
      // run textscript via direct pointer.
      // op 0x4C does the same via indirect pointer, but there's not much
      // we can do about that.
      // 
      // - 1b target slot num
      // - 2b pointer to target script
      if ((op == 0x1E) && (ifs.remaining() >= 3)) {
        int slot = ifs.readu8();
        int ptr = ifs.readu16le();
        
        // validate
        if (slot >= 8) goto not1E;
        
        StringSizeInfo& currentInfo = sizeInfo.at(slot);
        
        // don't use if not initialized
        if (currentInfo.windowW == -1) goto not1E;
        
        stringSizes[ptr - 0x4000] = currentInfo;
        
        i = ifs.tell();
        continue;
      }
      not1E:
      
      ++i;
    }
    
  }
  
  //=============================================
  // header
  //=============================================
  
  Zero4ScriptGenStream streamAll;

  streamAll.addAddFont("std", "zero4.tbl");
  streamAll.addAddFont("monospace", "zero4.tbl");
  streamAll.addAddFont("monospace_it", "zero4.tbl");
  streamAll.addAddFont("italic", "zero4.tbl");
  streamAll.addAddFont("bold", "zero4.tbl");
  streamAll.addAddFont("name", "zero4_name.tbl");
  streamAll.addAddFont("asciistrict", "ascii.tbl");
/*  streamAll.addAddFont("narrow", "zero4_font1.tbl");
  streamAll.addAddFont("bold", "zero4_font2.tbl");
  streamAll.addAddFont("italic", "zero4_font3.tbl");
  streamAll.addAddFont("scene", "zero4_scene.tbl");*/
  
  //=============================================
  // read strings from string tables
  //=============================================
  
//  std::map<int, int> stringToSize;
  
//  streamAll.addGenericLine("#OPENFONTKERNINGMATRIX(\"out/font/main/kerning_matrix.bin\")");
  
  streamAll.addSetBook("main", "Main text block");
  streamAll.addSetMetaRegion("main");
  
  streamAll.addSetRegion("main");
  
  streamAll.addSetFont("std");
  
  textIfs.seek(0);
  
  while (textIfs.tell() < 0x7E57) {
    int pos = textIfs.tell();
    
    // HACK: reset front/back page state where needed
/*    switch (pos) {
    case 0x2962:
      streamAll.usingFrontPage = false;
      break;
//    case :
//      streamAll.usingFrontPage = true;
//      break;
    default:
      break;
    }*/
    
    // HACK?
    streamAll.usingFrontPage = false;
    
    try {
      std::cerr << std::hex << pos << std::endl;
      std::string strId = std::string("maintext-")
          + TStringConversion::intToString(pos,
            TStringConversion::baseHex);
      
/*      auto it = stringSizes.find(pos);
      if (it != stringSizes.end()) {
        streamAll.addSetSize(
          (it->second.windowW - 2) * 8,
          (it->second.windowH - 2) / 2);
      }
      else {
        streamAll.addSetSize(-1, -1);
      }*/
      
      auto findIt = stringPosToSectionInfo.find(pos);
      if (findIt != stringPosToSectionInfo.end()) {
        for (const auto& entry: findIt->second) {
          std::string sectionId = std::string("textscript-section-")
            + TStringConversion::intToString(pos, TStringConversion::baseHex);
          std::string name = std::string("Textscript section ")
            + TStringConversion::intToString(pos, TStringConversion::baseHex);
          
          if (!entry.name.empty()) name = entry.name;
          
          if (entry.type == StringSectionInfo::type_chapter) {
            streamAll.addSetChapter(sectionId, name);
          }
          else if (entry.type == StringSectionInfo::type_page) {
            streamAll.addSetPage(sectionId, name);
          }
        }
      }
      
      streamAll.addScriptString(textIfs, tableStd, strId);
      
      if (pos == 0x1B89) {
        // HACK: remove [clear] command at start of 0x1B89.
        // this causes the car info window to be cleared whenever it's
        // changed, which isn't necessary and in fact introduces what
        // seems to be a race condition where, if the game were to lag
        // more (as the translation does), the car graphic in the window
        // would get erased.
        // also, the translation can't clear a box this large without
        // causing issues with the line interrupts used for the car
        // selection menu (we have more lag due to having to read back the
        // old window contents in order to deallocate any text tiles within).
        Zero4ScriptGenStreamEntry& entry = streamAll.entries.back();
        entry.subStrings[0].content = "";
//        if (entry.subStrings.size() == 0)
//        entry.subStrings.erase(entry.subStrings.begin());
      }
      
//      std::cerr << std::hex << "end: " << textIfs.tell() << std::endl;
    }
    catch (Pce::Zero4MsgDismException& e) {
      // retry at next byte position on failure
//      std::cerr << "failed: " << e.problem() << std::endl;
      textIfs.seek(pos + 1);
    }
    
    // de-optimize group selection menu
    if (pos == 0x36D) {
      textIfs.seek(0x378);
    }
    else if (pos == 0x378) {
      textIfs.seek(0x383);
    }
    else if (pos == 0x383) {
      textIfs.seek(0x38E);
    }
    else if (pos == 0x38E) {
      textIfs.seek(0x399);
    }
    else if (pos == 0x399) {
      textIfs.seek(0x3A4);
    }
    
    // ?
    if (pos == 0x1B17) {
      textIfs.seek(0x1B3B);
    }
    
    // ?
    if (pos == 0x1B5A) {
      textIfs.seek(0x1B78);
    }
    
    // de-optimize password menu
    if (pos == 0x7B75) {
      textIfs.seek(0x7B88);
    }
    
    // king introduction scene: recycled あーあ　こんなかおになっちまった・・・
    if (pos == 0x435A) {
      textIfs.seek(0x4383);
    }
    
    // TODO: there are undoubtedly more instances of substring recycling
  }
  
//  streamAll.addSetSize(189, 5);
//  streamAll.addSetFailOnBoxOverflow(true);
  
/*    seekToLorom16BitPos(rom, 0x08, pos);
    int ptrPos = rom.tell();
    int strPtr = rom.readInt(3, EndiannessTypes::little, SignednessTypes::nosign);
    seekToLoromPos(rom, strPtr);
    int strRomAddr = rom.tell();
    
    std::string strId = std::string("string-")
        + TStringConversion::intToString(ptrPos,
          TStringConversion::baseHex)
        + "-"
        + TStringConversion::intToString(strRomAddr,
          TStringConversion::baseHex);
    streamAll.addScriptString(rom, tableStd, strId);
    stringToSize[strRomAddr] = rom.tell() - strRomAddr;
    
    outputIncludeAndOverwrite(0x08, pos, strId,
      std::string("out/script/strings/"),
      ofsStringInclude, ofsStringOverwrite);
    
    pos += 3;*/
  
  //=============================================
  // bank01
  //=============================================
  
  {
    TBufStream textIfs;
    rom.seek(0x2000);
    textIfs.writeFrom(rom, 0x2000);
  
    streamAll.addSetBook("bank01", "Bank 01 strings");
    streamAll.addSetMetaRegion("bank01");
    
    streamAll.addSetRegion("bank01");
    
    streamAll.addSetSize(-1, -1);
    
    textIfs.seek(0xD5B9 - 0xC000);
    streamAll.addComment("'Censored' script data");
//    streamAll.usingFrontPage = false;
    streamAll.addScriptString(textIfs, tableStd, "censor-data");
    
    textIfs.seek(0xD740 - 0xC000);
    streamAll.addComment("Backup memory strings");
//    streamAll.usingFrontPage = false;
    for (int i = 0; i < 4; i++) {
      std::string id = std::string("backup-")
        + TStringConversion::intToString(i);
      streamAll.addScriptString(textIfs, tableStd, id);
    }
  }
  
  //=============================================
  // gamescripts
  //=============================================
  
  {
    TBufStream textIfs;
    rom.seek(0x20000);
    textIfs.writeFrom(rom, 0x8000);
  
    streamAll.addSetBook("gamescript", "Gamescript strings");
    streamAll.addSetMetaRegion("gamescript");
    
    streamAll.addSetSize(-1, -1);
    
    // just as a note: though you'll find various passwords on the internet
    // for the vs. mode to give you access to certain cars, the game has
    // no special password checks for that mode. all such "special" passwords
    // are really just normal passwords that happen to pass the game's
    // validation algorithm (five characters isn't hard to brute-force).
    // this is why some of them result in bugged cars.
    streamAll.addSetChapter("specialname", "Special names and passwords");
    streamAll.addSetRegion("specialname");
    
    int index = 0;
    while (true) {
      textIfs.seek(0xB53E - 0x4000 + (index * 2));
      int ptr = textIfs.readu16le();
      int offset = (ptr - 0x4000);
      textIfs.seek(offset);
      
      // table is terminated by a pointer to an entry beginning with 0x00.
      // note that we include this entry in our data to ensure it's represented
      // in the replacement table we generate.
      bool done = false;
      if (textIfs.peek() == 0x00) done = true;
      
      std::string id = std::string("specialname-")
        + TStringConversion::intToString(index);
      streamAll.addScriptString(textIfs, tableStd, id);
      
      if (done) break;
      
      ++index;
    }
    
    streamAll.addSetRegion("names");
    
//    streamAll.addSetChapter("cheatname", "Player's name if shop cheat used");
    streamAll.addComment("Default player name");
    streamAll.addSetFont("name");
    
    textIfs.seek(0xB248 - 0x4000);
    streamAll.addGenericString(textIfs, tableName, "default-name");
    
//    streamAll.addSetChapter("cheatname", "Player's name if shop cheat used");
    streamAll.addComment("Player's name if shop cheat used");
//    streamAll.addSetRegion("cheatname");
    streamAll.addSetFont("name");
    
    textIfs.seek(0x5555 - 0x4000);
    streamAll.addGenericString(textIfs, tableName, "shop-cheat-name");
    
    streamAll.addComment("Player's name if license test totally failed");
//    streamAll.addSetRegion("failname");
    streamAll.addSetFont("name");
    
    textIfs.seek(0x9495 - 0x4000);
    streamAll.addGenericString(textIfs, tableName, "license-fail-name");
  }
  
  //=============================================
  // credits
  //=============================================
  
  streamAll.addSetBook("credits", "Credits text");
  streamAll.addSetMetaRegion("credits");
  streamAll.addSetRegion("credits");
  
  streamAll.addSetFont("std");
  streamAll.addSetSize(-1, -1);

  // these are all sprites with custom graphics, so we just have to do them
  // by hand
  for (int i = 0; i < 50; i++) {
    std::string str;
    
    switch (i) {
    // frame 0x243
    case 0: str = "企画"; break;
    // frame 0x241
    case 1: str = "神長　豊"; break;
    // frame 0x248
    case 2: str = "脚本"; break;
    // frame 0x241
    case 3: str = "神長　豊"; break;
    // frame 0x255
    case 4: str = "プログラム"; break;
    // frame 0x23C
    case 5: str = "上嶋　満也"; break;
    // frame 0x25B
    case 6: str = "上坂　和幹"; break;
    // frame 0x24E
    case 7: str = "水林　準次"; break;
    // frame 0x258
    case 8: str = "高松　馨"; break;
    // frame 0x23A
    case 9: str = "グラフィック"; break;
    // frame 0x25C
    case 10: str = "三好　八太郎"; break;
    // frame 0x257
    case 11: str = "井伊　誠"; break;
    // frame 0x237
    case 12: str = "安倍　直子"; break;
    // frame 0x245
    case 13: str = "小滝　佳治"; break;
    // frame 0x253
    case 14: str = "音楽"; break;
    // frame 0x25A
    case 15: str = "佐々木　筑紫"; break;
    // frame 0x247
    case 16: str = "広報"; break;
    // frame 0x251
    case 17: str = "奈切　美香"; break;
    // frame 0x250
    case 18: str = "中茎　裕造"; break;
    // frame 0x238
    case 19: str = "アドバイザー"; break;
    // frame 0x23B
    case 20: str = "久米　秀夫"; break;
    // frame 0x239
    case 21: str = "遠藤　雄一郎"; break;
    // frame 0x24D
    case 22: str = "皆吉　清太郎"; break;
    // frame 0x23E
    case 23: str = "井関　靖典"; break;
    // frame 0x249
    case 24: str = "協力"; break;
    // frame 0x259 ("トヨタ自") + 0x23F/0x240 ("動車株式会社").
    // the sprite definition banks are almost completely full, so the most
    // generic components of the credits entries were presumably reused
    // to make everything fit.
    case 25: str = "トヨタ自動車株式会社"; break;
    // frame 0x252 + 0x23F/0x240
    case 26: str = "日産自動車株式会社"; break;
    // frame 0x24F + 0x23F/0x246/0x240
    case 27: str = "三菱自動車工業株式会社"; break;
    // frame 0x24A + 0x240
    case 28: str = "マツダ株式会社"; break;
    // frame 0x23D + 0x246/0x240
    case 29: str = "本田技研工業株式会社"; break;
    // frame 0x242
    case 30: str = "監督"; break;
    // frame 0x241
    case 31: str = "神長　豊"; break;
    // frame 0x254
    case 32: str = "プロデューサー"; break;
    // frame 0x24B
    case 33: str = "松木　吉彦"; break;
    // frame 0x244
    case 34: str = "小林　靖彦"; break;
    // frame 0x256
    case 35: str = "制作"; break;
    // frame 0x240 + 0x24C
    // (0x24C is the main component, 0x240 is shared with the auto companies)
    case 36: str = "株式会社メディアリング"; break;
/*    case 37: str = ""; break;
    case 38: str = ""; break;
    case 39: str = ""; break;
    case 40: str = ""; break;
    case 41: str = ""; break;
    case 42: str = ""; break;
    case 43: str = ""; break;
    case 44: str = ""; break;
    case 45: str = ""; break;
    case 46: str = ""; break;
    case 47: str = ""; break;
    case 48: str = ""; break;
    case 49: str = ""; break;*/
    default:
      
      break;
    }
    
    if (!str.empty()) {
      std::string id = std::string("credits-")
        + TStringConversion::intToString(i);
      std::map<std::string, std::string> propertyMap;
      propertyMap["noTerminator"] = "1";
      streamAll.addString(str, 0, 0, id, propertyMap);
    }
  }
  
  //=============================================
  // new strings for translation
  //=============================================
  
  streamAll.addSetBook("new", "New strings for translation");
  streamAll.addSetMetaRegion("new");
  streamAll.addSetRegion("new");
  streamAll.addSetFont("std");
  
  streamAll.addSetChapter("nameentry", "Name entry screen");
  streamAll.addSetFont("name");
  streamAll.addPlaceholderString("name-entry-layout", true);
  streamAll.addSetFont("std");
  streamAll.addPlaceholderStringSet("name-entry-new", 16, true);
  
  streamAll.addSetChapter("newmisc", "Miscellaneous new strings");
  streamAll.addComment("Replacement for recycled clear commands");
  streamAll.addPlaceholderString("recycled-clear-replacement");
  
/*  //=====
  // ending names
  //=====
  
  streamAll.addGenericLine("#SETAUTOINDENT(\"none\")");
  streamAll.addGenericLine("#SETPADANDCENTER(0, 189)");
  
  streamAll.addSetChapter("new_endnames", "Ending names");
  streamAll.addSetRegion("new_endnames");
  
  streamAll.addSetFont("std");
  streamAll.addSetSize(189, 1);
  streamAll.addSetFailOnBoxOverflow(true);
  
  streamAll.addPlaceholderStringSet("new-endnames", 48);
  
  streamAll.addGenericLine("#DISABLEPADANDCENTER()");
  
  //=====
  // ending descriptions
  //=====
  
  streamAll.addSetChapter("new_enddesc", "Ending descriptions");
  streamAll.addSetRegion("new_enddesc");
  
  streamAll.addSetFont("std");
  streamAll.addSetSize(189, 3);
  streamAll.addSetFailOnBoxOverflow(true);
  
  streamAll.addPlaceholderStringSet("new-enddesc", 48);
  
  //=====
  // help labels
  //=====
  
  streamAll.addSetChapter("new_helplabel", "Help text labels");
  streamAll.addSetRegion("new_helplabel");
  
  streamAll.addSetFont("std");
  streamAll.addSetSize(104, 1);
  streamAll.addSetFailOnBoxOverflow(true);
  
  streamAll.addPlaceholderString("new-helplabel-help", true);
  
  streamAll.addPlaceholderString("new-helplabel-about", true);
  streamAll.addPlaceholderString("new-helplabel-mechanics", true);
  streamAll.addPlaceholderString("new-helplabel-stats", true);
  streamAll.addPlaceholderString("new-helplabel-woods", true);
  streamAll.addPlaceholderString("new-helplabel-labyrinth", true);
  streamAll.addPlaceholderString("new-helplabel-sumar", true);
  streamAll.addPlaceholderString("new-helplabel-desert", true);
  streamAll.addPlaceholderString("new-helplabel-elbruz", true);
  streamAll.addPlaceholderString("new-helplabel-mahaal", true);
  streamAll.addPlaceholderString("new-helplabel-ziggurat", true);
  
  streamAll.addGenericLine("#SETAUTOINDENT(\"none\")");
  
  //=====
  // help
  //=====
  
  streamAll.addSetChapter("new_help", "Help text");
  streamAll.addSetRegion("new_help");
  
  streamAll.addSetFont("std");
  streamAll.addSetSize(189, 5);
  streamAll.addSetFailOnBoxOverflow(true);
  
  streamAll.addPlaceholderString("new-help-description");
  
  streamAll.addPlaceholderString("new-help-about");
  streamAll.addPlaceholderString("new-help-mechanics");
  streamAll.addPlaceholderString("new-help-stats");
  streamAll.addPlaceholderString("new-help-woods");
  streamAll.addPlaceholderString("new-help-labyrinth");
  streamAll.addPlaceholderString("new-help-sumar");
  streamAll.addPlaceholderString("new-help-desert");
  streamAll.addPlaceholderString("new-help-elbruz");
  streamAll.addPlaceholderString("new-help-mahaal");
  streamAll.addPlaceholderString("new-help-ziggurat");
  
  streamAll.addGenericLine("#SETAUTOINDENT(\"none\")");
  
  //=====
  // misc
  //=====
  
  streamAll.addSetChapter("new_misc", "Miscellaneous");
  streamAll.addSetRegion("new_misc");
  
  streamAll.addSetFont("std");
  streamAll.addSetSize(189, 1);
  streamAll.addSetFailOnBoxOverflow(true);
  
  // displayed in place of ending name for endings not yet unlocked
  streamAll.addGenericLine("#SETPADANDCENTER(0, 189)");
  streamAll.addPlaceholderString("new-endname-locked");
  streamAll.addGenericLine("#DISABLEPADANDCENTER()");
  
  streamAll.addPlaceholderString("new-empty");
  
  streamAll.addSetSize(189, 5);
  streamAll.addPlaceholderString("new-titlemenu-alt");
  
  streamAll.addPlaceholderString("new-eraseendlist");
  streamAll.addPlaceholderString("new-eraseendlist-confirm");*/
  
  //=============================================
  // string unbackgrounds
  //=============================================
  
/*  {
    std::ofstream ofs("asm/gen/unbackground_strings.inc");
    for (const auto& item: stringToSize) {
      ofs << ".unbackground " << getHexWordNumStr(item.first) << " "
        << getHexWordNumStr(item.first + item.second - 1)
        << std::endl;
    }
  }*/
  
  //=============================================
  // export sheets
  //=============================================
  
  {
    std::ofstream ofs;
    ofs.open("script/orig/spec.txt");
    streamAll.exportToSheet(sheetAll, ofs, "dlog");
    sheetAll.exportCsv("script/orig/script.csv");
  }
  
  
  return 0;
}
