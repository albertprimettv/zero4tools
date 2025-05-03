#ifndef ZERO4MSGDISMEXCEPTION_H
#define ZERO4MSGDISMEXCEPTION_H


#include "exception/TException.h"
#include <exception>
#include <string>

namespace Pce {


class Zero4MsgDismException : public BlackT::TException {
public:
  
  Zero4MsgDismException();
  
  Zero4MsgDismException(const char* nameOfSourceFile__,
                              int lineNum__,
                              const std::string& source__);
  
  Zero4MsgDismException(const char* nameOfSourceFile__,
                              int lineNum__,
                              const std::string& source__,
                              const std::string& problem__);
  
  std::string problem() const;
  
protected:
  
  std::string problem_;
  
};


}


#endif
