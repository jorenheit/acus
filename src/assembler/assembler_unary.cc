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
  Slot result = getTemp(obj.type());
  assignSlot(result, materialize(obj.slot()));
  unOpAssignImpl<Operator>(Expression{result}, API_FWD);

  if constexpr (std::is_same_v<Ret, bool>) {
    result.get().type = ts::u8();
  }
  return Expression { result };  
}

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
  Slot const result = getTemp(toType);
  if (slot.type() == toType) {
    // same type but direct slot, so we copy it directly into our temp
    assignSlot(result, slot);
    return Expression{result};
  }
  
  // All other cases: construct a temp to return and populate it based on the type conversion
  // First byte can be copied without modification.
  pushPtr();
  moveTo(slot, MacroCell::Value0);
  copyField(Cell{result, MacroCell::Value0}, Temps<1>::select(result, MacroCell::Scratch0));

  // If both types (from and to) are 16-bits, we need to copy the high byte as well:
  if (slot.type()->usesValue1() && toType->usesValue1()) {
    moveTo(slot, MacroCell::Value1);
    copyField(Cell{result, MacroCell::Value1}, Temps<1>::select(result, MacroCell::Scratch0));    
  }
  // If we're widening S8, we need to sign-extend
  else if (slot.type()->tag() == types::S8 && toType->usesValue1()) {
    moveTo(slot, MacroCell::Value0);    
    signBitConstructive(Cell{slot, MacroCell::Flag},
			Temps<4>::select(slot, MacroCell::Scratch0,
					 slot, MacroCell::Scratch1,
					 slot, MacroCell::Payload0,
					 slot, MacroCell::Payload1));
    moveTo(slot, MacroCell::Flag);
    loopOpen(); {
      moveTo(result, MacroCell::Value1); zeroCell(); dec();
      moveTo(slot, MacroCell::Flag);     zeroCell();
    } loopClose();
  }
  // All other cases, just zero the high byte
  else {
    moveTo(result, MacroCell::Value1);
    zeroCell();
  }

  popPtr();
  return Expression{result};
}



// Unary algorithm implementations

void Assembler::notSlot(Slot rhs) {
  assert(rhs.size() == 1);

  pushPtr();
  moveTo(rhs);

  if (rhs.type()->usesValue1()) {
    not16Destructive(Cell{rhs, MacroCell::Value1},
		     Temps<1>::select(rhs, MacroCell::Scratch0));
  } else {
    notDestructive(Temps<1>::select(rhs, MacroCell::Scratch0));
  }
  
  popPtr();
}


void Assembler::boolSlot(Slot rhs) {
  assert(rhs.size() == 1);
  
  pushPtr();
  moveTo(rhs);

  if (rhs.type()->usesValue1()) {
    bool16Destructive(Cell{rhs, MacroCell::Value1},
		     Temps<1>::select(rhs, MacroCell::Scratch0));
  } else {
    boolDestructive(Temps<1>::select(rhs, MacroCell::Scratch0));
  }
  
  popPtr();
}

void Assembler::negateSlot(Slot rhs) {
  assert(rhs.size() == 1);

  pushPtr();

  moveTo(rhs, MacroCell::Value0);
  if (rhs.type()->usesValue1()) {
    Slot tmp = getTemp(ts::raw(1));
    negate16Destructive(Cell{rhs, MacroCell::Value1},
			Temps<6>::select(rhs, MacroCell::Scratch0,
					 rhs, MacroCell::Scratch1,
					 tmp, MacroCell::Scratch0,
					 tmp, MacroCell::Scratch1,
					 tmp, MacroCell::Payload0, 
					 tmp, MacroCell::Payload1));
    freeTempSlot(tmp);
  } else {
    negateDestructive(Temps<2>::select(rhs, MacroCell::Scratch0,
				       rhs, MacroCell::Scratch1));
  }

  popPtr();
}

