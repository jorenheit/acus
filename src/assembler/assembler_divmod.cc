// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"


void Assembler::divSlotBySlotUnsigned(Slot lhs, Slot rhs, std::optional<Slot> const &modSlot, bool const destroyRhs) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));

  pushPtr();
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    bool freeRhsWork = false;
    Slot rhsWork = rhs;

    // Need a copy if rhs must survive, or if an 8-bit rhs must
    // be widened to the 16-bit representation expected by divMod16.
    if (!rhs.type()->usesValue1() || !destroyRhs) {
      rhsWork = getTemp(ts::u16());
      assignSlot(rhsWork, rhs,
                 destroyRhs ? TransferMode::Move : TransferMode::Copy);
      freeRhsWork = true;
    }

    [[maybe_unused]] auto const [qlo, qhi, rlo, rhi] =
      divMod16Destructive(ws::promiseClean16(lhs),
                          ws::promiseClean16(rhsWork)).cells<4>();

    if (modSlot.has_value()) {
      moveField(rlo, Cell{*modSlot, MacroCell::Value0});
      moveField(rhi, Cell{*modSlot, MacroCell::Value1});
    } else {
      zeroCell(rlo);
      zeroCell(rhi);
    }

    if (freeRhsWork)
      freeTempSlot(rhsWork);
  } else {
    [[maybe_unused]] auto const [quotient, remainder] =
      divModDestructive(ws::promiseClean8(lhs),
                        ws::promiseClean8(rhs),
                        destroyRhs ? TransferMode::Move : TransferMode::Copy).cells<2>();
    if (modSlot.has_value()) {
      moveField(remainder, Cell{*modSlot, MacroCell::Value0});
    } else {
      zeroCell(remainder);
    }
  }

  popPtr();
}

void Assembler::divSlotByConstUnsigned(Slot lhs, int denom, std::optional<Slot> const &modSlot) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(denom >= 0);
  
  if (denom == 0 || denom == 1) {
    if (modSlot.has_value()) {
      setSlotToValue(*modSlot, 0);
    }
    if (denom == 0) {
      setSlotToValue(lhs, 0xffff);
    }
    return;
  }
  
  Slot tmp = getTemp(denom > 0xff ? ts::u16() : ts::u8());
  setSlotToValue(tmp, denom);
  divSlotBySlotUnsigned(lhs, tmp, modSlot, true);
  freeTempSlot(tmp);
}

void Assembler::divSlotBySlotSigned(Slot lhs, Slot rhs, std::optional<Slot> const &modSlot) {
  assert(types::isSignedInteger(lhs.type()));
  assert(types::isSignedInteger(rhs.type()));

  pushPtr();

  // Construct sign-bit in lhs::Scratch0
  copyField(Cell{lhs, lhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0},
	    Cell{lhs, MacroCell::Scratch0},
	    Cell{lhs, MacroCell::Scratch1});
  
  signBitDestructive(ws::promise(Cell{lhs, MacroCell::Scratch0},
				 ws::Layout<ws::Data<>, ws::ScratchCells<4>>{}));

  Slot tmp = getTemp(ts::raw(2));
  Slot const resultNegative = tmp.sub(ts::u8(), 0);
  Slot const rhsCopy = tmp.sub(rhs.type(), 1);
  Cell const resultNegativeFlag = {tmp, MacroCell::Value0};
  zeroCell(resultNegativeFlag);
  
  // Take absolute value of lhs and set the resultNegative flag if necessary
  {
    Cell const signBitFlag = {lhs, MacroCell::Scratch0};
    loop(signBitFlag, [&] {
      // lhs < 0  ==> negate LHS and set negative flag
      zeroCell(signBitFlag);
      negateSlot(lhs);
      inc(resultNegativeFlag);
    });
  }


  // Construct sign-bit of rhs in rhsCopy::Scratch0
  assignSlot(rhsCopy, rhs);
  copyField(Cell{rhsCopy, rhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0},
	    Cell{rhsCopy, MacroCell::Scratch0},
	    Cell{rhsCopy, MacroCell::Scratch1});
  
  signBitDestructive(ws::promise(Cell{rhsCopy, MacroCell::Scratch0},
				 ws::Layout<ws::Data<>, ws::ScratchCells<4>>{}));

  // Take absolute value of rhsCopy and set/adjust resultNegative flag if necessary
  {
    Cell const signBitFlag = {rhsCopy, MacroCell::Scratch0};
    loop(signBitFlag, [&] {
      zeroCell(signBitFlag);
      negateSlot(rhsCopy);

      notDestructive(ws::promise(
        resultNegativeFlag,
        ws::Layout<ws::Data<>, ws::Untouched, ws::Scratch>{}
      ));
    });
  }

  // Both operands are now positive and resultNegative holds the sign bit.
  divSlotBySlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), modSlot, true);

  // Correct the sign.
  loop(resultNegativeFlag, [&] {
    zeroCell(resultNegativeFlag);
    negateSlot(lhs);
  });

  popPtr();
  freeTempSlot(tmp);
}

