// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

Expression Assembler::rValue(Expression val, API_CTX) const {
  if (val.isLiteral()) return rValue(val.literal(), API_FWD);
  return Expression{val};
}

Expression Assembler::rValue(std::string const &var, API_CTX) const {
  return Expression{proxyFromVariableName(var, API_FWD)};
}

Expression Assembler::rValue(SlotProxy slot, API_CTX) const {
  (void)API_CTX_NAME;
  return Expression{slot};
}

Expression Assembler::rValue(literal::Literal val, API_CTX) const {
  if (_program.mode == Program::Mode::StraightLine) {
    auto check = [&](auto &&self, literal::Literal const &value) -> void {
      if (types::isFunctionPointer(value.type())) {
        API_REQUIRE_BLOCK_DISPATCH_MODE();
      } else if (types::isArray(value.type()) || types::isString(value.type())) {
        auto const array = literal::cast<types::ArrayLike>(value);
        for (int i = 0; i < types::cast<types::ArrayLike>(value.type())->length(); ++i) self(self, array->element(i));
      } else if (types::isStruct(value.type())) {
        auto const type = types::cast<types::StructType>(value.type());
        auto const fields = literal::cast<types::StructType>(value);
        for (int i = 0; i < type->fieldCount(); ++i)
          self(self, fields->field(type->fieldName(i)));
      }
    };
    check(check, val);
  }
  return Expression{val};
}

Expression Assembler::lValue(Expression val, API_CTX) const {
  API_REQUIRE(val.hasSlot(),
	      error::ErrorCode::ReadOnlyExpression,
	      "cannot convert expression '", val.str(), "' to L-value.");
  return Expression{val};
}

Expression Assembler::lValue(std::string const &var, API_CTX) const {
  return Expression{proxyFromVariableName(var, API_FWD)};
}
  
Expression Assembler::lValue(SlotProxy slot, API_CTX) const {
  (void)API_CTX_NAME;
  return Expression{slot};
}
