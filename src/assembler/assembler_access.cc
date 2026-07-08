// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

Expression Assembler::structFieldImpl(Expression obj, int fieldIndex, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_IS_STRUCT(obj.type());
  API_REQUIRE_FIELD_INDEX_IN_BOUNDS(obj, fieldIndex);
  
  auto structType = types::cast<types::StructType>(obj.type());  
  return structFieldImpl(obj, structType->fieldName(fieldIndex), API_FWD);
}

Expression Assembler::structFieldImpl(Expression obj, std::string const &fieldName, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_IS_STRUCT(obj.type());
  API_REQUIRE_IS_FIELD(obj, fieldName);

  if (obj.isLiteral()) {
    auto structVal = literal::cast<types::StructType>(obj.literal());
    return Expression{structVal->field(fieldName)};
  }
  
  return Expression{proxy::structField(obj.slot(), fieldName)};
}

Expression Assembler::arrayElementImpl(Expression arr, int index, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_IS_ARRAY_OR_STRING(arr.type());
  API_REQUIRE_INDEX_IN_BOUNDS(arr, index);

  if (arr.isLiteral()) {
    auto arrVal = literal::cast<types::ArrayLike>(arr.literal());
    return Expression{arrVal->element(index)};
  }
  
  return Expression{proxy::arrayElement(arr.slot(), index)};  
}  

Expression Assembler::arrayElementImpl(Expression arr, Expression index, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_IS_ARRAY_OR_STRING(arr.type());
  API_REQUIRE_IS_INTEGER(index.type());
  
  if (index.isLiteral()) {
    int const i = literal::cast<types::IntegerType>(index.literal())->semanticValue();
    API_REQUIRE(i >= 0,
		error::ErrorCode::NegativeIndex,
		"index may not be negative.");
    return arrayElementImpl(arr, i, API_FWD);
  }

  if (arr.isLiteral()) {
    Slot const tmp = getTemp(arr.type());
    assignSlot(tmp, arr.literal());
    return Expression{proxy::arrayElement(tmp, index.slot())};
  }
  
  return Expression{proxy::arrayElement(arr.slot(), index.slot())};
}

Expression Assembler::dereferencePointerImpl(Expression ptr, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_IS_POINTER(ptr.type());
  assert(not ptr.isLiteral());
  
  return Expression{proxy::dereferencedPointer(ptr.slot())};
}