void Assembler::divSlotByConstSigned(Slot lhs, int denom, std::optional<Slot> const &modSlot) {
  assert(types::isSignedInteger(lhs.type()));

  if (denom == 0) {
    setSlotToValue(lhs, 0xffff);
    if (modSlot.has_value()) {
      setSlotToValue(*modSlot, 0);
    }
    return;
  }
  if (denom == 1 || denom == -1) {
    if (modSlot.has_value()) {
      setSlotToValue(*modSlot, 0);
    }
    if (denom == -1) {
      negateSlot(lhs);
    }
    return;
  }

  // For signed integers, check if the value is negative. If so, take the
  // absolute value but remember the sign.
  pushPtr();

  // Create a new slot and move the sign-byte to its Value1 field
  Slot signBit = getTemp(ts::raw(1));
  auto const [_, S, SCopy1, SCopy2] = ws::promiseClean16(signBit).cells<4>();
  
  Cell const lhsSignByte {
    lhs,
    lhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0
  };
  copyField(lhsSignByte, S, SCopy1);
  signBitDestructive(ws::promise(S, ws::Layout<ws::Data<>, ws::ScratchCells<5>>{}));

  // Copy sign bit to adjacent cells so we have enough independent copies.
  literalBf(S, modSlot
	       ? "[->+>+>+<<<]>>>[-<<<+>>>]<<<" // copy to SCopy1 and SCopy2
	       : "[->+>+<<]>>[-<<+>>]<<");      // only to SCopy1

  // If lhs was negative, negate it before passing it to the unsigned algorithm.
  loop(S, [&] {
    zeroCell(S);
    negateSlot(lhs);
  });

  divSlotByConstUnsigned(lhs.unsignedView(), std::abs(denom), modSlot);

  // Fix division sign.
  if (denom < 0) {
    // SCopy2 may still hold the original sign for the remainder, so keep it
    // untouched and borrow the next clean cell as scratch.
    notDestructive(ws::promise(
      SCopy1,
      ws::Layout<ws::Data<>, ws::Untouched, ws::Scratch>{}
    ));
  }
  
  loop(SCopy1, [&] {
    zeroCell(SCopy1);
    negateSlot(lhs);
  });

  // Fix remainder sign: it has the same sign as lhs.
  if (modSlot) {
    loop(SCopy2, [&] {
      zeroCell(SCopy2);
      negateSlot(*modSlot);
    });
  }

  popPtr();
  freeSlot(signBit);
}

