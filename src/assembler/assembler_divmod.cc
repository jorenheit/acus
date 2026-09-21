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
				 ws::Layout<ws::Data, ws::ZeroCells<4>>{}));

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
				 ws::Layout<ws::Data, ws::ZeroCells<4>>{}));

  // Take absolute value of rhsCopy and set/adjust resultNegative flag if necessary
  {
    Cell const signBitFlag = {rhsCopy, MacroCell::Scratch0};
    loop(signBitFlag, [&] {
      zeroCell(signBitFlag);
      negateSlot(rhsCopy);

      moveTo(resultNegativeFlag);
      notDestructive(Cell{resultNegative, MacroCell::Scratch0});
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
  signBitDestructive(ws::promise(S, ws::Layout<ws::Data, ws::ZeroCells<5>>{}));

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
    moveTo(SCopy1);
    notDestructive(Temps<1>(S)); // Reuse S, already zero.
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
				 ws::Layout<ws::Data, ws::ZeroCells<4>>{}));

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
  signBitDestructive(ws::promise(S, ws::Layout<ws::Data, ws::ZeroCells<5>>{}));

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
      moveTo(SCopy2);
      notDestructive(Temps<1>{SCopy1}); // Reuse S2, already zero.
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


Assembler::DivModResult Assembler::divModDestructive(DivModNum const &num, SingleCell const &denom, TransferMode rhsMode) {
  auto v = num.view("N", "D", "CopyTemp", "", "", "", "ZeroFlag");

  pushPtr();

  // Bring the denominator into this workspace.
  copyOrMoveField(rhsMode, denom, v["D"], v["CopyTemp"]);

  // Reuse the CopyTemp field for DTest and pick a new CopyTemp field
  v.rename("CopyTemp", "DTest");
  v.rename<3>("CopyTemp");
  copyField(v["D"], v["DTest"], v["CopyTemp"]);

  // Assume denominator == 0.
  inc(v["ZeroFlag"]);
  loop(v["DTest"], [&] {
    // If D != 0
    zeroCell(v["DTest"]);
    dec(v["ZeroFlag"]);

    auto prepared = ws::promise(v["N"], ws::Layout<
				ws::Prepared<ws::Role::NumeratorLow>,
				ws::Prepared<ws::Role::DenominatorLow>,
				ws::ZeroCells<4>>{});

    v = divModDestructiveKernel(prepared).view("Q", "R"); 
  });

  loop(v["ZeroFlag"], [&] {
    // Else D == 0
    dec(v["ZeroFlag"]);

    // quotient = 0xff, remainder = 0
    zeroCell(v["Q"]); dec(v["Q"]);
    zeroCell(v["R"]);
  });

  popPtr();

  return ws::promise(
    num.start(),
    ws::Layout<
      ws::Prepared<ws::Role::QuotientLow>,
      ws::Prepared<ws::Role::RemainderLow>,
      ws::ZeroCells<5>
    >{}
  );
}

Assembler::DivModResult Assembler::divModDestructiveKernel(DivModPrepared const &prep) {

  auto const [N, D, Q, CopyTemp, DCopy, RestoreFlag] = prep.cells<6>();

  pushPtr();

  // Initial layout:
  //
  // N | D | Q | CopyTemp | DCopy | RestoreFlag
  // n | d | 0 |    0     |   0   |     0

  // Preserve D and initialize the restore flag.
  copyField(D, DCopy, Q);
  inc(RestoreFlag);

  // N | D | Q | CopyTemp | DCopy | RestoreFlag
  // n | d | 0 |    0     |   d   |     1

  loop(N, [&] {
    // Consume one numerator unit and one denominator unit.
    // Q is incremented provisionally; the raw fragment undoes that
    // increment when D has not yet reached zero.
    dec(N);
    inc(Q);
    dec(D);

    // If D is still nonzero, undo the provisional Q increment.
    // Both control paths synchronize back on D.
    literalBf(D,
              "[>->]"    // D != 0: --Q and land on the zero CopyTemp cell
              ">>[-<<]"  // D != 0: clear RestoreFlag and return to CopyTemp
              "<<");     // both paths converge back on D

    // If D reached zero, RestoreFlag is still set.
    // Restore D from its persistent copy.
    loop(RestoreFlag, [&] {
      dec(RestoreFlag);
      copyField(DCopy, D, CopyTemp);
    });

    // Prepare the flag for the next iteration.
    inc(RestoreFlag);
  });

  // The outer loop has finished; this flag is no longer needed.
  dec(RestoreFlag);

  // Current layout:
  //
  // N | D | Q   | CopyTemp | DCopy | RestoreFlag
  // 0 | c | n/d |    0     |   d   |     0
  //
  // remainder = d - c
  subDestructive(DCopy, D);

  // D and N are now both zero, so place the final results there.
  moveField(DCopy, D); // TODO: known zero move optimization
  moveField(Q, N);

  // Final layout:
  //
  // Q | R | 0 | 0 | 0 | 0
  popPtr();

  return ws::promise(
    prep.start(),
    ws::Layout<
      ws::Prepared<ws::Role::QuotientLow>,
      ws::Prepared<ws::Role::RemainderLow>,
      ws::ZeroCells<4>
    >{}
  );
}

Assembler::DivMod16DigitResult Assembler::divMod16Digit(DivMod16Num const &num, DivMod16Denom const &den) {

  using Dec17Operand = ws::Workspace<ws::DataCells<3>,
				     ws::Zero,
				     ws::DoNotTouch,
				     ws::ZeroCells<2>>;
  
  auto const dec17 = [&](Dec17Operand const &op) -> Dec17Operand {
    auto const [low, high, guard, sentinel, _, highBorrow, lowBorrow] = op.cells();
    
    pushPtr();

    // Start by assuming that both lower bytes will borrow.
    // These flags are cleared below when that assumption proves false.
    inc(highBorrow);
    inc(lowBorrow);

    // Speculatively propagate borrow through high into guard.
    dec(guard);
    literalBf(high, "[>+>]"    // high != 0: cancel guard borrow
	            ">>[-<<]"  // clear highBorrow and sync pointer on sentinel
	            "<<-");     // return from sentinel to high and decrement that

    // If low != 0, no borrow was necessary at all: restore high and,
    // if necessary, guard.
    literalBf(low, "[>+>>>>[-<<<+>>>]<<]" // low != 0: restore high; if highBorrow, restore guard
	           ">>>[-<<<]"            // clear lowBorrow if set and synchronize on sentinel
	           "<<<-");                // return from sentinel to low and decrement that

    zeroCell(highBorrow);
    zeroCell(lowBorrow);
    popPtr();
    return op;
  };

  pushPtr();

  [[maybe_unused]] auto const [Rlo, Rhi, G, scratch, Qinitial, Qfinal] = num.cells<6>();
  auto const [Dlo, Dhi, DloCopy, DhiCopy, CopyTemp] = den.cells<5>();

  // Preserve the denominator. The subtraction loop consumes Dlo/Dhi
  // on every iteration, so these copies are restored afterwards.
  copyField(Dlo, DloCopy, DhiCopy);
  copyField(Dhi, DhiCopy, CopyTemp);


  // Q starts at -1 because the loop performs one subtraction too many.
  dec(Qinitial);

  // Extra 17th remainder bit.
  inc(G);
  loop(G, [&] {
    // 17-bit subtraction: Rlo:Rhi:G -= Dlo:Dhi
    loop(Dlo, [&] {
      dec(Dlo);
      dec17(ws::promise(Rlo, ws::Layout<
			ws::DataCells<3>,
			ws::Zero,
			ws::DoNotTouch, // Qinitial
			ws::ZeroCells<2>>{}));
    });

    loop(Dhi, [&] {
      dec(Dhi);
      dec16(ws::promise(Rhi, ws::Layout<
			ws::DataCells<2>,
			ws::Zero,
			ws::DoNotTouch, // Qinitial
			ws::ZeroCells<2>>{}));
    });

    // Restore denominator for the next subtraction.
    copyField(DloCopy, Dlo, CopyTemp);
    copyField(DhiCopy, Dhi, CopyTemp);

    inc(Qinitial);
  });

  // Copies are no longer necessary. Dlo/Dhi themselves are currently
  // restored and will be consumed by the final add-back.
  zeroCell(DloCopy);
  zeroCell(DhiCopy);

  // Move Q from cell 4 to its final position at cell 5.
  moveField(Qinitial, Qfinal);
  
  auto currentRemainder =
    ws::promise(Rlo, ws::Layout<
		ws::Prepared<ws::Role::RemainderLow>,
		ws::Prepared<ws::Role::RemainderHigh>,
		ws::ZeroCells<3>,
		ws::DoNotTouch // Q
		>{});

  // The subtraction loop deliberately overshot by one denominator,
  // so add D back to the remainder.  
  add16Destructive(currentRemainder, den);
  popPtr();


  return ws::promise(num.start(), ws::Layout<
		     ws::Prepared<ws::Role::RemainderLow>,
		     ws::Prepared<ws::Role::RemainderHigh>,
		     ws::ZeroCells<3>,
		     ws::Prepared<ws::Role::QuotientLow>,
		     ws::ZeroCells<1>
		     >{});
}

Assembler::DivMod16Result Assembler::divMod16Destructive(DivMod16Num const &num, DivMod16Denom const &den) {
  Slot const tmpSlot = getTemp(ts::raw(1));
  auto const tmp = ws::promiseClean16(tmpSlot);

  pushPtr();

  auto nv = num.view("Nlo", "Nhi", "CopyTemp");
  auto dv = den.view("Dlo", "Dhi", "CopyTemp");
  auto tv = tmp.view("DloCopy", "DhiCopy", "ElseFlag");

  copyField(dv["Dlo"], tv["DloCopy"], dv["CopyTemp"]);
  copyField(dv["Dhi"], tv["DhiCopy"], dv["CopyTemp"]);

  inc(tv["ElseFlag"]);
  loop(tv["DhiCopy"], [&]{
    zeroCell(tv["DhiCopy"]);
    dec(tv["ElseFlag"]);

    nv = divMod16Digit(num, den)
      .view("Rlo", "Rhi", ws::At<5>{"Qlo"});

    // Move remainder to cells 2 and 3 and quotient to cell 0
    moveField(nv["Rlo"], nv[2]);
    moveField(nv["Rhi"], nv[3]);
    moveField(nv["Qlo"], nv[0]);

    nv.reset(); // Reset to initial name-state for the else-branch
  });

  loop(tv["ElseFlag"], [&] {

    loop(tv["DloCopy"], [&]{
      // If Dlo != 0

      zeroCell(tv["DloCopy"]);
      zeroCell(tv["ElseFlag"]);

      // Prepare tmp for 8-bit division Nhi / Dlo
      // All tmp cells have already been cleared at this point
      tv.renameAll("Nhi", "", "");
      moveField(nv["Nhi"], tv["Nhi"]);

      // Calulate Nhi / Dlo
      tv = divModDestructive(ws::promiseClean8(tv["Nhi"]), dv["Dlo"], TransferMode::Copy)
	.view("Qhi", "Carry");

      // The Qhi that was returned by the 8-bit algorithm is already final -> move into final position
      moveField(tv["Qhi"], nv[1]);

      // Move Nlo into the tmp workspace to prepare for the calculation of
      // (Nlo:Carry) / Dlo
      tv.renameAll("Nlo", "Carry");
      moveField(nv["Nlo"], tv["Nlo"]);
      tv = divMod16Digit(ws::promiseClean16(tv["Nlo"]), ws::promiseClean16(dv["Dlo"]))
	.view("Rlo", "Rhi", ws::At<5>("Qlo"));

      nv.renameAll("Qlo", "Qhi", "Rlo", "Rhi");
      moveField(tv["Rlo"], nv["Rlo"]);
      moveField(tv["Rhi"], nv["Rhi"]);
      moveField(tv["Qlo"], nv["Qlo"]);

      // tv[2] is left empty and becomes the ElseFlag again
      tv.renameAll("", "", "ElseFlag");
    });

    loop(tv["ElseFlag"], [&] {
      // Else Dlo == 0
      zeroCell(tv["ElseFlag"]);

      // quotient = 0xffff
      zeroCell(nv["Qlo"]); dec(nv["Qlo"]);
      zeroCell(nv["Qhi"]); dec(nv["Qhi"]);
    });
  });

  popPtr();
  freeSlot(tmpSlot);

  return ws::promise(num.start(), ws::Layout<
		     ws::Prepared<ws::Role::QuotientLow>,
		     ws::Prepared<ws::Role::QuotientHigh>,
		     ws::Prepared<ws::Role::RemainderLow>,
		     ws::Prepared<ws::Role::RemainderHigh>,
		     ws::ZeroCells<3>> {});
}
