#include "acus/sugar/sugar_impl.h"

namespace acus::sugar::impl {
  std::stack<std::string> ControlStack::_continueStack;
  std::stack<std::string> ControlStack::_breakStack;

  std::string nextLabel() {
    static size_t count = 0;
    return "__sugar_label_" + std::to_string(count++);
  }
  
}
