#include "zero4/Zero4MsgDismException.h"

using namespace BlackT;

namespace Pce {


Zero4MsgDismException::Zero4MsgDismException()
  : TException() { };

Zero4MsgDismException::Zero4MsgDismException(
    const char* nameOfSourceFile__,
    int lineNum__,
    const std::string& source__)
  : TException(nameOfSourceFile__,
               lineNum__,
               source__) { };

Zero4MsgDismException::Zero4MsgDismException(
    const char* nameOfSourceFile__,
    int lineNum__,
    const std::string& source__,
    const std::string& problem__)
  : TException(nameOfSourceFile__,
               lineNum__,
               source__),
    problem_(problem__) { };

std::string Zero4MsgDismException::problem() const {
  return problem_;
}


} 
