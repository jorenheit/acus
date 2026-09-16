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

    moveTo(lhs, MacroCell::Value0);
    divMod16Destructive(Cell{rhsWork, MacroCell::Value0});

    if (modSlot.has_value()) {
      moveTo(lhs, MacroCell::Scratch0);
      moveField(Cell{*modSlot, MacroCell::Value0});
      moveTo(lhs, MacroCell::Scratch1);
      moveField(Cell{*modSlot, MacroCell::Value1});
    } else {
      moveTo(lhs, MacroCell::Scratch0);
      zeroCell();
      moveTo(lhs, MacroCell::Scratch1);
      zeroCell();
    }

    if (freeRhsWork)
      freeTempSlot(rhsWork);
  } else {
    moveTo(lhs, MacroCell::Value0);
    divModDestructive(Cell{rhs, MacroCell::Value0},
		      destroyRhs ? TransferMode::Move : TransferMode::Copy);

    moveTo(lhs, MacroCell::Value1);
    if (modSlot.has_value()) {
      moveField(Cell{*modSlot, MacroCell::Value0});
    } else {
      zeroCell();
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
  moveTo(lhs, lhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitConstructive(Cell{lhs, MacroCell::Flag},
		      Temps<3>::select(lhs, MacroCell::Scratch0,
				       lhs, MacroCell::Scratch1,
				       lhs, MacroCell::Payload0));

  Slot tmp = getTemp(ts::raw(2));
  Cell const resultNegative { tmp, MacroCell::Flag };
  moveTo(lhs, MacroCell::Flag);
  loopOpen(); {    
    // lhs < 0  ==>  negate LHS and set negative flag
    zeroCell();      
    negateSlot(lhs);
    moveTo(resultNegative);
    zeroCell(); inc();
    moveTo(lhs, MacroCell::Flag);
  } loopClose();


  Slot const rhsCopy = tmp.sub(rhs.type(), 1);
  assignSlot(rhsCopy, rhs);
  
  moveTo(rhsCopy, rhsCopy.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitConstructive(Cell{rhsCopy, MacroCell::Flag},
		      Temps<3>::select(rhsCopy, MacroCell::Scratch0,
				       rhsCopy, MacroCell::Scratch1,
				       rhsCopy, MacroCell::Payload0));

  
  moveTo(rhsCopy, MacroCell::Flag);
  loopOpen(); {
    zeroCell();      
    negateSlot(rhsCopy);
    // rhs < 0  ==> set tmp flag only if it was not already set and negate rhs
    moveTo(resultNegative);
    notDestructive(Cell{tmp, MacroCell::Scratch0});
    moveTo(rhsCopy, MacroCell::Flag);
  } loopClose();

  // Both operands are now positive and the resultNegative cell holds the sign bit for the result.  
  divSlotBySlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), modSlot, true);
  
  // Correct the sign
  moveTo(resultNegative);
  loopOpen(); {
    zeroCell();
    negateSlot(lhs);
  } loopClose();

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

  Slot signBit = getTemp(ts::u8());
  moveTo(lhs, lhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  copyField(Cell{signBit, MacroCell::Value0},
	    Temps<1>::select(signBit, MacroCell::Scratch0));
  moveTo(signBit, MacroCell::Value1);
  zeroCell(); // needs explicit zero (not a scratch field)
  moveTo(signBit, MacroCell::Value0);
  signBitDestructive();

  
  // Copy signbit to 2 adjacent cells so we have 3 copies in total
  auto constexpr S1 = MacroCell::Value0; // holds sign bit currently
  auto constexpr S2 = static_cast<MacroCell::Field>(S1 + 1); // copy 1
  auto constexpr S3 = static_cast<MacroCell::Field>(S1 + 2); // copy 2 (only if modresult is needed)

  if (modSlot) {
    // 2 copies
    emit<primitive::Inline>("[->+>+>+<<<]>>>[-<<<+>>>]<<<");
  } else {
    // 1 copy
    emit<primitive::Inline>("[->+>+<<]>>[-<<+>>]<<");
  }
  
  // If the lhs was negative, negate it before passing it to the unsigned algorithm
  // Use first signbit-copy
  moveTo(signBit, S1);
  loopOpen(); {
    zeroCell();
    negateSlot(lhs);
  } loopClose();

  divSlotByConstUnsigned(lhs.unsignedView(), std::abs(denom), modSlot);

  // Fix div sign
  moveTo(signBit, S2);
  if (denom < 0) {
    notDestructive(Temps<1>::select(signBit, S1)); // Reuse S1 (already zero by this point)
  }
  loopOpen(); {
    zeroCell();
    negateSlot(lhs);
  } loopClose();

  // Fix mod sign
  if (modSlot) {
    // Need the mod-result -> has same sign as lhs (= signbit)
    moveTo(signBit, S3);
    loopOpen(); {
      zeroCell();
      negateSlot(*modSlot);
    } loopClose();
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

    moveTo(lhs, MacroCell::Value0);
    divMod16Destructive(Cell{rhsWork, MacroCell::Value0});

    if (divSlot.has_value()) {
      moveTo(lhs, MacroCell::Value0);
      moveField(Cell{*divSlot, MacroCell::Value0});
      moveTo(lhs, MacroCell::Value1);
      moveField(Cell{*divSlot, MacroCell::Value1});
    }

    moveTo(lhs, MacroCell::Scratch0);
    moveField(Cell{lhs, MacroCell::Value0});
    moveTo(lhs, MacroCell::Scratch1);
    moveField(Cell{lhs, MacroCell::Value1});

    if (freeRhsWork)
      freeTempSlot(rhsWork);
    
  } else {
    moveTo(lhs, MacroCell::Value0);
    divModDestructive(Cell{rhs, MacroCell::Value0},
		      destroyRhs ? TransferMode::Move : TransferMode::Copy);

    if (divSlot.has_value()) {
      moveField(Cell{*divSlot, MacroCell::Value0});
    }
    
    moveTo(lhs, MacroCell::Value1);
    moveField(Cell{lhs, MacroCell::Value0});
  }

  popPtr();
}

void Assembler::modSlotBySlotSigned(Slot lhs, Slot rhs, std::optional<Slot> const &divSlot) {
  assert(types::isSignedInteger(lhs.type()));
  assert(types::isSignedInteger(rhs.type()));

  pushPtr();

  // For signed integers, the sign of the result is equal to the sign of the LHS
  moveTo(lhs, lhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitConstructive(Cell{lhs, MacroCell::Flag},
		      Temps<3>::select(lhs, MacroCell::Scratch0,
				       lhs, MacroCell::Scratch1,
				       lhs, MacroCell::Payload0));

  Slot tmp = getTemp(ts::raw(2));
  Cell const resultNegative { tmp, MacroCell::Flag };
  moveTo(lhs, MacroCell::Flag);
  loopOpen(); {
    moveTo(resultNegative);
    zeroCell(); inc();
    negateSlot(lhs);
    moveTo(lhs, MacroCell::Flag);
    zeroCell();      
  } loopClose();


  Slot const rhsCopy = tmp.sub(rhs.type(), 1);
  assignSlot(rhsCopy, rhs);
  if (types::isSignedInteger(rhs.type())) {
    absSlot(rhsCopy);
  }

  modSlotBySlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), divSlot, true);
  
  moveTo(resultNegative);
  loopOpen(); {
    zeroCell();
    negateSlot(lhs);
  } loopClose();

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

  // Copy the lhs into a temp and reduce it to its sign bit
  Slot signBit = getTemp(ts::u8());
  moveTo(lhs, lhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  copyField(Cell{signBit, MacroCell::Value0},
	    Temps<1>::select(signBit, MacroCell::Scratch0));
  moveTo(signBit, MacroCell::Value1);
  zeroCell(); // needs explicit zero (not a scratch field)
  moveTo(signBit, MacroCell::Value0);
  signBitDestructive();

  // Copy signbit to 1 or 2 adjacent cells so we have 2 or 3 copies in total
  auto constexpr S1 = MacroCell::Value0; // holds sign bit currently
  auto constexpr S2 = static_cast<MacroCell::Field>(S1 + 1); // copy 1
  auto constexpr S3 = static_cast<MacroCell::Field>(S1 + 2); // copy 2 (only if modresult is needed)

  if (divSlot) {
    // 2 copies
    emit<primitive::Inline>("[->+>+>+<<<]>>>[-<<<+>>>]<<<");
  } else {
    // 1 copy
    emit<primitive::Inline>("[->+>+<<]>>[-<<+>>]<<");
  }
  
  // If the lhs was negative, negate it before passing it to the unsigned algorithm
  // Use first signbit-copy
  moveTo(signBit, S1);
  loopOpen(); {
    zeroCell();
    negateSlot(lhs);
  } loopClose();

  modSlotByConstUnsigned(lhs.unsignedView(), std::abs(denom), divSlot);

  // Fix mod sign (same sign as lhs)
  moveTo(signBit, S2);
  loopOpen(); {
    zeroCell();
    negateSlot(lhs);
  } loopClose();

  // Fix div sign
  if (divSlot) {
    moveTo(signBit, S3);
    if (denom < 0) {
      notDestructive(Temps<1>::select(signBit, S2)); // Reuse S2 (already zero by this point)
    }
    loopOpen(); {
      zeroCell();
      negateSlot(*divSlot);
    } loopClose();
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


// Implementations of the divmod algorithms

void Assembler::divModDestructive(Cell denom, TransferMode rhsMode) {
  // Prepare the current cell and pass it to divModDestructiveKernel
  assert(_dp.current().field == MacroCell::Value0);
  const auto current = _dp.current();
  pushPtr();
  
  moveTo(denom);
  copyOrMoveField(rhsMode, Cell{current, MacroCell::Value1},
		  Temps<1>::select(current, MacroCell::Scratch0));

  // Make a disposable copy of D in Scratch0 so we can branch on
  // D != 0 without consuming the actual denominator.
  moveTo(current, MacroCell::Value1);
  copyField(Cell{current, MacroCell::Scratch0},
	    Temps<1>::select(current, MacroCell::Scratch1));

  // Assume D == 0.
  // Payload1 is not touched by the normal 8-bit kernel.
  auto constexpr ZeroFlag = MacroCell::Payload1;
  moveTo(current, ZeroFlag);
  inc();

  // D != 0
  moveTo(current, MacroCell::Scratch0);
  loopOpen(); {
    zeroCell();

    // Not the zero-denominator path.
    moveTo(current, ZeroFlag);
    zeroCell();
  
    moveTo(current, MacroCell::Value0);
    divModDestructiveKernel();
    moveTo(current, MacroCell::Scratch0);
  } loopClose();

  // D == 0
  moveTo(current, ZeroFlag);
  loopOpen(); {
    zeroCell();

    // quotient = 0xff
    moveTo(current, MacroCell::Value0);
    zeroCell();
    dec();

    // remainder = 0
    switchField(MacroCell::Value1);
    zeroCell();

    moveTo(current, ZeroFlag);
  } loopClose();

  popPtr();
  
}

void Assembler::divModDestructiveKernel() {

  // This algorithm assumes that the cell pointed to is the first in
  // an already prepared block. When calculating n/d, the memory
  // layout at this point should be:
  //
  // N | D | R | S1 | S2 | S3
  // n | d | 0 | 0  | 0  | 0
  
  const auto current = _dp.current();
  const auto N = MacroCell::Value0;
  const auto D = static_cast<MacroCell::Field>(N + 1);
  const auto R = static_cast<MacroCell::Field>(N + 2);
  const auto S1 = static_cast<MacroCell::Field>(N + 3);
  const auto S2 = static_cast<MacroCell::Field>(N + 4);
  const auto S3 = static_cast<MacroCell::Field>(N + 5);

  assert(current.field == N);
  pushPtr();
  
  // Further prepare the memory, such that S2 holds a copy of d and
  // S3 is set:
  // N | D | R | S1 | S2 | S3
  // n | d | 0 | 0  | d  | 1

  // Move or copy the denominator cell into both D and S2, depending on the
  // transfer mode for the rhs (= denominator).

  // Copy D into S2 and set S3
  moveTo(current, D);
  emit<primitive::Inline>("[->+>>+<<<]>[-<+>]>>>+<<<<");

  switchField(N);
  loopOpen(); {
  // First, unconditionally decrement N and D, while incrementing R
    dec();
    switchField(R); inc();
    switchField(D); dec();

    // Now, if D is still nonzero, we undo the increment of R and move two 
    // cells to the right. This lands us on S1 if D != 0 or D if D == 0. Sync
    // the pointer locations by moving another two cells and conditionally
    // moving back from S3 if that was hit. We also reset S3 in the process
    // and end back on D (which is where the compiler thinks we are).
    emit<primitive::Inline>("[>->]>>[-<<]<<");

    // We're now always at D. If D was zero, S3 is still set, which we use
    // to decide if we need to restore D from its copy in S2.
    switchField(S3);
    loopOpen(); {
      dec();
      switchField(S2);
      copyField(Cell{current, D}, Temps<1>::select(current, S1));
      switchField(S3);
    } loopClose();
    inc(); // Restore S3 flag
  
    // Close the outer loop
    switchField(N);
  } loopClose();

  // Clear S3, not necessary anymore
  switchField(S3);
  dec();
  
  // At this point, the memory layout is:
  // N | D | R   | S1 | S2 | S3
  // 0 | c | n/d | 0  | d  | 0
  // Where the remainder is d - c -> construct this value in S2
  switchField(S2);
  subDestructive(Cell{current, D});

  // Now move the result back into N and the remainder into D. Both
  // are now known zeroes so we simply add to them.
  switchField(D);
  addDestructive(Cell{current, S2});
  switchField(N);
  addDestructive(Cell{current, R});
  
  popPtr();
}

void Assembler::divMod16DestructiveGuaranteed8BitResult(Cell denom) {
  // TODO: document contract for calling this function and the resulting
  // layout after it returns.
  
  assert(_dp.current().field == MacroCell::Value0);
  pushPtr();
  Cell const num = _dp.current();

  // Use the cells beyond the denominator as scratch space to hold a copy of D
  // This assumes that these cells are available as scratch
  auto constexpr D0 = MacroCell::Value0;
  auto constexpr D1 = static_cast<MacroCell::Field>(D0 + 1);
  auto constexpr D0c = static_cast<MacroCell::Field>(D0 + 2);
  auto constexpr D1c = static_cast<MacroCell::Field>(D0 + 3);
  auto constexpr Temp = static_cast<MacroCell::Field>(D0 + 4);
  
  moveTo(denom, D0);
  copyField(Cell{denom, D0c}, Temps<1>::select(denom, Temp));
  moveTo(denom, D1);
  copyField(Cell{denom, D1c}, Temps<1>::select(denom, Temp));

  auto constexpr R0 = MacroCell::Value0;
  auto constexpr R1 = static_cast<MacroCell::Field>(R0 + 1);
  auto constexpr G = static_cast<MacroCell::Field>(R0 + 2);
  //  auto constexpr S1 = static_cast<MacroCell::Field>(R0 + 3);
  auto constexpr S2 = static_cast<MacroCell::Field>(R0 + 4);
  //  auto constexpr S3 = static_cast<MacroCell::Field>(R0 + 5);
  //  auto constexpr S4 = static_cast<MacroCell::Field>(R0 + 6);

  // Both kernels initialize and clear their own synchronization flags.
  // DEC16 starts and ends on R1.
  // DEC17 starts and ends on R0.
  // Both leave S2 untouched.  
  static constexpr char const *DEC16 =
    ">>>>+<<<"
    "-<[>+>]>>[<<]>>-<<<<-";
  static constexpr char const *DEC17 =
    ">>>>>+>+<<<<"
    "-<[>+>]>>[-<<]<<"
    "-<[>+>>>>[-<<<+>>>]<<]"
    ">>>[-<<<]>>>[-]<[-]<<<<<-";

  moveTo(num, S2); dec(); // This will hold the quotient Q
  switchField(G); inc();
  loopOpen(); {
    // 17-bit subtraction: R|G -= D
    {
      // Subtract from the low byte, borrow from high and G
      moveTo(denom, D0);
      loopOpen(); {
	dec();
	// 17 bit dec, double borrow. First prepare S3 and S4 which will be used
	// as sync flags
	moveTo(num, R0);
	emit<primitive::Inline>(DEC17);
	moveTo(denom, D0);
      } loopClose();

      // Subtract high byte
      moveTo(denom, D1);
      loopOpen(); {
	dec();
	moveTo(num, R1);
	emit<primitive::Inline>(DEC16);
	moveTo(denom, D1);
      } loopClose();
    }

    // Restore D0 and D1
    moveTo(denom, D0c);
    copyField(Cell{denom, D0}, Temps<1>::select(denom, Temp));
    moveTo(denom, D1c);
    copyField(Cell{denom, D1}, Temps<1>::select(denom, Temp));
    
    // Increment Q and loop
    moveTo(num, S2);
    inc();
    switchField(G);
  } loopClose();

  switchField(R0);
  add16Destructive(Cell{denom, D0});

  // State at this point:
  // D0 | D1 | G | S1 | S2 | S3 | S4
  // R0 | R1 | 0 |  0 | Q | 0  | 0
  
  // Clear D0c and D1c (D0 and D1 already destroyed by this point
  moveTo(denom, D0c); zeroCell();
  moveTo(denom, D1c); zeroCell();

  popPtr();
}

void Assembler::divMod16Destructive(Cell denom) {
  assert(_dp.current().field == MacroCell::Value0);

  Cell const num = _dp.current();
  Slot const tmp = getTemp(ts::raw(1));

  pushPtr();

  // Preserve a copy of the denominator for branching.
  moveTo(denom, MacroCell::Value0);
  copyField(Cell{tmp, MacroCell::Value0},
	    Temps<1>::select(tmp, MacroCell::Scratch0));

  moveTo(denom, MacroCell::Value1);
  copyField(Cell{tmp, MacroCell::Value1},
	    Temps<1>::select(tmp, MacroCell::Scratch0));

  // ------------------------------------------------------------------
  // Branch 1: Dhi != 0
  //
  // Then D >= 256, so the complete quotient is guaranteed to fit
  // in one byte.
  // ------------------------------------------------------------------

  // Else flag for Dhi == 0.
  moveTo(tmp, MacroCell::Flag);
  inc();

  moveTo(tmp, MacroCell::Value1);
  loopOpen(); {
    // One-shot branch.
    zeroCell();

    // Disable Dhi == 0 branch.
    switchField(MacroCell::Flag);
    zeroCell();

    moveTo(num, MacroCell::Value0);
    divMod16DestructiveGuaranteed8BitResult(denom);

    // Helper returned:
    //
    // Rlo | Rhi | 0 | 0 | Qlo
    //
    // Canonical divmod layout:
    //
    // Qlo | Qhi | Rlo | Rhi
    //       (=0)

    moveTo(num, MacroCell::Value0);
    moveField(Cell{num, MacroCell::Scratch0});

    switchField(MacroCell::Value1);
    moveField(Cell{num, MacroCell::Scratch1});

    switchField(MacroCell::Flag);
    moveField(Cell{num, MacroCell::Value0});

    // Value1 is already zero => Qhi = 0.

    moveTo(tmp, MacroCell::Value1);
  } loopClose();

  // ------------------------------------------------------------------
  // Branch 2: Dhi == 0
  // ------------------------------------------------------------------

  moveTo(tmp, MacroCell::Flag);
  loopOpen(); {
    // Consume the Dhi==0 branch flag.
    zeroCell();

    // Preserve Dlo across the destructive first radix-256 stage.
    //
    // denom.Scratch0 is restored to zero again below.
    moveTo(denom, MacroCell::Value0);
    copyField(
        Cell{denom, MacroCell::Scratch0},
        Temps<1>::select(denom, MacroCell::Scratch1));

    // Assume Dlo == 0.
    moveTo(tmp, MacroCell::Flag);
    inc();

    // --------------------------------------------------------------
    // Dlo != 0
    // --------------------------------------------------------------

    // tmp.Value0 still contains the saved Dlo, so use it directly
    // as our one-shot condition.
    moveTo(tmp, MacroCell::Value0);
    loopOpen(); {
      zeroCell();

      // Not the zero-denominator case.
      moveTo(tmp, MacroCell::Flag);
      zeroCell();

      // First radix-256 quotient digit:
      //
      // Qhi, carry = Nhi / Dlo
      moveTo(num, MacroCell::Value1);
      moveField(Cell{tmp, MacroCell::Value0});

      moveTo(tmp, MacroCell::Value0);
      divModDestructive(
          Cell{denom, MacroCell::Value0},
          TransferMode::Move);

      // tmp:
      // Qhi | carry

      // Restore Dlo for the second stage.
      moveTo(denom, MacroCell::Scratch0);
      moveField(Cell{denom, MacroCell::Value0});

      // Qhi is already final.
      moveTo(tmp, MacroCell::Value0);
      moveField(Cell{num, MacroCell::Value1});

      // Bring down Nlo:
      //
      // partial = carry:Nlo
      moveTo(num, MacroCell::Value0);
      moveField(Cell{tmp, MacroCell::Value0});

      // Second quotient digit:
      //
      // Qlo, remainder = partial / Dlo
      moveTo(tmp, MacroCell::Value0);
      divMod16DestructiveGuaranteed8BitResult(
          Cell{denom, MacroCell::Value0});

      // tmp now:
      //
      // Rlo | Rhi | 0 | 0 | Qlo

      moveTo(tmp, MacroCell::Value0);
      moveField(Cell{num, MacroCell::Scratch0});

      switchField(MacroCell::Value1);
      moveField(Cell{num, MacroCell::Scratch1});

      switchField(MacroCell::Flag);
      moveField(Cell{num, MacroCell::Value0});

      // tmp.Value0 is zero again, which also closes this
      // one-shot Dlo != 0 branch.
      moveTo(tmp, MacroCell::Value0);
    } loopClose();

    // --------------------------------------------------------------
    // Dlo == 0  => complete denominator == 0
    // --------------------------------------------------------------

    moveTo(tmp, MacroCell::Flag);
    loopOpen(); {
      zeroCell();

      // quotient = 0xffff
      moveTo(num, MacroCell::Value0);
      zeroCell();
      dec();

      switchField(MacroCell::Value1);
      zeroCell();
      dec();

      // remainder is already zero because Scratch0/Scratch1 have
      // not been touched on this path.

      moveTo(tmp, MacroCell::Flag);
    } loopClose();

    // Return to the outer Dhi==0 branch condition.
    moveTo(tmp, MacroCell::Flag);
  } loopClose();

  popPtr();
  freeSlot(tmp);
}
