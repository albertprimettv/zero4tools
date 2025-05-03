#include "pce/PceVram.h"
#include "util/ByteConversion.h"
#include "util/TStringConversion.h"
#include "util/TBufStream.h"
#include "exception/TGenericException.h"
#include <string>

using namespace BlackT;

namespace Pce {


PceVram::PceVram()
  : data_(dataSize) {
  // zero-init
  for (int i = 0; i < dataSize; i++) data_[i] = 0;
}

PcePattern PceVram::getPattern(int patternNum) const {
  if (patternNum >= numPatterns) {
    throw TGenericException(T_SRCANDLINE,
                            "PceVram::getPattern()",
                            std::string("Out-of-range pattern number: ")
                              + TStringConversion::intToString(patternNum));
  }
  
  TBufStream ifs;
  ifs.write((char*)(data_.data() + (patternNum * PcePattern::size)),
            PcePattern::size);
  
  ifs.seek(0);
  PcePattern pattern;
  pattern.read(ifs);
  
  return pattern;
}

PceSpritePattern PceVram::getSpritePattern(int patternNum) const {
  if (patternNum >= numPatterns) {
    throw TGenericException(T_SRCANDLINE,
                            "PceVram::getSpritePattern()",
                            std::string("Out-of-range pattern number: ")
                              + TStringConversion::intToString(patternNum));
  }
  
  TBufStream ifs;
  ifs.write((char*)(data_.data() + (patternNum * PceSpritePattern::size)),
            PceSpritePattern::size);
  
  ifs.seek(0);
  PceSpritePattern pattern;
  pattern.read(ifs);
  
  return pattern;
}

void PceVram::setPattern(int patternNum, const PcePattern& src) {
  if (patternNum >= numPatterns) {
    throw TGenericException(T_SRCANDLINE,
                            "PceVram::setPattern()",
                            std::string("Out-of-range pattern number: ")
                              + TStringConversion::intToString(patternNum));
  }

  TBufStream temp;
  src.write(temp);
  temp.seek(0);
  temp.read((char*)(data_.data() + (patternNum * PcePattern::size)),
            PcePattern::size);
}

void PceVram::setSpritePattern(int patternNum, const PceSpritePattern& src) {
  if (patternNum >= numPatterns) {
    throw TGenericException(T_SRCANDLINE,
                            "PceVram::setSpritePattern()",
                            std::string("Out-of-range pattern number: ")
                              + TStringConversion::intToString(patternNum));
  }

  TBufStream temp;
  src.write(temp);
  temp.seek(0);
  temp.read((char*)(data_.data() + (patternNum * PceSpritePattern::size)),
            PceSpritePattern::size);
}

void PceVram::readPatterns(BlackT::TStream& src, int patternNum, int count) {
  if (count == -1) {
    count = src.remaining() / PcePattern::size;
  }
  
  if (patternNum + count > numPatterns) {
    throw TGenericException(T_SRCANDLINE,
                            "PceVram::readPatterns()",
                            std::string("Loading ")
                              + TStringConversion::intToString(count)
                              + " patterns to "
                              + TStringConversion::intToString(patternNum)
                              + " would overflow VRAM");
  }
  
  for (int i = 0; i < count; i++) {
    PcePattern pattern;
    pattern.read(src);
    setPattern(patternNum + i, pattern);
  }
}

void PceVram::readSpritePatterns(
    BlackT::TStream& src, int patternNum, int count) {
  if (count == -1) {
    count = src.remaining() / PcePattern::size;
  }
  
  if (patternNum + count > numPatterns) {
    throw TGenericException(T_SRCANDLINE,
                            "PceVram::readSpritePatterns()",
                            std::string("Loading ")
                              + TStringConversion::intToString(count)
                              + " patterns to "
                              + TStringConversion::intToString(patternNum)
                              + " would overflow VRAM");
  }
  
  for (int i = 0; i < count; i++) {
    PceSpritePattern pattern;
    pattern.read(src);
    setSpritePattern(patternNum + (i * 4), pattern);
  }
}

void PceVram::readRawByte(BlackT::TStream& src, int byteAddr, int length) {
  for (int i = 0; i < length; i++) {
    data_[byteAddr + i] = src.get();
  }
}

void PceVram::readRawWord(BlackT::TStream& src, int wordAddr, int length) {
  for (int i = 0; i < (length * 2); i += 2) {
    data_[(wordAddr * 2) + i + 0] = src.get();
    data_[(wordAddr * 2) + i + 1] = src.get();
  }
}

void PceVram::writeRawByte(
    BlackT::TStream& dst, int byteAddr, int length) const {
  for (int i = 0; i < length; i++) {
    dst.put(data_[byteAddr + i]);
  }
}

void PceVram::writeRawWord(
    BlackT::TStream& dst, int wordAddr, int length) const {
  for (int i = 0; i < (length * 2); i += 2) {
    dst.put(data_[(wordAddr * 2) + i + 0]);
    dst.put(data_[(wordAddr * 2) + i + 1]);
  }
}




}
