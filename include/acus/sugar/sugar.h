#pragma once
#include "acus/assembler/assembler.h"
#include "acus/sugar/sugar_types.h"
#include "acus/sugar/sugar_impl.h"

#define LOC loc
#define LOC_FWD LOC
#define SUGAR_LOC std::source_location LOC
#define SUGAR_FUNC SUGAR_LOC = std::source_location::current()

namespace acus::sugar {

  extern Assembler __assembler;

  class Expr {
    Expression _expr;
    std::source_location _loc;
    
  public:
    Expr(SUGAR_FUNC);
    Expr(Expr const &expr, SUGAR_FUNC);
    Expr(Expression expr, SUGAR_FUNC);
    Expr(literal::Literal lit, SUGAR_FUNC);
    Expr(int val, SUGAR_FUNC);

    template <typename T> requires std::convertible_to<T, std::string>
    Expr(T const &str, SUGAR_FUNC);

    template <typename T> requires impl::IsSugarType<T>
    Expr(T const &val, SUGAR_FUNC);
    
    Expression const &get() const;
    Expression &get();
    std::source_location const &loc() const { return _loc; }

    template <typename T>
    Expr cast(SUGAR_FUNC) const;
    
    Expr field(std::string const &name, SUGAR_FUNC) const;
    Expr operator[](size_t index) const;
    Expr operator[](Expr const &index) const;    
    Expr &operator=(Expr const &other);
    Expr &operator++();
    Expr &operator--();
    Expr operator-();
    Expr operator+();
    Expr operator!();
    Expr &operator+=(Expr const &other);
    Expr &operator-=(Expr const &other);
    Expr &operator*=(Expr const &other);
    Expr &operator/=(Expr const &other);
    Expr &operator%=(Expr const &other);
  };

  Expr operator+(Expr const &lhs, Expr const &rhs);
  Expr operator-(Expr const &lhs, Expr const &rhs);
  Expr operator*(Expr const &lhs, Expr const &rhs);
  Expr operator/(Expr const &lhs, Expr const &rhs);
  Expr operator%(Expr const &lhs, Expr const &rhs);
  Expr operator<(Expr const &lhs, Expr const &rhs);
  Expr operator>(Expr const &lhs, Expr const &rhs);
  Expr operator<=(Expr const &lhs, Expr const &rhs);
  Expr operator>=(Expr const &lhs, Expr const &rhs);
  Expr operator==(Expr const &lhs, Expr const &rhs);
  Expr operator!=(Expr const &lhs, Expr const &rhs);
  Expr operator&&(Expr const &lhs, Expr const &rhs);
  Expr operator||(Expr const &lhs, Expr const &rhs);

  template <typename T> requires impl::IsSugarType<T>
  Expr let(std::string const &varName, SUGAR_FUNC);

  Expr var(std::string const &varName, SUGAR_FUNC);
  
  void program(std::string const &name, std::string const &entry = "main", SUGAR_FUNC);
  void endProgram(SUGAR_FUNC);
  void endFunction(SUGAR_FUNC);
  void break_(SUGAR_FUNC);
  void continue_(SUGAR_FUNC);  
  void return_(SUGAR_FUNC);
  void return_(Expr const &expr, SUGAR_FUNC);
  void print(char c, SUGAR_FUNC);
  void print(int x, SUGAR_FUNC);
  void print(Expr const &expr, SUGAR_FUNC);
  void println(auto&& arg, SUGAR_FUNC);

  std::string generateBrainfuck(std::string const &programName, SUGAR_FUNC);

  template <typename Signature>
  class FunctionHandle;

  template <typename Signature>
  FunctionHandle<Signature> function_fwd(std::string const &name, SUGAR_FUNC);  

  template <typename Signature>
  FunctionHandle<Signature> call(std::string const &name, SUGAR_FUNC);

  template <typename Init, typename Condition, typename Increment>
  class ForBuilder;

  template <typename Condition>
  class WhileBuilder;

  template <typename Condition>
  class IfBuilder;
  
  template <typename Init, typename Condition, typename Increment>
  auto makeForLoop(Init&& init, Condition&& condition, Increment&& increment, SUGAR_LOC) {
    return ForBuilder{std::forward<Init>(init),
		      std::forward<Condition>(condition),
		      std::forward<Increment>(increment),
		      LOC_FWD};
  }

  template <typename Condition>
  auto makeWhileLoop(Condition&& condition, SUGAR_LOC) {
    return WhileBuilder{std::forward<Condition>(condition), LOC_FWD};
  }

  template <typename Condition>
  auto makeIfStatement(Condition&& condition, SUGAR_LOC) {
    return IfBuilder{std::forward<Condition>(condition), LOC_FWD};
  }
  
  
#define for_(init, condition, increment)		\
  makeForLoop([&]{ init; },				\
	      [&]{ return (condition).get(); },		\
	      [&]{ increment; },			\
	      std::source_location::current()) << [&]

#define while_(condition)				\
  makeWhileLoop([&]{ return (condition).get(); },	\
		std::source_location::current()) << [&]
  
#define if_(condition)					\
  makeIfStatement([&]{ return (condition).get(); },	\
		  std::source_location::current()) << [&]

#define else_ << [&]
  
#include "sugar_public.tpp"
  
} // sugar
