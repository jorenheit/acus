// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::mulSlotByConst(Slot const &lhs, int factor) {
  assert(types::isInteger(lhs.type));
  
  if (types::isUnsignedInteger(lhs.type)) return mulSlotByConstUnsigned(lhs, factor);
  if (types::isSignedInteger(lhs.type)) return mulSlotByConstSigned(lhs, factor);
  std::unreachable();
}

void Assembler::mulSlotByConstUnsigned(Slot const &lhs, int factor) {
  assert(types::isUnsignedInteger(lhs.type));
  assert(factor >= 0);
  
  pushPtr();
  moveTo(lhs, MacroCell::Value0);    
  if (lhs.type->usesValue1()) {
    Slot const tmp = getTemp(ts::raw(1));
    mul16Const(factor, Cell{lhs, MacroCell::Value1},
	       Temps<8>::select(lhs, MacroCell::Scratch0,
				lhs, MacroCell::Scratch1,
				lhs, MacroCell::Payload0,
				lhs, MacroCell::Payload1,
				tmp, MacroCell::Scratch0,
				tmp, MacroCell::Scratch1,
				tmp, MacroCell::Payload0,
				tmp, MacroCell::Payload1));
    freeTempSlot(tmp);
  } else {
    mulConst(factor,
	     Temps<3>::select(lhs, MacroCell::Scratch0,
			      lhs, MacroCell::Scratch1,
			      lhs, MacroCell::Payload0));
  }
  
  popPtr();
}

