// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

// TODO: rename this file to assembler_io.cc

void Assembler::readImpl(Expression rhs, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE(rhs.type()->size() == 1,
	      error::ErrorCode::InvalidReadType,
	      "input must be read into variable-type of size 1.");
  assert(not rhs.isLiteral());

  _cache.write(rhs.slot(), [&](Slot dest){
    readSlot(dest);
  });
}

void Assembler::readSlot(Slot target) {
  pushPtr();
  moveTo(target);
  emit<primitive::In>();
  popPtr();
}

void Assembler::writeImpl(Expression rhs, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  Slot slot = [&] {
    if (rhs.hasSlot()) return materialize(rhs.slot());
    return getTemp(rhs.literal());
  }();

  writeSlot(slot);
  if (slot.kind() == Slot::Temp) freeTempSlot(slot);
}

void Assembler::writeSlot(Slot slot) {
  pushPtr();
  for (int i = 0; i != slot.type()->size(); ++i) {
    moveTo(slot + i, MacroCell::Value0);
    emit<primitive::Out>();
    if (slot.type()->usesValue1()) {
      moveTo(slot + i, MacroCell::Value1);
      emit<primitive::Out>();
    }
  }
  popPtr();
}

void Assembler::printImpl(Expression val, API_CTX) {
  
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  
  if (types::isInteger(val.type())) {
    // For literals, inline the printing code
    if (val.isLiteral()) return printDecimal(val);

    // For slots, call the builtin functions
    BuiltinFunction const func = [&]{
      switch (val.type()->tag()) {
      case types::U8:  return BuiltinFunction::PrintUnsigned8;
      case types::S8:  return BuiltinFunction::PrintSigned8;
      case types::U16: return BuiltinFunction::PrintUnsigned16;
      case types::S16: return BuiltinFunction::PrintSigned16;
      default: std::unreachable();
      }
      std::unreachable();
    }();

    _usedBuiltinFunctions.insert(func);
    callFunctionImpl(builtinFunctionName(func), {}, { val }, API_FWD);
  }
  else if (types::isString(val.type())) {
    // TODO: investigate if this should be a builtin function call as well
    printString(val);
  }
  else {
    API_REQUIRE(false, error::ErrorCode::NotPrintable, "print is not supported for values of type '", val.type()->str(), "'.");
  }
}

void Assembler::printDecimal(Expression expr) {
  assert(types::isInteger(expr.type()));
  
  if (expr.hasSlot()) {
    Slot const slot = materialize(expr.slot());
    printDecimalSlot(slot);
    return;
  }

  int const x = literal::cast<types::IntegerType>(expr.literal())->semanticValue();
  printDecimalConst(x);
}

void Assembler::printDecimalConst(int value) {
  printStringConst(std::to_string(value));
}

void Assembler::printDecimalSlot(Slot slot) {
  assert(types::isInteger(slot.type()));

  if (types::isUnsignedInteger(slot.type())) return printDecimalSlotUnsigned(slot);
  if (types::isSignedInteger(slot.type())) return printDecimalSlotSigned(slot);
  std::unreachable();
}

