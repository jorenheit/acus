#pragma once
#include "acus/assembler/assembler.h"
#include "acus/sugar/sugar_types.h"
#include "acus/sugar/sugar_impl.h"

namespace acus::sugar {

  extern Assembler __assembler;

  class Expr {
    Expression _expr;

  public:
    Expr() = default;
    Expr(Expression expr);
    Expr(literal::Literal lit);
    Expr(int val);
    Expr(std::string const &str);

    template <typename T> requires impl::IsSugarType<T>
    Expr(T const &val);
    
    Expression const &get() const;
    Expression &get();

    template <typename T>
    Expr cast() const;
    
    Expr field(std::string const &name) const;
    Expr operator[](size_t index) const;
    Expr operator[](Expr const &index) const;    
    Expr &operator=(Expr const &other);
    Expr &operator++();
    Expr &operator--();
    Expr &operator+=(Expr const &other);
    Expr &operator-=(Expr const &other);
    
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
  Expr let(std::string const &varName);

  Expr var(std::string const &varName);
  
  void program(std::string const &name, std::string const &entry = "main");
  void endProgram();
  void endFunction();
  void return_();
  void return_(Expr const &expr);
  void print(char c);
  void print(int x);
  void print(std::string const &str);
  void print(Expr const &expr);
  void println(auto&& arg);

  std::string generateBrainfuck(std::string const &programName);

  template <typename Signature>
  class FunctionHandle;

  template <typename Signature>
  FunctionHandle<Signature> function_fwd(std::string const &name);  

  template <typename Signature>
  FunctionHandle<Signature> call(std::string const &name);
  
  template <typename Signature, typename ... ArgNames>
  FunctionHandle<Signature> function(std::string const &name, ArgNames&& ... args);
  
  template <typename Signature, typename ... ArgNames>
  FunctionHandle<Signature> function(FunctionHandle<Signature> const &caller, ArgNames&& ... args);  
  
  
#define for_(init, condition, increment, body) do {	\
  forLoop([&]{ init; },					\
	  [&]{ return (condition).get(); },		\
	  [&]{ increment; },				\
	  [&]{ body; }); } while (0);


#define while_(condition, body) do {		\
  whileLoop([&] { return (condition).get(); },	\
	    [&] { body; }); } while (0); 
  
#define if_(condition, thenBody, ...) do {			\
  ifCondition([&] { return (condition).get(); },		\
	      [&] { thenBody; }					\
	      __VA_OPT__(, [&] { __VA_ARGS__; }));} while (0);	\


#include "sugar_public.tpp"
  
} // sugar
