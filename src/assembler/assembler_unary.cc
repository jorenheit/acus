// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"


template <typename Operator>
Expression Assembler::unOpAssignImpl(Expression obj, API_CTX) {
  using Ret = Operator::ReturnType;
  static_assert(std::is_same_v<Ret, bool> || std::is_same_v<Ret, int>);

  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_IS_INTEGER(obj.type());
  assert(not obj.isLiteral());

  Slot const objSlot = materialize(obj.slot());
  Operator::applyToSlot(*this, objSlot);
  return obj;
}

template <typename Operator>
Expression Assembler::unOpImpl(Expression obj, API_CTX) {
  using Ret = Operator::ReturnType;
  static_assert(std::is_same_v<Ret, bool> || std::is_same_v<Ret, int>);

  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_IS_INTEGER(obj.type());
  
  auto makeLiteral = [&](int x) -> literal::Literal {
    if constexpr (std::is_same_v<Ret, bool>) return literal::u8(x);
    else {
      if (types::isU8(obj.type()))  return literal::u8(x);
      if (types::isS8(obj.type()))  return literal::s8(x);
      if (types::isU16(obj.type())) return literal::u16(x);
      if (types::isS16(obj.type())) return literal::s16(x);
      std::unreachable();
    }
  };

  if (obj.isLiteral()) {
    int const val = literal::cast<types::IntegerType>(obj.literal())->encodedValue();
    return Expression { makeLiteral(Operator::fold(val)) };
  }

  // Apply to temp copy
  Slot src = materialize(obj.slot());
  Slot result = [&]{
    if (src.kind() == Slot::Temp) {
      return src;
    } else {
      Slot copy = getTemp(obj.type(), allocHint(src));
      assignSlot(copy, src);
      return copy;
    }
  }();

  unOpAssignImpl<Operator>(Expression{result}, API_FWD);
  if constexpr (std::is_same_v<Ret, bool>) {
    result.get().type = ts::u8();
  }
  return Expression { result };
}

// Explicit instantiations for all of the unary operations
#define INSTANTIATE_FOR(op)						\
  template Expression Assembler::unOpImpl<op>(Expression, API_CTX); \
  template Expression Assembler::unOpAssignImpl<op>(Expression, API_CTX);

INSTANTIATE_FOR(Assembler::LogicalNot);
INSTANTIATE_FOR(Assembler::LogicalBool);
INSTANTIATE_FOR(Assembler::SignBit);
INSTANTIATE_FOR(Assembler::Negate);
INSTANTIATE_FOR(Assembler::Abs);
#undef INSTANTIATE_FOR

Expression Assembler::castImpl(Expression obj, types::TypeHandle toType, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  auto opResult = types::rules::castResult(obj.type(), toType);
  API_REQUIRE(opResult, error::ErrorCode::IncompatibleOperands, opResult.errorMsg);

  assert(not obj.isLiteral());
  assert(types::isInteger(obj.type()));
  assert(types::isInteger(toType));
  assert(toType == opResult.type);

  Slot const slot = materialize(obj.slot());
  Slot const result = getTemp(toType, allocHint(slot));
  if (slot.type() == toType) {
    // same type but direct slot, so we copy it directly into our temp
    assignSlot(result, slot);
    return Expression{result};
  }
  
  // All other cases: construct a temp to return and populate it based on the type conversion
  // First byte can be copied without modification.
  Cell const srcLow = Cell{slot, MacroCell::Value0};
  Cell const srcHigh = Cell{slot, MacroCell::Value1};
  Cell const resultLow = Cell{result, MacroCell::Value0};
  Cell const resultHigh = Cell{result, MacroCell::Value1};
  Cell const tmp = Cell{slot, MacroCell::Scratch0};
  
  if (slot.type()->usesValue1() && toType->usesValue1()) {
    // If both types (from and to) are 16-bits, we need to copy the high byte as well:
    copyField(srcLow, resultLow, tmp, true);
    copyField(srcHigh, resultHigh, tmp, true);
  }
  else if (slot.type()->tag() == types::S8 && toType->usesValue1()) {
    // If we're widening S8, we need to sign-extend
    copyField(srcLow, {resultLow, resultHigh}, tmp, true);
    signExtend(ws::promiseClean16(result), false);
  }
  else {
    // All other cases, just zero the high byte
    copyField(srcLow, resultLow, tmp, true);
    zeroCell(resultHigh);
  }

  return Expression{result};
}

// Unary algorithm implementations

void Assembler::notSlot(Slot rhs) {
  assert(rhs.size() == 1);

  if (rhs.type()->usesValue1()) {
    not16Destructive(ws::promiseClean16(rhs));
  } else {
    notDestructive(ws::promiseClean8(rhs));
  }
}


void Assembler::boolSlot(Slot rhs) {
  assert(rhs.size() == 1);
  
  if (rhs.type()->usesValue1()) {
    bool16Destructive(ws::promiseClean16(rhs));
  } else {
    zeroCell(Cell{rhs, MacroCell::Value1});
    boolDestructive(ws::promise(rhs, ws::Layout<ws::Data<>, ws::Scratch>{}));
  }
}

void Assembler::negateSlot(Slot rhs) {
  assert(types::isInteger(rhs.type()));

  // Move the original value out, leaving rhs zero.
  Slot const copy = getTemp(rhs.type(), allocHint(rhs));
  assignSlot(copy, rhs, TransferMode::Move);

  // rhs = 0 - original
  if (rhs.type()->usesValue1()) {
    sub16Destructive(ws::promise(rhs, ws::Layout<ws::Data<0>, ws::Data<0>, ws::ScratchCells<5>>{}),
                     ws::promiseClean16(copy));
  } else {
    subDestructive(ws::promise(rhs, ws::Layout<ws::Data<0>, ws::Data<>, ws::ScratchCells<5>>{}),
                   ws::promiseClean8(copy));
  }

  freeTempSlot(copy);
}

void Assembler::absSlot(Slot rhs) {
  assert(types::isInteger(rhs.type()));
  if (types::isUnsignedInteger(rhs.type())) return;

  copyFieldToZero(Cell{rhs, rhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0},
            Cell{rhs, MacroCell::Scratch0},
            Cell{rhs, MacroCell::Scratch1}, true);

  signBitDestructive(ws::promise(Cell{rhs, MacroCell::Scratch0},
                                 ws::Layout<ws::Data<>, ws::ScratchCells<4>>{}));
  
  // If the sign-bit was set, negate the slot
  Cell const signBitFlag = Cell{rhs, MacroCell::Scratch0};
  loop(signBitFlag, [&]{
    zeroCell(signBitFlag);
    negateSlot(rhs);
  });
}

void Assembler::signBitSlot(Slot rhs) {

  assert(types::isSignedInteger(rhs.type()));
  if (rhs.type()->usesValue1()) {
    moveField(Cell{rhs, MacroCell::Value1},
              Cell{rhs, MacroCell::Value0});
  } else {
    zeroCell(Cell{rhs, MacroCell::Value1});
  }

  signBitDestructive(ws::promise(rhs, ws::Layout<ws::Data<>, ws::ScratchCells<5>>{}));
}