void Assembler::mulSlotByConstSigned(Slot const &lhs, int factor) {
  assert(types::isSignedInteger(lhs.type));

  // For signed integers, check if the value is negative. If so, take the
  // absolute value but remember the sign.
  pushPtr();
  moveTo(lhs, lhs.type->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitConstructive(Cell{lhs, MacroCell::Flag},
		      Temps<4>::select(lhs, MacroCell::Scratch0,
				       lhs, MacroCell::Scratch1,
				       lhs, MacroCell::Payload0,
				       lhs, MacroCell::Payload1));

  Slot const tmp = getTemp(ts::raw(1));
  Cell const lhsNegative { tmp, MacroCell::Flag };
  moveTo(lhs, MacroCell::Flag);
  loopOpen(); {
    moveTo(lhsNegative);
    setToValue(1);
    negateSlot(lhs);
    moveTo(lhs, MacroCell::Flag);
    zeroCell();      
  } loopClose();

  // operand is now positive -> pass it to the unsigned version
  mulSlotByConstUnsigned(lhs.unsignedView(), std::abs(factor));

  moveTo(lhsNegative);
  if (factor < 0) {
    notDestructive(Cell{tmp, MacroCell::Scratch0});
  }
  loopOpen(); {
    negateSlot(lhs);
    moveTo(lhsNegative); zeroCell();
  } loopClose();

  popPtr();
  freeTempSlot(tmp);
}


void Assembler::mulSlotBySlot(Slot const &lhs, Slot const &rhs) {
  assert(types::isInteger(lhs.type));
  assert(types::isInteger(rhs.type));
  assert(types::cast<types::IntegerType>(lhs.type)->signedness() ==
	 types::cast<types::IntegerType>(rhs.type)->signedness());
    
  if (types::isUnsignedInteger(lhs.type)) return mulSlotBySlotUnsigned(lhs, rhs);
  if (types::isSignedInteger(lhs.type))   return mulSlotBySlotSigned(lhs, rhs);
  std::unreachable();
}


void Assembler::mulSlotBySlotUnsigned(Slot const &lhs, Slot const &rhs, bool const destroyRhs) {
  assert(types::isUnsignedInteger(lhs.type));
  assert(types::isUnsignedInteger(rhs.type));

  pushPtr();
  if (lhs.type->usesValue1() || rhs.type->usesValue1()) {
    
    /*
      a = a0 + a1 * 256
      b = b0 + b1 * 256
      c = a * b = a0 * b0 + 256 * (a0 * b1 + b0 * a1) + 256^2 * (a1 * b1)
      
      -> c0 = a0 * b0 (mod 256)
      -> c1 = (a0 * b1 + b0 * a1) (mod 256) + (a0 * b0 >> 8)

      Given a naive 8-bit and 16-bit algorithm that simply do repeated addition,
      we can therefore implement a 16 bit multiplication as follows:
       -> tmp1 = mul16(a0, b0)
       -> c0 = tmp1.low
       -> tmp2 = mul8(a0, b1) + mul8(a1, b0)
       -> c1 = add8(tmp2.low, tmp1.high)
       -> c = (c0, c1)
     */

    constexpr auto Low  = MacroCell::Value0;
    constexpr auto High = MacroCell::Value1;

    auto copyByte = [&](Slot const &fromSlot, MacroCell::Field fromField, Slot const &toSlot, MacroCell::Field toField) {
      moveTo(fromSlot, fromField);
      copyField(Cell{toSlot, toField}, Temps<1>::select(toSlot, MacroCell::Scratch0));
    };

    auto moveByte = [&](Slot const &fromSlot, MacroCell::Field fromField, Slot const &toSlot, MacroCell::Field toField) {
      moveTo(fromSlot, fromField);
      moveField(Cell{toSlot, toField});
    };

    auto zeroByte = [&](Slot const &slot, MacroCell::Field field) {
      moveTo(slot, field);
      zeroCell();
    };

    auto addByteInto = [&](Slot const &lhs, MacroCell::Field lhsField, Slot const &rhs, MacroCell::Field rhsField) {
      moveTo(lhs, lhsField);
      addDestructive(Cell{rhs, rhsField});
    };

    // Copies of operands    
    Slot const lhsCopy = getTemp(lhs.type);
    assignSlot(lhsCopy, lhs);


    bool freeRhsCopy = false;
    Slot const rhsCopy = destroyRhs ? rhs : [&] {
      Slot const tmp = getTemp(rhs.type);
      assignSlot(tmp, rhs);
      freeRhsCopy = true;
      return tmp;    
    }();
    
    // Result will be constructed in lhs; after a0*b0, lhs.low is already the final c0.
    // We zero its high byte and leave only a0.
    zeroByte(lhs, High);

    // rhsLow = only low byte of rhs (b0), high byte zeroed
    Slot const rhsLow = getTemp(ts::u16());
    copyByte(rhsCopy, Low, rhsLow, Low);
    zeroByte(rhsLow, High);

    // lhs <- a0 * b0.
    //
    // After this:
    //   lhs.low  = c0
    //   lhs.high = high(a0*b0)
    moveTo(lhs, Low);
    mul16Destructive(Cell{lhs, High}, Cell{rhsLow, Low}, Cell{rhsLow, High},
		     Temps<9>::select(lhs,    MacroCell::Scratch0,  lhs,    MacroCell::Scratch1,
				      lhs,    MacroCell::Payload0,  lhs,    MacroCell::Payload1,
				      rhsLow, MacroCell::Scratch0,  rhsLow, MacroCell::Scratch1,
				      rhsLow, MacroCell::Payload0,  rhsLow, MacroCell::Payload1,
				      rhsCopy, MacroCell::Scratch0));

    // Reuse rhsLow as scratch storage for one cross term, since it has
    // been consumed by mul16Destructive and is no longer needed.
    Slot const cross0 = rhsLow;

    // Compute cross-terms a0 * b1 and a1 * b0, using the copy in lhsCopy = (a0, a1)
    // lhsCopy.low = a0 * b1
    moveTo(lhsCopy, Low);
    mulDestructive(Cell{rhsCopy, High}, Temps<3>::select(lhsCopy, MacroCell::Scratch0,
							 lhsCopy, MacroCell::Scratch1,
							 rhsCopy, MacroCell::Scratch0));

    // lhsCopy.high = a1 * b0
    moveTo(lhsCopy, High);
    mulDestructive(Cell{rhsCopy, Low}, Temps<3>::select(lhsCopy, MacroCell::Scratch0,
							lhsCopy, MacroCell::Scratch1,
							rhsCopy, MacroCell::Scratch0));

    // Move low(a0*b1) out of lhsCopy.low, because lhsCopy will be considered
    // cross-term storage now.
    moveByte(lhsCopy, Low, cross0, Low);

    // Now:
    //   cross0.low    = low(a0*b1)
    //   lhsCopy.high  = low(a1*b0)
    //
    // Complete high byte:
    //   lhs.high += low(a0*b1) + low(a1*b0)
    addByteInto(lhs, High, cross0, Low);
    addByteInto(lhs, High, lhsCopy, High);

    freeTempSlot(lhsCopy);
    freeTempSlot(rhsLow);
    if (freeRhsCopy) freeTempSlot(rhsCopy);
    
  } else {
    Slot const tmp = getTemp(rhs.type);
    assignSlot(tmp, rhs);  
    moveTo(lhs, MacroCell::Value0);    
    mulDestructive(Cell{tmp, MacroCell::Value0},
		   Temps<3>::select(lhs, MacroCell::Scratch0,
				    lhs, MacroCell::Scratch1,
				    tmp, MacroCell::Scratch0));
    freeTempSlot(tmp);
  }
  popPtr();
}

void Assembler::mulSlotBySlotSigned(Slot const &lhs, Slot const &rhs) {
  assert(types::isSignedInteger(lhs.type));
  assert(types::isSignedInteger(rhs.type));

  pushPtr();
  moveTo(lhs, lhs.type->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitConstructive(Cell{lhs, MacroCell::Flag},
		      Temps<4>::select(lhs, MacroCell::Scratch0,
				       lhs, MacroCell::Scratch1,
				       lhs, MacroCell::Payload0,
				       lhs, MacroCell::Payload1));

  Slot const tmp = getTemp(ts::raw(2));
  Cell const resultNegative { tmp, MacroCell::Flag };
  moveTo(lhs, MacroCell::Flag);
  loopOpen(); {    
    // lhs < 0  ==>  negate LHS and set negative flag
    zeroCell();      
    negateSlot(lhs);
    moveTo(resultNegative);
    setToValue(1);
    moveTo(lhs, MacroCell::Flag);
  } loopClose();


  Slot const rhsCopy = tmp.sub(rhs.type, 1);
  assignSlot(rhsCopy, rhs);
  
  moveTo(rhsCopy, rhsCopy.type->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitConstructive(Cell{rhsCopy, MacroCell::Flag},
		      Temps<4>::select(rhsCopy, MacroCell::Scratch0,
				       rhsCopy, MacroCell::Scratch1,
				       rhsCopy, MacroCell::Payload0,
				       rhsCopy, MacroCell::Payload1));

  
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
  mulSlotBySlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), true);
  
  // Correct the sign
  moveTo(resultNegative);
  loopOpen(); {
    zeroCell();
    negateSlot(lhs);
  } loopClose();

  popPtr();
  freeTempSlot(tmp);
}

// Implementations of mul algorithms

void Assembler::mulConst(int factor, Temps<3> tmp) {
  // TODO: optimize for powers of 2
  // TODO: big factors should have runtime implementation
  
  if (factor == 0) {
    zeroCell();
    return;
  }
  if (factor == 1) return;

  pushPtr();
  Cell const current = _dp.current();
  Cell const copy1 = tmp.get<0>();
  Cell const copy2 = tmp.get<1>();
  
  copyField(copy1, tmp.select<2>());
  copyField(copy2, tmp.select<2>());

  
  for (int i = 0; i != std::abs(factor) - 1; ++i) {
    moveTo(current);    
    addDestructive(copy1);
    moveTo(copy2);
    copyField(copy1, tmp.select<2>());
  }

  // Clear temporary copies
  moveTo(copy1); zeroCell();
  moveTo(copy2); zeroCell();
  
  // All temps have been cleared by this point
  if (factor < 0) {
    moveTo(current);
    negateDestructive(tmp.select<0, 1>());
  }
  
  popPtr();
}

void Assembler::mul16Const(int factor, Cell high, Temps<8> tmp) {
  if (factor == 0) {
    pushPtr();
    zeroCell();
    moveTo(high);
    zeroCell();
    popPtr();
    return;
  }

  if (factor == 1) return;

  pushPtr();
  Cell const current     = _dp.current();
  Cell const copy1low  = tmp.get<0>();
  Cell const copy1high = tmp.get<1>();
  Cell const copy2low  = tmp.get<2>();
  Cell const copy2high = tmp.get<3>();

  moveTo(current);
  copyField(copy1low,  tmp.select<4>());
  copyField(copy2low,  tmp.select<4>());

  moveTo(high);
  copyField(copy1high, tmp.select<4>());
  copyField(copy2high, tmp.select<4>());

  for (int i = 0; i != std::abs(factor) - 1; ++i) {
    moveTo(current);
    add16Destructive(high, copy1low, copy1high, tmp.select<4, 5, 6, 7>());
    moveTo(copy2low);
    copyField(copy1low, tmp.select<4>());
    moveTo(copy2high);
    copyField(copy1high, tmp.select<4>());      
  }

  // Clear temporary copies
  moveTo(copy1low); zeroCell();
  moveTo(copy1high); zeroCell();
  moveTo(copy2low); zeroCell();
  moveTo(copy2high); zeroCell();
  
  // All tmp cells have been cleared by this point and can be reused 
  if (factor < 0) {
    moveTo(current);
    negate16Destructive(high, tmp.select<0, 1, 2, 3, 4, 5>());
  }
  
  popPtr();
}


void Assembler::mulDestructive(Cell factor, Temps<3> tmp) {
  pushPtr();
  
  Cell const current = _dp.current();
  Cell const copy1 = tmp.get<0>();
  Cell const copy2 = tmp.get<1>();
  
  copyField(copy1, tmp.select<2>());
  copyField(copy2, tmp.select<2>());
  zeroCell();
  
  moveTo(factor);
  loopOpen(); {
    dec();
    moveTo(current);
    addDestructive(copy1);
    moveTo(copy2);
    copyField(copy1, tmp.select<2>());
    moveTo(factor);
  } loopClose();

  moveTo(copy1); zeroCell();
  moveTo(copy2); zeroCell();

  popPtr();
}

// TODO: make a 16-bit version that uses the 8-bit version of mul rather than brute force repeat the addition.
void Assembler::mul16Destructive(Cell high, Cell factorLow, Cell factorHigh, Temps<9> tmp) {

  pushPtr();
  
  Cell const current   = _dp.current();
  Cell const copy1low  = tmp.get<0>();
  Cell const copy2low  = tmp.get<1>();
  Cell const copy1high = tmp.get<2>();
  Cell const copy2high = tmp.get<3>();
  Cell const factorNonzero = tmp.get<4>();

  moveTo(current);
  copyField(copy1low, tmp.select<5>());
  copyField(copy2low, tmp.select<5>());
  zeroCell();

  moveTo(high);
  copyField(copy1high, tmp.select<5>());
  copyField(copy2high, tmp.select<5>());
  zeroCell();

  auto computeFactorNonzero = [&]() {
    moveTo(factorNonzero);
    zeroCell(); // TODO: this is superfluous right?
    moveTo(factorLow);
    orConstructive(factorNonzero, factorHigh, tmp.select<5, 6>());
    moveTo(factorNonzero);
  };
  
  computeFactorNonzero();  
  loopOpen(); {
    moveTo(factorLow);
    dec16(factorHigh, tmp.select<5, 6>());

    moveTo(current);
    add16Destructive(high, copy1low, copy1high, tmp.select<5, 6, 7, 8>());

    moveTo(copy2low);
    copyField(copy1low, tmp.select<5>());
    moveTo(copy2high);
    copyField(copy1high, tmp.select<5>());

    computeFactorNonzero();
  } loopClose();


  moveTo(copy1low);  zeroCell();
  moveTo(copy1high); zeroCell();
  moveTo(copy2low);  zeroCell();
  moveTo(copy2high); zeroCell();
  
  popPtr();
}

void Assembler::mulConstructive(Cell result, Cell factor, Temps<4> tmp) {
  Cell const factorCopy = tmp.get<0>();

  pushPtr();
  copyField(result, tmp.get<1>());
  moveTo(factor);
  copyField(factorCopy, tmp.get<1>());
  moveTo(result);
  mulDestructive(factorCopy, tmp.select<1, 2, 3>());
  popPtr();
}


void Assembler::mul16Constructive(Cell high, Cell resultLow, Cell resultHigh, Cell factorLow, Cell factorHigh, Temps<11> tmp) {

  Cell const & low      = _dp.current();
  Cell const & factorLowCopy  = tmp.get<0>();
  Cell const & factorHighCopy = tmp.get<1>();
  
  pushPtr();
  moveTo(low);       copyField(resultLow, tmp.select<2>());
  moveTo(high);      copyField(resultHigh, tmp.select<2>());
  moveTo(factorLow);  copyField(factorLowCopy, tmp.select<2>());
  moveTo(factorHigh); copyField(factorHighCopy, tmp.select<2>());

  moveTo(resultLow);
  mul16Destructive(resultHigh, factorLowCopy, factorHighCopy, tmp.select<2, 3, 4, 5, 6, 7, 8, 9, 10>());
  popPtr();
}
