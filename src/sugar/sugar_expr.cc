// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "acus/sugar/sugar.h"

namespace acus::sugar {

  Expr::Expr(Expr const &expr, SUGAR_LOC):
    _expr(expr.get()),
    _loc(LOC_FWD)
  {}
  
  Expr::Expr(Expression expr, SUGAR_LOC):
    _expr(std::move(expr)),
    _loc(LOC_FWD)
  {}
  
  Expr::Expr(literal::Literal lit, SUGAR_LOC):
    _expr(lit),
    _loc(LOC_FWD)
  {}

  Expr::Expr(int val, SUGAR_LOC):
    _expr(impl::toLiteral(val)),
    _loc(LOC_FWD)
  {}

  Expression const &Expr::get() const {
    return _expr;
  }

  Expression &Expr::get() {
    return _expr;
  }
  
  Expr Expr::operator*() {
    return Expr {__assembler.dereferencePointer(get(), _loc), _loc};
  }

  Expr Expr::operator&() {
    return Expr {__assembler.addressOf(get(), _loc), _loc};
  }
  
  Expr Expr::operator-() {
    return Expr {__assembler.negate(get(), _loc), _loc};
  }

  Expr Expr::operator+() {
    return *this;
  }

  Expr Expr::operator!() {
    return Expr {__assembler.lnot(get(), _loc), _loc};
  }
  
  Expr &Expr::operator++() {
    __assembler.addAssign(get(), impl::toLiteral(1, get().type()), _loc);
    return *this;
  }

  Expr Expr::operator++(int) {
    auto copy = Expr{__assembler.declareLocal(impl::nextVarName(), get().type())};
    copy = *this;
    __assembler.addAssign(get(), impl::toLiteral(1, get().type()), _loc);
    return copy;
  }
  
  Expr &Expr::operator--() {
    __assembler.subAssign(get(), impl::toLiteral(1, get().type()), _loc);
    return *this;
  }

  
  Expr Expr::field(std::string const &name, SUGAR_LOC) const {
    return Expr(__assembler.structField(get(), name, LOC_FWD), LOC_FWD);
  }

  Expr Expr::operator[](size_t index) const {
    return Expr(__assembler.arrayElement(get(), impl::toLiteral(index), _loc), _loc);
  }

  Expr Expr::operator[](Expr const &index) const {
    return Expr(__assembler.arrayElement(get(), index.get(), _loc), _loc);
  }


  // Assignment operators
#define ASSIGN_OPERATOR(symbol, asmFunc)		\
  Expr &Expr::operator symbol(Expr const &other) {	\
    __assembler.asmFunc(get(), other.get(), _loc);	\
    return *this;					\
  }

  ASSIGN_OPERATOR(=, assign);
  ASSIGN_OPERATOR(+=, addAssign);
  ASSIGN_OPERATOR(-=, subAssign);
  ASSIGN_OPERATOR(*=, mulAssign);
  ASSIGN_OPERATOR(/=, divAssign);
  ASSIGN_OPERATOR(%=, modAssign);

#undef ASSIGN_OPERATOR
  
  
  // Free operators
#define FREE_OPERATOR(symbol, asmFunc)					\
  Expr operator symbol(Expr const& lhs, Expr const& rhs) {              \
    auto const loc = lhs.loc();                                         \
    return Expr{__assembler.asmFunc(lhs.get(), rhs.get(), loc), loc};	\
  }
  
  FREE_OPERATOR(+,  add);
  FREE_OPERATOR(-,  sub);
  FREE_OPERATOR(*,  mul);
  FREE_OPERATOR(/,  div);
  FREE_OPERATOR(%,  mod);
  FREE_OPERATOR(<,  lt);
  FREE_OPERATOR(>,  gt);
  FREE_OPERATOR(<=, le);
  FREE_OPERATOR(>=, ge);
  FREE_OPERATOR(==, eq);
  FREE_OPERATOR(!=, neq);
  FREE_OPERATOR(&&, land);
  FREE_OPERATOR(||, lor);

#undef FREE_OPERATOR
  
} // namespace sugar
