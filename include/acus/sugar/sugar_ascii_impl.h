#pragma once
#include "acus/sugar/sugar_ascii.h"

namespace acus::sugar::impl {

  inline void IsDigit::emit(Expr &result, Expr const &val) {
    result = val >= '0' && val <= '9';
  }


  inline void IsLower::emit(Expr &result, Expr const &val) {
    result = val >= 'a' && val <= 'z';
  }


  inline void IsUpper::emit(Expr &result, Expr const &val) {
    result = val >= 'A' && val <= 'Z';
  }


  inline void IsAlpha::emit(Expr &result, Expr const &val) {
    result = (val >= 'a' && val <= 'z') ||
             (val >= 'A' && val <= 'Z');
  }


  inline void IsAlphanumeric::emit(Expr &result, Expr const &val) {
    result = (val >= '0' && val <= '9') ||
             (val >= 'a' && val <= 'z') ||
             (val >= 'A' && val <= 'Z');
  }


  inline void IsWhitespace::emit(Expr &result, Expr const &val) {
    result = val == ' '  ||
             val == '\t' ||
             val == '\n' ||
             val == '\v' ||
             val == '\f' ||
             val == '\r';
  }


  inline void ToLower::emit(Expr &result, Expr const &val) {
    result = val;

    if_(val >= 'A' && val <= 'Z') {
      result += 'a' - 'A';
    };
  }


  inline void ToUpper::emit(Expr &result, Expr const &val) {
    result = val;

    if_(val >= 'a' && val <= 'z') {
      result -= 'a' - 'A';
    };
  }

} // namespace acus::sugar::impl