void Assembler::absSlot(Slot rhs) {
  assert(types::isInteger(rhs.type()));

  if (types::isUnsignedInteger(rhs.type())) return;

  pushPtr();
  // Construct the signbit in rhs.Flag
  moveTo(rhs, rhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitConstructive(Cell{rhs, MacroCell::Flag},
		      Temps<4>::select(rhs, MacroCell::Scratch0, rhs, MacroCell::Scratch1,
				       rhs, MacroCell::Payload0, rhs, MacroCell::Payload1));

  // If the sign-bit was set, negate the slot
  moveTo(rhs, MacroCell::Flag);
  loopOpen(); {
    zeroCell();
    negateSlot(rhs);
  } loopClose();

  popPtr();
}

void Assembler::signBitSlot(Slot rhs) {
  assert(types::isSignedInteger(rhs.type()));
  
  pushPtr();
  moveTo(rhs, rhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitDestructive(Temps<3>::select(rhs, MacroCell::Scratch0,
				      rhs, MacroCell::Scratch1,
				      rhs, MacroCell::Payload0));

  if (rhs.type()->usesValue1()) {
    moveField(Cell{rhs, MacroCell::Value0});
  }
  popPtr();
}

void Assembler::signBitDestructive(Temps<3> tmp) {
  Cell const current = _dp.current();
  Cell const oneTwentyEight = tmp.get<0>();

  pushPtr();
  moveTo(oneTwentyEight);
  setToValue(128, tmp.select<1>());
  moveTo(current);
  greaterOrEqualDestructive(oneTwentyEight, tmp.select<1, 2>());
  popPtr();
}

void Assembler::signBitConstructive(Cell result, Temps<4> tmp) {
  pushPtr();
  copyField(result, tmp.get<0>());
  moveTo(result);
  signBitDestructive(tmp.select<1, 2, 3>());
  popPtr();
}


void Assembler::boolDestructive(Temps<1> tmp) {
  auto [current, tmp0] = getFieldIndices(_dp.current(), tmp.get<0>());
  emit<primitive::Boolean>(current, tmp0);
}

void Assembler::bool16Destructive(Cell high, Temps<1> tmp) {
  orDestructive(high, tmp.select<0>());
}

void Assembler::boolConstructive(Cell result, Temps<1> tmp) {
  pushPtr();
  copyField(result, tmp);
  moveTo(result);
  boolDestructive(tmp);
  popPtr();
}

void Assembler::bool16Constructive(Cell high, Cell result, Temps<2> tmp) {
  Cell const resultHigh = tmp.get<0>();
  
  pushPtr();
  copyField(result, tmp.select<1>());
  moveTo(high);
  copyField(resultHigh, tmp.select<1>());
  moveTo(result);
  bool16Destructive(resultHigh, tmp.select<1>());
  popPtr();  
}


void Assembler::notDestructive(Temps<1> tmp) {
  auto [cur, tmp0] = getFieldIndices(_dp.current(), tmp.get<0>());
  emit<primitive::Not>(cur, tmp0);
}

void Assembler::not16Destructive(Cell high, Temps<1> tmp) {
  orDestructive(high, tmp.select<0>());
  notDestructive(tmp.select<0>());
}

void Assembler::notConstructive(Cell result, Temps<1> tmp) {
  pushPtr();
  copyField(result, tmp);
  moveTo(result);
  notDestructive(tmp);
  popPtr();
}

void Assembler::not16Constructive(Cell high, Cell result, Temps<2> tmp) {
  Cell const resultHigh = tmp.get<0>();
  
  pushPtr();
  copyField(result, tmp.select<1>());
  moveTo(high);
  copyField(resultHigh, tmp.select<1>());
  moveTo(result);
  not16Destructive(resultHigh, tmp.select<1>());
  popPtr();  
}

void Assembler::negateDestructive(Temps<2> tmp) {
  Cell const copy = tmp.get<0>();

  pushPtr();
  copyField(copy, tmp.select<1>());
  zeroCell();
  subDestructive(copy);
  popPtr();
}


void Assembler::negateConstructive(Cell result, Temps<2> tmp) {
  pushPtr();
  copyField(result, tmp.select<0>());
  moveTo(result);
  negateDestructive(tmp);
  popPtr();
}

void Assembler::negate16Destructive(Cell high, Temps<6> tmp) {
  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;
  Cell const copyLow = tmp.get<0>();
  Cell const copyHigh = tmp.get<1>();

  pushPtr();
  moveTo(currentLow);
  copyField(copyLow, tmp.select<2>());
  zeroCell();
  moveTo(currentHigh);
  copyField(copyHigh, tmp.select<2>());
  zeroCell();

  moveTo(currentLow);
  sub16Destructive(currentHigh, copyLow, copyHigh, tmp.select<2, 3, 4, 5>());
  popPtr();
}

void Assembler::negate16Constructive(Cell high, Cell result, Temps<7> tmp) {
  Cell const resultHigh = tmp.get<0>();
  
  pushPtr();
  copyField(result, tmp.select<1>());
  moveTo(high);
  copyField(resultHigh, tmp.select<1>());
  moveTo(result);
  negate16Destructive(resultHigh, tmp.select<1, 2, 3, 4, 5, 6>());
  popPtr();  
}

