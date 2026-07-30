#pragma once

namespace acus::sugar::impl {

  template <concepts::Clearable Type>
  void Clear<Type>::emit(Expr &expr) {
    operations::Clear<Type>::apply(expr);
  }


  template <concepts::Assignable Type>
  void Swap<Type>::emit(Expr x1, Expr x2) {
    auto tmp = (let_<Type>(nextVarName()) = x1);
    x1 = x2;
    x2 = tmp;
  }
  

}