void Assembler::printDecimalSlotUnsigned(Slot slot, bool const destroySlot) {
  assert(types::isUnsignedInteger(slot.type()));

  bool freeValSlot = false;
  Slot valSlot = [&] {
    if (destroySlot) return slot;
    Slot const copy = getTemp(slot.type());
    assignSlot(copy, slot);
    freeValSlot = true;
    return copy;
  }();

  pushPtr();
  
  // We can destroy the contents of valSlot while working on it.
  int const maxDigits = valSlot.type()->usesValue1() ? 5 : 3;
	  
  Slot digits = getTemp(ts::raw(maxDigits));
  for (int i = 0; i != maxDigits; ++i) {
    Slot const currentDigitSlot = digits.sub(ts::u8(), i);
    divSlotByConst(valSlot, 10, currentDigitSlot);
  }
	
  // Already add '0' to the 1's digit (at the base) to make sure there is at least 1 nonzero.
  // Plant a marker at the base of the array.
  moveTo(digits);
  addConst('0');	  
  setSeekMarker();
	
  // Start at right-most digit and move left until first nonzero is found.
  moveTo(digits + maxDigits - 1);
  seek(MacroCell::Value0, primitive::Left, {}, true);

  // Now just print every character until the start of the string has been reached (seekMarker).
  // Don't print the 1's digit in the loop.
  switchField(MacroCell::SeekMarker);
  notConstructive(Cell{_dp.current().offset, MacroCell::Flag},
		  Temps<1>::select(_dp.current().offset, MacroCell::Scratch0));
  switchField(MacroCell::Flag);
  loopOpen(); {
    zeroCell();

    switchField(MacroCell::Value0);
    addConst('0');
    emit<primitive::Out>();
	    
    emit<primitive::MovePointerRelative>(-1 * MacroCell::FieldCount);
    switchField(MacroCell::SeekMarker);
    notConstructive(Cell{_dp.current().offset, MacroCell::Flag},
		    Temps<1>::select(_dp.current().offset, MacroCell::Scratch0));
    switchField(MacroCell::Flag);
  } loopClose();

  // At seekmarker -> print 1's digit and clear seekMarker
  switchField(MacroCell::Value0);
  emit<primitive::Out>();
  resetSeekMarker();

  // Rebase datapointer to the start of the digits-array
  _dp.set(digits, MacroCell::Value0);

  popPtr();
  freeTempSlot(digits);
  if (freeValSlot) freeTempSlot(valSlot);
}

void Assembler::printDecimalSlotSigned(Slot slot) {
  assert(types::isSignedInteger(slot.type()));

  Slot const valSlot = getTemp(slot.type());
  assignSlot(valSlot, slot);
  
  pushPtr();
  // Construct sign bit in the flag and use that to determine whether to print a - sign.
  moveTo(valSlot, valSlot.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitConstructive(Cell{valSlot, MacroCell::Flag},
		      Temps<4>::select(valSlot, MacroCell::Scratch0,
				       valSlot, MacroCell::Scratch1,
				       valSlot, MacroCell::Payload0,
				       valSlot, MacroCell::Payload1));

  moveTo(valSlot, MacroCell::Flag);
  loopOpen(); {
    // Negate valSlot while we're here
    negateSlot(valSlot);
    setToValue('-', Temps<1>::select(valSlot, MacroCell::Scratch0));
    emit<primitive::Out>();
    zeroCell();
  } loopClose();
  popPtr();

  printDecimalSlotUnsigned(valSlot.unsignedView(), true);
  // valSlot will already be freed
}

void Assembler::printString(Expression expr) {
  assert(types::isString(expr.type()));
  
  if (expr.hasSlot()) {
    Slot const slot = materialize(expr.slot());
    printStringSlot(slot);
    return;
  }
  assert(expr.isLiteral());
  
  std::string const &str = literal::cast<types::StringType>(expr.literal())->stdstr();
  printStringConst(str);
}

void Assembler::printStringConst(std::string const &str) {

  if (str.size() == 0) return;
  
  Slot ch = getTemp(ts::u8());

  pushPtr();
  moveTo(ch);
  setToValue(str[0], Temps<1>::select(ch, MacroCell::Scratch0));
  emit<primitive::Out>();
  for (size_t i = 1; i != str.size(); ++i) {
    int const diff = str[i] - str[i - 1];
    emit<primitive::ChangeBy>(diff);
    emit<primitive::Out>();
  }
  zeroCell();
  popPtr();

  freeTempSlot(ch);
}
 
void Assembler::printStringSlot(Slot slot) {
  assert(types::isString(slot.type()));

  pushPtr();
  moveTo(slot, MacroCell::Value0);
  setSeekMarker();

  emit<primitive::CopyData>(MacroCell::Value0, MacroCell::Flag, MacroCell::Scratch0);
  switchField(MacroCell::Flag);
  loopOpen(); {
    zeroCell();
    switchField(MacroCell::Value0);
    emit<primitive::Out>();
    emit<primitive::MovePointerRelative>(MacroCell::FieldCount);

    // Check if end of string was reached by using the current value as a flag.
    // If NULL terminator hit, we exit the loop and go back to start.
    emit<primitive::CopyData>(MacroCell::Value0, MacroCell::Flag, MacroCell::Scratch0);
    switchField(MacroCell::Flag);
  } loopClose();

  // We hit the end of the string -> return to seek marker (no payload, check current as well)
  seek(MacroCell::SeekMarker, primitive::Left, {}, true);
  resetSeekMarker();
  popPtr();
}
