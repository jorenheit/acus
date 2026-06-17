#pragma once
#include "acus/sugar/sugar_types.h"
#include <stack>

namespace acus::sugar::impl {
  
  template <typename T>
  inline constexpr bool IsSugarType = std::is_same_v<T, void> || std::is_base_of_v<SugarType, std::remove_cvref_t<T>>;

  template <typename Tuple>
  struct IsTupleOfSugarTypesImpl: std::false_type {};

  template <typename ... Types>
  struct IsTupleOfSugarTypesImpl<std::tuple<Types ...>> {
    static constexpr bool value = (IsSugarType<Types> && ...);
  };
  
  template <typename Tuple>
  inline constexpr bool IsTupleOfSugarTypes = IsTupleOfSugarTypesImpl<std::remove_cvref_t<Tuple>>::value;

  
  inline literal::Literal toLiteral(int x) {
    if (x >= 256) return literal::u16(x);
    if (x >= 0)   return literal::u8(x);
    if (x < -127) return literal::s16(x);
    return literal::s8(x);
  }

  inline literal::Literal toLiteral(int x, types::TypeHandle type) {
    assert(types::isInteger(type));
    if (type == ts::u8())  return literal::u8(x);
    if (type == ts::u16()) return literal::u16(x);
    if (type == ts::s8())  return literal::s8(x);
    if (type == ts::s16()) return literal::s16(x);
    std::unreachable();
  }

  inline literal::Literal toLiteral(literal::Literal lit) {
    return lit;
  }
  
  template <typename T> requires IsSugarType<T>
  literal::Literal toLiteral(T const &val) {
    return val.toLiteral();
  }


  template <typename T> requires IsSugarType<T>
  types::TypeHandle getTypeHandle() {
    if constexpr (std::is_same_v<T, void>) return ts::void_t();
    else return T::type();
  }

  std::string nextLabel();

  class ControlStack {

    static std::stack<std::string> _continueStack;
    static std::stack<std::string> _breakStack;

  public:
    
    static std::string getContinueLabel() {
      assert(not _continueStack.empty());
      return _continueStack.top();
    }

    static std::string getBreakLabel() {
      assert(not _breakStack.empty());
      return _breakStack.top();
    }

    static void pop() {
      assert(not _continueStack.empty());      
      assert(not _breakStack.empty());
      _breakStack.pop();
      _continueStack.pop();
    }

    static void push(std::string const &continueLabel, std::string const &breakLabel) {
      _continueStack.push(continueLabel);
      _breakStack.push(breakLabel);
    }
    
  };
  
} // impl