void Assembler::modSlotBySlotUnsigned(Slot lhs, Slot rhs, std::optional<Slot> const &divSlot, bool const destroyRhs) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));

  pushPtr();

  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    bool freeRhsWork = false;
    Slot rhsWork = rhs;

    // Need a copy if rhs must survive, or if an 8-bit rhs must
    // be widened to the 16-bit representation expected by divMod16.
    if (!rhs.type()->usesValue1() || !destroyRhs) {
      rhsWork = getTemp(ts::u16());
      assignSlot(rhsWork, rhs,
                 destroyRhs ? TransferMode::Move
                            : TransferMode::Copy);
      freeRhsWork = true;
    }

    auto const [qlo, qhi, rlo, rhi] =
      divMod16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsWork)).cells<4>();

    if (divSlot.has_value()) {
      moveField(qlo, Cell{*divSlot, MacroCell::Value0});
      moveField(qhi, Cell{*divSlot, MacroCell::Value1});
    }

    moveField(rlo, qlo);
    moveField(rhi, qhi);

    if (freeRhsWork)
      freeTempSlot(rhsWork);

  } else {
    auto const [quotient, remainder] =
      divModDestructive(ws::promiseClean8(lhs),
			ws::promiseClean8(rhs),
			destroyRhs ? TransferMode::Move : TransferMode::Copy).cells<2>();;

    if (divSlot.has_value()) {
      moveField(quotient, Cell{*divSlot, MacroCell::Value0});
    }

    moveField(remainder, quotient);
  }

  popPtr();
}

void Assembler::modSlotBySlotSigned(Slot lhs, Slot rhs, std::optional<Slot> const &divSlot) {
  assert(types::isSignedInteger(lhs.type()));
  assert(types::isSignedInteger(rhs.type()));

  pushPtr();

  // Construct sign-bit in lhs::Scratch0
  copyField(Cell{lhs, lhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0},
	    Cell{lhs, MacroCell::Scratch0},
	    Cell{lhs, MacroCell::Scratch1});
  
  signBitDestructive(ws::promise(Cell{lhs, MacroCell::Scratch0},
				 ws::Layout<ws::Data<>, ws::ScratchCells<4>>{}));

  Slot tmp = getTemp(ts::raw(2));
  Slot const resultNegative = tmp.sub(rhs.type(), 0);
  Slot const rhsCopy = tmp.sub(rhs.type(), 1);

  Cell const resultNegativeFlag = {resultNegative, MacroCell::Value0};
  Cell const signBitFlag  = {lhs, MacroCell::Scratch0 };

  zeroCell(resultNegativeFlag);
  loop(signBitFlag, [&] {
    zeroCell(signBitFlag);
    inc(resultNegativeFlag);
    negateSlot(lhs);
  });

  assignSlot(rhsCopy, rhs);
  if (types::isSignedInteger(rhs.type())) {
    absSlot(rhsCopy);
  }

  modSlotBySlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), divSlot, true);

  loop(resultNegativeFlag, [&] {
    dec(resultNegativeFlag);
    negateSlot(lhs);
  });

  popPtr();
  freeTempSlot(tmp);
}

void Assembler::modSlotByConstUnsigned(Slot lhs, int denom, std::optional<Slot> const &divSlot) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(denom >= 0);

  if (denom == 0) {
    setSlotToValue(lhs, 0);
    if (divSlot.has_value()) {
      setSlotToValue(*divSlot, 0xffff);
    }
    return;
  }

  if (denom == 1) {
    if (divSlot.has_value()) {
      assignSlot(*divSlot, lhs);
    }
    setSlotToValue(lhs, 0);
    return;
  }
  
  Slot tmp = getTemp(denom > 0xff ? ts::u16() : ts::u8());
  setSlotToValue(tmp, denom);
  modSlotBySlotUnsigned(lhs, tmp, divSlot, true);
  freeTempSlot(tmp);
}


