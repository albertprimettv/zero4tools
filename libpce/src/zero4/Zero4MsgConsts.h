#ifndef ZERO4MSGCONSTS_H
#define ZERO4MSGCONSTS_H


#include "util/TStream.h"
#include <string>
#include <vector>
#include <map>

namespace Pce {


class Zero4MsgConsts {
public:
  const static int opcodeBase = 0x00;
//  const static int opcodeRangeEnd = 0x10;
  const static int opcodeRangeEnd = 0x20;
  
//  const static int opcodeBaseCorrected = 0xFFF0;
//  const static int opcodeRangeEndCorrected = 0x10000;
  
  const static int opcode_end = 0x00;
  const static int opcode_printChar = 0x01;
  const static int opcode_wait = 0x02;
  const static int opcode_blankCurrentPos = 0x03;
  const static int opcode_setPrintPos = 0x04;
  const static int opcode_call = 0x05;
  const static int opcode_speed = 0x06;
  const static int opcode_07 = 0x07;
  const static int opcode_dummy08 = 0x08;
  const static int opcode_toNext8 = 0x09;
  const static int opcode_br = 0x0A;
  const static int opcode_decPos = 0x0B;
  const static int opcode_incPos = 0x0C;
  const static int opcode_carriage = 0x0D;
  const static int opcode_selectBackPal = 0x0E;
  const static int opcode_selectFrontPal = 0x0F;
  const static int opcode_printNum3 = 0x10;
  const static int opcode_printNum3_zeroes = 0x11;
  const static int opcode_printNum3_FF = 0x12;
  const static int opcode_printNum5 = 0x13;
  const static int opcode_printNum5_zeroes = 0x14;
  const static int opcode_printNum5_FF = 0x15;
  const static int opcode_dummy16 = 0x16;
  const static int opcode_dummy17 = 0x17;
  const static int opcode_jump = 0x18;
  const static int opcode_call19 = 0x19;
  const static int opcode_clear = 0x1A;
  // op 1B uses various subops, which we encode directly into the opcode
  const static int opcode_subop1B = 0x1B;
    const static int opcode_subop1B_end = 0x1B00;
    const static int opcode_subop1B_setTextPage = 0x1B01;
    const static int opcode_subop1B_setTextPal = 0x1B02;
    const static int opcode_subop1B_03 = 0x1B03;
    // synonyms of setTextPage
    const static int opcode_subop1B_setTextPage04 = 0x1B04;
    const static int opcode_subop1B_setTextPage05 = 0x1B05;
    const static int opcode_subop1B_setTextPage06 = 0x1B06;
  const static int opcode_eraseToLineEnd = 0x1C;
  const static int opcode_delay = 0x1D;
  const static int opcode_resetPrintPos = 0x1E;
  const static int opcode_dummy1F = 0x1F;
  
  // NEW FOR TRANSLATION
  const static int transOp_setFont = 0x08;
  const static int transOp_setFont_std = 0x0800;
  const static int transOp_setFont_monospace = 0x0801;
  const static int transOp_setFont_monospaceIt = 0x0802;
  const static int transOp_setFont_bold = 0x0803;
  const static int transOp_setFont_italic = 0x0804;
  const static int transOp_setFont_ascii = 0x0805;
  
  // inline preprocessor directives (new for translation)
//  const static int pseudoOp_startFont_bold = 0x10000;
//  const static int pseudoOp_endFont_bold = 0x10001;
//  const static int pseudoOp_startFont_narrow = 0x10002;
//  const static int pseudoOp_endFont_narrow = 0x10003;
//  const static int pseudoOp_startFont_gil = 0x10004;
//  const static int pseudoOp_endFont_gil = 0x10005;
  
  static int getOpArgsSize(int opcode);
  static int numDismPreOpLinebreaks(int opcode);
  static int numDismPostOpLinebreaks(int opcode);
  static bool isOp(int opcode);
  static bool isInlineableOp(int opcode);
protected:
  
};


}


#endif
