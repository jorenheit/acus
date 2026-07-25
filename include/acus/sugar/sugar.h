// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include "acus/assembler/assembler.h"
#include "acus/sugar/sugar_types.h"
#include "acus/sugar/sugar_impl.h"
#include "acus/sugar/sugar_loc.h"

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
    Expr operator++(int);
    Expr &operator--();
    Expr operator-();
    Expr operator+();
    Expr operator!();
    Expr operator&();
    Expr operator*();
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
  Expr let_(std::string const &varName, SUGAR_FUNC);

  template <typename T> requires impl::IsSugarType<T>
  void global_(std::string const &varName, SUGAR_FUNC);
  
  Expr var_(std::string const &varName, SUGAR_FUNC);
  
  void program_(std::string const &name, std::string const &entry = "main", SUGAR_FUNC);
  void endProgram(SUGAR_FUNC);
  void endFunction(SUGAR_FUNC);
  void break__(SUGAR_FUNC);
  void continue__(SUGAR_FUNC);  
  void return__(SUGAR_FUNC);
  void return__(Expr const &expr, SUGAR_FUNC);

  std::string generateBrainfuck(std::string const &programName, SUGAR_FUNC);

  template <typename Signature> class FunctionHandle;
  template <typename Signature> class FunctionBuilder;

  template <typename Signature>
  auto function_(std::string const &functionName, auto&& ... argNames);

  namespace impl {
    class Return {
      bool _consumed = false;
      std::source_location _loc;
    
    public:
      Return(SUGAR_FUNC):
	_loc(std::move(loc))
      {}
    
      ~Return() {
	if (!_consumed) return__(_loc);
      }
    
      void operator()(auto&& ... arg) && {
	_consumed = true;
	return__(std::forward<decltype(arg)>(arg)..., _loc);
      }
    };
  }

  
#define for_(init, condition, increment)			\
  impl::makeForLoop([&]{ init; },				\
		    [&]{ return (condition).get(); },		\
		    [&]{ increment; },				\
		    std::source_location::current()) << [&]

#define while_(condition)					\
  impl::makeWhileLoop([&]{ return Expr{condition}.get(); },	\
		      std::source_location::current()) << [&]
  
#define if_(condition)						\
  impl::makeIfStatement([&]{ return (condition).get(); },	\
			std::source_location::current()) << [&]

#define else_ << [&]
#define define impl::FunctionDefinition{} << [&]
#define declare impl::FunctionDeclaration{}
#define break_ break__()
#define continue_ continue__()
#define return_ impl::Return{}
  
#include "sugar_public.tpp"
  
} // sugar