void Assembler::modSlotByConstSigned(Slot lhs, int denom, std::optional<Slot> const &divSlot) {
  assert(types::isSignedInteger(lhs.type()));

  if (denom == 0) {
    setSlotToValue(lhs, 0);
    if (divSlot.has_value())
      setSlotToValue(*divSlot, 0xffff);
    return;
  }

  if (denom == 1 || denom == -1) {
    if (divSlot.has_value()) {
      assignSlot(*divSlot, lhs);
      if (denom == -1)
        negateSlot(*divSlot);
    }

    setSlotToValue(lhs, 0);
    return;
  }

  // For signed integers, check if the value is negative. If so, take the
  // absolute value but remember the sign.
  pushPtr();

  // Copy lhs into a temp and reduce it to its sign bit.
  Slot signBit = getTemp(ts::u8());
  auto const [_, S, SCopy1, SCopy2] = ws::promiseClean8(signBit).cells<4>();

  Cell const lhsSignByte {
    lhs,
    lhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0
  };

  copyField(lhsSignByte, S, SCopy1);
  signBitDestructive(ws::promise(S, ws::Layout<ws::Data<>, ws::ScratchCells<5>>{}));

  // Copy sign bit to adjacent cells so we have enough independent copies.
  literalBf(S, divSlot
	       ? "[->+>+>+<<<]>>>[-<<<+>>>]<<<" // copy to SCopy1 and SCopy2
	       : "[->+>+<<]>>[-<<+>>]<<");      // only to SCopy1

  // If lhs was negative, negate it before passing it to the unsigned algorithm.
  loop(S, [&] {
    zeroCell(S);
    negateSlot(lhs);
  });

  modSlotByConstUnsigned(lhs.unsignedView(), std::abs(denom), divSlot);

  // Fix remainder sign (same sign as lhs).
  loop(SCopy1, [&] {
    zeroCell(SCopy1);
    negateSlot(lhs);
  });

  // Fix division sign.
  if (divSlot) {
    if (denom < 0) {
      notDestructive(ws::promise(
        SCopy2,
        ws::Layout<ws::Data<>, ws::Scratch>{}
      ));
    }
    loop(SCopy1, [&] {
      zeroCell(SCopy1);
      negateSlot(*divSlot);
    });
  }

  popPtr();
  freeSlot(signBit);
}

void Assembler::divSlotBySlot(Slot lhs, Slot rhs) {
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return divSlotBySlotUnsigned(lhs, rhs);
  if (types::isSignedInteger(lhs.type()))   return divSlotBySlotSigned(lhs, rhs);
  std::unreachable();
}

void Assembler::divSlotBySlot(Slot lhs, Slot rhs, Slot modSlot) {
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return divSlotBySlotUnsigned(lhs, rhs, modSlot);
  if (types::isSignedInteger(lhs.type()))   return divSlotBySlotSigned(lhs, rhs, modSlot);
  std::unreachable();
}

void Assembler::divSlotByConst(Slot lhs, int denom, Slot modSlot) {
  assert(types::isInteger(lhs.type()));
  
  if (types::isUnsignedInteger(lhs.type())) return divSlotByConstUnsigned(lhs, denom, modSlot);
  if (types::isSignedInteger(lhs.type())) return divSlotByConstSigned(lhs, denom, modSlot);
  std::unreachable();
}

void Assembler::divSlotByConst(Slot lhs, int denom) {
  assert(types::isInteger(lhs.type()));
  
  if (types::isUnsignedInteger(lhs.type())) return divSlotByConstUnsigned(lhs, denom);
  if (types::isSignedInteger(lhs.type())) return divSlotByConstSigned(lhs, denom);
  std::unreachable();
}


void Assembler::modSlotByConst(Slot lhs, int denom, Slot divSlot) {
  assert(types::isInteger(lhs.type()));
  if (types::isUnsignedInteger(lhs.type()))  return modSlotByConstUnsigned(lhs, denom, divSlot);
  if (types::isSignedInteger(lhs.type())) return modSlotByConstSigned(lhs, denom, divSlot);
  std::unreachable();
}

void Assembler::modSlotByConst(Slot lhs, int denom) {
  assert(types::isInteger(lhs.type()));
  if (types::isUnsignedInteger(lhs.type()))  return modSlotByConstUnsigned(lhs, denom);
  if (types::isSignedInteger(lhs.type())) return modSlotByConstSigned(lhs, denom);
  std::unreachable();
}

void Assembler::modSlotBySlot(Slot lhs, Slot rhs, Slot divSlot) {
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return modSlotBySlotUnsigned(lhs, rhs, divSlot);
  if (types::isSignedInteger(lhs.type()))   return modSlotBySlotSigned(lhs, rhs, divSlot);
  std::unreachable();
}

void Assembler::modSlotBySlot(Slot lhs, Slot rhs) {
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return modSlotBySlotUnsigned(lhs, rhs);
  if (types::isSignedInteger(lhs.type()))   return modSlotBySlotSigned(lhs, rhs);
  std::unreachable();
}




