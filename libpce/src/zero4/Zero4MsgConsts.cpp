#include "zero4/Zero4MsgConsts.h"
#include "zero4/Zero4MsgDismException.h"
#include "util/TStringConversion.h"
#include "util/TBufStream.h"
#include "util/TParse.h"
#include <iostream>

using namespace BlackT;

namespace Pce {


int Zero4MsgConsts::getOpArgsSize(int opcode) {
//  if ((opcode < opcodeBase) || (opcode >= opcodeRangeEnd)) {
  if (!isOp(opcode)) {
    throw Zero4MsgDismException(T_SRCANDLINE,
                                "Zero4MsgConsts::getOpArgsSize()",
                                std::string("invalid opcode: ")
                                  + TStringConversion::intToString(opcode,
                                      TStringConversion::baseHex));
  }

  switch (opcode) {
  case opcode_printChar:
  case opcode_speed:
  case opcode_07:
  case opcode_subop1B_setTextPage:
  case opcode_subop1B_setTextPage04:
  case opcode_subop1B_setTextPage05:
  case opcode_subop1B_setTextPage06:
  case opcode_subop1B_setTextPal:
  case opcode_delay:
    return 1;
    break;
  case opcode_setPrintPos:
  case opcode_call:
  case opcode_call19:
  case opcode_printNum3:
  case opcode_printNum3_zeroes:
  case opcode_printNum3_FF:
  case opcode_printNum5:
  case opcode_printNum5_zeroes:
  case opcode_printNum5_FF:
  case opcode_jump:
    return 2;
    break;
  case opcode_subop1B_03:
    return 3;
    break;
  default:
    return 0;
    break;
  }
}

int Zero4MsgConsts::numDismPreOpLinebreaks(int opcode) {
  if (!isOp(opcode)) {
    throw Zero4MsgDismException(T_SRCANDLINE,
                                "Zero4MsgConsts::getOpArgsSize()",
                                std::string("invalid opcode: ")
                                  + TStringConversion::intToString(opcode,
                                      TStringConversion::baseHex));
  }
  
  switch (opcode) {
//  case opcode_wait:
//    return 1;
  case opcode_setPrintPos:
  case opcode_resetPrintPos:
    return 1;
    break;
  default:
    return 0;
    break;
  }
}

int Zero4MsgConsts::numDismPostOpLinebreaks(int opcode) {
  if (!isOp(opcode)) {
    throw Zero4MsgDismException(T_SRCANDLINE,
                                "Zero4MsgConsts::getOpArgsSize()",
                                std::string("invalid opcode: ")
                                  + TStringConversion::intToString(opcode,
                                      TStringConversion::baseHex));
  }
  
  switch (opcode) {
  case opcode_br:
  case opcode_setPrintPos:
  case opcode_resetPrintPos:
    return 1;
  case opcode_wait:
    return 2;
  default:
    return 0;
    break;
  }
}

bool Zero4MsgConsts::isOp(int opcode) {
  // special-case multi-byte 1B subops
  if (((opcode & 0xFF00) == 0x1B00)
      && ((opcode == opcode_subop1B_end)
          || (opcode == opcode_subop1B_setTextPage)
          || (opcode == opcode_subop1B_setTextPal)
          || (opcode == opcode_subop1B_03)
          || (opcode == opcode_subop1B_setTextPage04)
          || (opcode == opcode_subop1B_setTextPage05)
          || (opcode == opcode_subop1B_setTextPage06)
          )
      ) {
    return true;
  }
  
  if ((opcode >= opcodeBase) && (opcode < opcodeRangeEnd)) return true;
  
  return false;
}

bool Zero4MsgConsts::isInlineableOp(int opcode) {
  switch (opcode) {
  case opcode_printChar:
  case opcode_blankCurrentPos:
  case opcode_setPrintPos:
  case opcode_call:
  case opcode_speed:
  // ?
  case opcode_07:
  case opcode_dummy08:
  case opcode_toNext8:
  case opcode_br:
  case opcode_decPos:
  case opcode_incPos:
  case opcode_carriage:
  case opcode_selectBackPal:
  case opcode_selectFrontPal:
  case opcode_printNum3:
  case opcode_printNum3_zeroes:
  case opcode_printNum3_FF:
  case opcode_printNum5:
  case opcode_printNum5_zeroes:
  case opcode_printNum5_FF:
  case opcode_dummy16:
  case opcode_dummy17:
//  case opcode_jump:
  case opcode_call19:
//  case opcode_clear:
//  case opcode_subop1B_end:
  case opcode_subop1B_setTextPage:
  case opcode_subop1B_setTextPal:
  case opcode_subop1B_03:
  case opcode_subop1B_setTextPage04:
  case opcode_subop1B_setTextPage05:
  case opcode_subop1B_setTextPage06:
  case opcode_eraseToLineEnd:
  case opcode_delay:
  case opcode_resetPrintPos:
  case opcode_dummy1F:
    return true;
  default:
    return false;
    break;
  }
}


}
