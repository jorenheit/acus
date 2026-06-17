#include "acus/sugar/sugar.h"

namespace acus::sugar {


  Expr::Expr(Expression expr):
    _expr(std::move(expr))
  {}
  
  Expr::Expr(literal::Literal lit):
    _expr(lit)
  {}

  Expr::Expr(int val):
    _expr(impl::toLiteral(val))
  {}

  Expr::Expr(std::string const &str):
    _expr(literal::string(str))
  {}
    
    
  Expression const &Expr::get() const {
    return _expr;
  }

  Expression &Expr::get() {
    return _expr;
  }
  
  Expr &Expr::operator=(Expr const &other) {
    __assembler.assign(get(), other.get());
    return *this;
  }

  Expr &Expr::operator++() {
    __assembler.addAssign(get(), impl::toLiteral(1, get().type()));
    return *this;
  }

  Expr &Expr::operator--() {
    __assembler.subAssign(get(), impl::toLiteral(1, get().type()));
    return *this;
  }

  Expr &Expr::operator+=(Expr const &other) {
    __assembler.addAssign(get(), other.get());
    return *this;
  }
  
  Expr &Expr::operator-=(Expr const &other) {
    __assembler.addAssign(get(), other.get());
    return *this;
  }
  
  Expr Expr::field(std::string const &name) const {
    return __assembler.structField(_expr, name);
  }

  Expr Expr::operator[](size_t index) const {
    return __assembler.arrayElement(_expr, impl::toLiteral(index));
  }

  Expr Expr::operator[](Expr const &index) const {
    return __assembler.arrayElement(_expr, index.get());
  }

  
  // Free operators
  Expr operator+(Expr const &lhs, Expr const &rhs) { return __assembler.add(lhs.get(), rhs.get()); }
  Expr operator-(Expr const &lhs, Expr const &rhs) { return __assembler.sub(lhs.get(), rhs.get()); }
  Expr operator*(Expr const &lhs, Expr const &rhs) { return __assembler.mul(lhs.get(), rhs.get()); }
  Expr operator/(Expr const &lhs, Expr const &rhs) { return __assembler.div(lhs.get(), rhs.get()); }
  Expr operator%(Expr const &lhs, Expr const &rhs) { return __assembler.mod(lhs.get(), rhs.get()); }
  Expr operator<(Expr const &lhs, Expr const &rhs) { return __assembler.lt(lhs.get(), rhs.get()); }
  Expr operator>(Expr const &lhs, Expr const &rhs) { return __assembler.gt(lhs.get(), rhs.get()); }
  Expr operator<=(Expr const &lhs, Expr const &rhs) { return __assembler.le(lhs.get(), rhs.get()); }
  Expr operator>=(Expr const &lhs, Expr const &rhs) { return __assembler.ge(lhs.get(), rhs.get()); }
  Expr operator==(Expr const &lhs, Expr const &rhs) { return __assembler.eq(lhs.get(), rhs.get()); }
  Expr operator!=(Expr const &lhs, Expr const &rhs) { return __assembler.neq(lhs.get(), rhs.get()); }
  Expr operator&&(Expr const &lhs, Expr const &rhs) { return __assembler.land(lhs.get(), rhs.get()); }
  Expr operator||(Expr const &lhs, Expr const &rhs) { return __assembler.lor(lhs.get(), rhs.get()); }
  

} // namespace sugar
