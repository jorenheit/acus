// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

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

void Assembler::divSlotByConstUnsigned(Slot lhs, int denom, std::optional<Slot> const &modSlot) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(denom >= 0);
  
  pushPtr();
  if (lhs.type()->usesValue1()) {
    // TODO: less temps: negateFlag can share slot with this one
    Slot tmp = getTemp(ts::raw(2));
    moveTo(lhs, MacroCell::Value0);    
    divMod16Const(denom, Cell{lhs, MacroCell::Value1},
		  Cell{lhs, MacroCell::Payload0},
		  Cell{lhs, MacroCell::Payload1},
		  Temps<8>::select(lhs, MacroCell::Scratch0,
				   lhs, MacroCell::Scratch1,
				   tmp, MacroCell::Scratch0,
				   tmp, MacroCell::Scratch1,
				   tmp + 1, MacroCell::Scratch0,
				   tmp + 1, MacroCell::Scratch1,
				   tmp + 1, MacroCell::Payload0,
				   tmp + 1, MacroCell::Payload1));

    moveTo(lhs, MacroCell::Payload0);    
    if (modSlot.has_value()) moveField(Cell{*modSlot, MacroCell::Value0});
    else zeroCell();

    moveTo(lhs, MacroCell::Payload1);
    if (modSlot.has_value()) moveField(Cell{*modSlot, MacroCell::Value1});
    else zeroCell();

    freeTempSlot(tmp);
  } else {
    Slot tmp = getTemp(ts::raw(1));
    moveTo(lhs, MacroCell::Value0);    
    divModConst(denom, Cell{lhs, MacroCell::Payload0},
		Temps<5>::select(lhs, MacroCell::Scratch0,
				 lhs, MacroCell::Scratch1,
				 lhs, MacroCell::Payload1,
				 tmp, MacroCell::Scratch0,
				 tmp, MacroCell::Scratch1));

    moveTo(lhs, MacroCell::Payload0);    
    if (modSlot.has_value()) moveField(Cell{*modSlot, MacroCell::Value0});
    else zeroCell();
    freeTempSlot(tmp);
  }
  
  popPtr();
}

void Assembler::divSlotByConstSigned(Slot lhs, int denom, std::optional<Slot> const &modSlot) {
  assert(types::isSignedInteger(lhs.type()));
  
  // For signed integers, check if the value is negative. If so, take the
  // absolute value but remember the sign.
  pushPtr();
  moveTo(lhs, lhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitConstructive(Cell{lhs, MacroCell::Flag},
		      Temps<3>::select(lhs, MacroCell::Scratch0,
				       lhs, MacroCell::Scratch1,
				       lhs, MacroCell::Payload0));

  Slot tmp = getTemp(ts::raw(1));
  Cell const lhsNegative { tmp, MacroCell::Flag };
  moveTo(lhs, MacroCell::Flag);
  loopOpen(); {
    moveTo(lhsNegative);
    setToValue(1);
    negateSlot(lhs);
    moveTo(lhs, MacroCell::Flag);
    zeroCell();      
  } loopClose();

  
  divSlotByConstUnsigned(lhs.unsignedView(), std::abs(denom), modSlot);

  moveTo(lhsNegative);
  if (denom < 0) {
    notDestructive(Cell{tmp, MacroCell::Scratch0});
  }
  loopOpen(); {
    negateSlot(lhs);
    moveTo(lhsNegative); zeroCell();
  } loopClose();

  popPtr();
  freeTempSlot(tmp);
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

void Assembler::divSlotBySlotUnsigned(Slot lhs, Slot rhs, std::optional<Slot> const &modSlot, bool const destroyRhs) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));

  bool freeRhsCopy = false;
  Slot rhsCopy = [&] {
    if (destroyRhs) return rhs;
    Slot const tmp = getTemp(rhs.type());
    assignSlot(tmp, rhs);
    freeRhsCopy = true;
    return tmp;    
  }();


  pushPtr();
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {

    bool freeTmpDonor = false;
    Slot tmpDonor = [&] {
      if (destroyRhs) {
	freeTmpDonor = true;
	return getTemp(ts::raw(1));
      }
      return rhs;
    }();
    
    moveTo(lhs, MacroCell::Value0);    
    divMod16Destructive(Cell{lhs, MacroCell::Value1},
			Cell{rhsCopy, MacroCell::Value0},
			Cell{rhsCopy, MacroCell::Value1},
			Cell{lhs, MacroCell::Payload0},
			Cell{lhs, MacroCell::Payload1},
			Temps<8>::select(lhs, MacroCell::Scratch0,
					 lhs, MacroCell::Scratch1,
					 tmpDonor, MacroCell::Scratch0,
					 tmpDonor, MacroCell::Scratch1,
					 rhsCopy, MacroCell::Scratch0,
					 rhsCopy, MacroCell::Scratch1,
					 rhsCopy, MacroCell::Payload0,
					 rhsCopy, MacroCell::Payload1));

    moveTo(lhs, MacroCell::Payload0);    
    if (modSlot.has_value()) moveField(Cell{*modSlot, MacroCell::Value0});
    else zeroCell();

    moveTo(lhs, MacroCell::Payload1);
    if (modSlot.has_value()) moveField(Cell{*modSlot, MacroCell::Value1});
    else zeroCell();

    if (freeTmpDonor) freeTempSlot(tmpDonor);

  } else {
    moveTo(lhs, MacroCell::Value0);    
    divModDestructive(Cell{rhsCopy, MacroCell::Value0},
		      Cell{lhs, MacroCell::Payload0},
		      Temps<5>::select(lhs, MacroCell::Scratch0,
				       lhs, MacroCell::Scratch1,
				       lhs, MacroCell::Payload1,
				       rhsCopy, MacroCell::Scratch0,
				       rhsCopy, MacroCell::Scratch1));

    moveTo(lhs, MacroCell::Payload0);    
    if (modSlot.has_value()) moveField(Cell{*modSlot, MacroCell::Value0});
    else zeroCell();
  }
  
  popPtr();

  if (freeRhsCopy) freeTempSlot(rhsCopy);
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
    setToValue(1);
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

void Assembler::modSlotByConstUnsigned(Slot lhs, int denom, std::optional<Slot> const &divSlot) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(denom >= 0);
  
  pushPtr();
  moveTo(lhs, MacroCell::Value0);    
  if (lhs.type()->usesValue1()) {
    Slot tmp = getTemp(ts::raw(2));
    divMod16Const(denom, Cell{lhs, MacroCell::Value1},
		  Cell{lhs, MacroCell::Payload0},
		  Cell{lhs, MacroCell::Payload1},
		  Temps<8>::select(lhs, MacroCell::Scratch0,
				   lhs, MacroCell::Scratch1,
				   tmp, MacroCell::Scratch0,
				   tmp, MacroCell::Scratch1,
				   tmp + 1, MacroCell::Scratch0,
				   tmp + 1, MacroCell::Scratch1,
				   tmp + 1, MacroCell::Payload0,
				   tmp + 1, MacroCell::Payload1));


    // If divslot provided, move division result into there before moving the modresult back into the value-cells
    moveTo(lhs, MacroCell::Value0); 
    if (divSlot.has_value())  moveField(Cell{*divSlot, MacroCell::Value0});
    moveTo(lhs, MacroCell::Value1); 
    if (divSlot.has_value())  moveField(Cell{*divSlot, MacroCell::Value1});

    moveTo(lhs, MacroCell::Payload0);
    moveField(Cell{lhs, MacroCell::Value0});
    moveTo(lhs, MacroCell::Payload1);
    moveField(Cell{lhs, MacroCell::Value1});
    freeTempSlot(tmp);
  } else {
    Slot tmp = getTemp(ts::raw(1));
    divModConst(denom, Cell{lhs, MacroCell::Payload0},
		Temps<5>::select(lhs, MacroCell::Scratch0,
				 lhs, MacroCell::Scratch1,
				 tmp, MacroCell::Scratch0,
				 tmp, MacroCell::Scratch1,
				 tmp, MacroCell::Payload0));

    // If divslot provided, move division result into there before moving the modresult back into the value-cells
    moveTo(lhs, MacroCell::Value0); 
    if (divSlot.has_value())  moveField(Cell{*divSlot, MacroCell::Value0});
    moveTo(lhs, MacroCell::Payload0);
    moveField(Cell{lhs, MacroCell::Value0});
    freeTempSlot(tmp);    
  }

  popPtr();
}


void Assembler::modSlotByConstSigned(Slot lhs, int denom, std::optional<Slot> const &divSlot) {
  assert(types::isSignedInteger(lhs.type()));

  pushPtr();
  // For signed integers, the sign of the result is equal to the sign of the LHS
  moveTo(lhs, lhs.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  signBitConstructive(Cell{lhs, MacroCell::Flag},
		      Temps<3>::select(lhs, MacroCell::Scratch0,
				       lhs, MacroCell::Scratch1,
				       lhs, MacroCell::Payload0));

  Slot tmp = getTemp(ts::u8());
  Cell const resultNegative { tmp, MacroCell::Flag };
  moveTo(lhs, MacroCell::Flag);
  loopOpen(); {
    moveTo(resultNegative);
    setToValue(1);
    negateSlot(lhs);
    moveTo(lhs, MacroCell::Flag);
    zeroCell();      
  } loopClose();
  
  // lhs is now positive and the resulting sign has been stored -> use unsigned version
  modSlotByConstUnsigned(lhs.unsignedView(), std::abs(denom), divSlot);
  
  // Restore sign
  moveTo(resultNegative);
  loopOpen(); {
    zeroCell();
    negateSlot(lhs);
  } loopClose();
  
  popPtr();
  freeTempSlot(tmp);
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

void Assembler::modSlotBySlotUnsigned(Slot lhs, Slot rhs, std::optional<Slot> const &divSlot, bool const destroyRhs) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));
  
  bool freeRhsCopy = false;
  Slot rhsCopy = destroyRhs ? rhs : [&] {
    Slot const tmp = getTemp(rhs.type());
    assignSlot(tmp, rhs);
    freeRhsCopy = true;
    return tmp;    
  }();

  Slot tmp = destroyRhs ? getTemp(ts::raw(1)) : rhs;
  
  pushPtr();
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {

    bool freeTmpDonor = false;
    Slot tmpDonor = [&] {
      if (destroyRhs) {
	freeTmpDonor = true;
	return getTemp(ts::raw(1));
      }
      return rhs;
    }();

    
    moveTo(lhs, MacroCell::Value0);    
    divMod16Destructive(Cell{lhs, MacroCell::Value1},
			Cell{rhsCopy, MacroCell::Value0},
			Cell{rhsCopy, MacroCell::Value1},
			Cell{lhs, MacroCell::Payload0},
			Cell{lhs, MacroCell::Payload1},
			Temps<8>::select(lhs, MacroCell::Scratch0,
					 lhs, MacroCell::Scratch1,
					 tmpDonor, MacroCell::Scratch0,
					 tmpDonor, MacroCell::Scratch1,
					 rhsCopy, MacroCell::Scratch0,
					 rhsCopy, MacroCell::Scratch1,
					 rhsCopy, MacroCell::Payload0,
					 rhsCopy, MacroCell::Payload1));

    // If divslot provided, move division result into there before moving the modresult back into the value-cells
    moveTo(lhs, MacroCell::Value0); 
    if (divSlot.has_value())  moveField(Cell{*divSlot, MacroCell::Value0});
    moveTo(lhs, MacroCell::Value1); 
    if (divSlot.has_value())  moveField(Cell{*divSlot, MacroCell::Value1});

    moveTo(lhs, MacroCell::Payload0);
    moveField(Cell{lhs, MacroCell::Value0});
    moveTo(lhs, MacroCell::Payload1);
    moveField(Cell{lhs, MacroCell::Value1});
    if (freeTmpDonor) freeTempSlot(tmpDonor);
        
  } else {
    moveTo(lhs, MacroCell::Value0);    
    divModDestructive(Cell{rhsCopy, MacroCell::Value0},
		      Cell{lhs, MacroCell::Payload0},
		      Temps<5>::select(lhs, MacroCell::Scratch0,
				       lhs, MacroCell::Scratch1,
				       lhs, MacroCell::Payload1,
				       rhsCopy, MacroCell::Scratch0,
				       rhsCopy, MacroCell::Scratch1));

    // If divslot provided, move division result into there before moving the modresult back into the value-cells
    moveTo(lhs, MacroCell::Value0); 
    if (divSlot.has_value())  moveField(Cell{*divSlot, MacroCell::Value0});
    moveTo(lhs, MacroCell::Payload0);
    moveField(Cell{lhs, MacroCell::Value0});
  }

  popPtr();
  if (freeRhsCopy) freeTempSlot(rhsCopy);
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
    setToValue(1);
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


// Implementations of the divmod algorithms

void Assembler::divModConst(int denom, Cell modResult, Temps<5> tmp) {
  pushPtr();
  Cell const divResult = _dp.current();

  moveTo(modResult); setToValue(0);
  if (denom == 1) {
    popPtr();
    return;
  }
  if (denom == 0) {
    moveTo(divResult); zeroCell(); dec();
    popPtr();
    return;
  }

  Cell const numeratorIsZero = tmp.get<0>();
  Cell const loopFlag = tmp.get<1>();
  Cell const numCopy = tmp.get<2>();
  Cell const denomCopy = tmp.get<3>();

  moveTo(divResult);  moveField(numCopy);
  moveTo(denomCopy);  setToValue(denom & 0xff, tmp.select<4>());
  moveTo(loopFlag);   setToValue(1);

  moveTo(numCopy);
  notConstructive(numeratorIsZero, tmp.select<4>());
  moveTo(numeratorIsZero);
  loopOpen(); {
    zeroCell();
    // 0 / x -> divResult and modResult remain 0 -> return
    moveTo(loopFlag);
    setToValue(0);
    moveTo(numeratorIsZero);
  } loopClose();

  moveTo(loopFlag);
  loopOpen(); {    

    moveTo(modResult); inc();
    moveTo(numCopy);   dec();
    moveTo(denomCopy); dec();

    // Repurpose flag-cell
    Cell const denominatorIsZero = numeratorIsZero;
    moveTo(denomCopy);
    notConstructive(denominatorIsZero, tmp.select<4>());
    moveTo(denominatorIsZero);
    loopOpen(); {
      zeroCell();
      
      // denomCopy was decremented all the way to 0 -> increase div-result, restore numCopy,  and reset mod-result
      moveTo(divResult); inc();
      moveTo(modResult); zeroCell();
      moveTo(denomCopy);
      setToValue(denom & 0xff, tmp.select<4>());
      
      moveTo(denominatorIsZero);
    } loopClose();

    // Repurpose flag-call
    Cell const numeratorIsZero = denominatorIsZero;
    moveTo(numCopy);
    notConstructive(numeratorIsZero, tmp.select<4>());
    moveTo(numeratorIsZero);
    loopOpen(); {
      zeroCell();
      moveTo(loopFlag); setToValue(0);
      moveTo(numeratorIsZero);
    } loopClose();

    moveTo(loopFlag);
  } loopClose();

  // Cleanup
  moveTo(denomCopy); zeroCell();

  popPtr();
}

void Assembler::divMod16Const(int denom, Cell high, Cell modResultLow, Cell modResultHigh, Temps<8> tmp) {
  pushPtr();

  Cell const divResultLow = _dp.current();
  Cell const divResultHigh = high;
  
  moveTo(modResultLow);  setToValue(0);
  moveTo(modResultHigh); setToValue(0);

  if (denom == 1) {
    popPtr();
    return;
  }
  
  if (denom == 0) {
    moveTo(divResultLow);  zeroCell(); dec();
    moveTo(divResultHigh); zeroCell(); dec();
    popPtr();
    return;
  }

  Cell const numeratorIsZero = tmp.get<0>();
  Cell const loopFlag = tmp.get<1>();
  Cell const numCopyLow = tmp.get<2>();
  Cell const numCopyHigh = tmp.get<3>();  
  Cell const denomCopyLow = tmp.get<4>();
  Cell const denomCopyHigh = tmp.get<5>();

  moveTo(divResultLow);  moveField(numCopyLow);
  moveTo(divResultHigh); moveField(numCopyHigh);
  moveTo(denomCopyLow);  setToValue(denom & 0xff, tmp.select<6>());
  moveTo(denomCopyHigh); setToValue((denom >> 8) & 0xff, tmp.select<6>());
  moveTo(loopFlag);      setToValue(1);

  moveTo(numCopyLow);
  not16Constructive(numCopyHigh, numeratorIsZero, tmp.select<6, 7>());
  moveTo(numeratorIsZero);
  loopOpen(); { // if numerator is 0, return immediately (divResult and modResult remain 0)
    zeroCell();
    moveTo(loopFlag);
    setToValue(0);
    moveTo(numeratorIsZero);
  } loopClose();

  moveTo(loopFlag);
  loopOpen(); { // Division algorithm starts here

    // increase mod while decrementing numerator and denominator
    moveTo(modResultLow); inc16(modResultHigh, tmp.select<6, 7>());
    moveTo(numCopyLow);   dec16(numCopyHigh, tmp.select<6, 7>());
    moveTo(denomCopyLow); dec16(denomCopyHigh, tmp.select<6, 7>());

    // Check if the denominator (copy) has been decremented to 0
    Cell const denominatorIsZero = numeratorIsZero;
    moveTo(denomCopyLow);    
    not16Constructive(denomCopyHigh, denominatorIsZero, tmp.select<6, 7>());
    moveTo(denominatorIsZero);
    loopOpen(); {
      zeroCell();
      
      // denomCopy was decremented all the way to 0 -> increase div-result, restore numCopy, and reset mod-result
      moveTo(divResultLow);  inc16(divResultHigh, tmp.select<6, 7>());
      moveTo(modResultLow);  zeroCell();
      moveTo(modResultHigh); zeroCell();
      moveTo(denomCopyLow);  setToValue(denom & 0xff, tmp.select<6>());
      moveTo(denomCopyHigh); setToValue((denom >> 8) & 0xff, tmp.select<6>());
      
      moveTo(denominatorIsZero);
    } loopClose();

    // Check if the numerator (copy) has been decremented to 0 -> we're done
    Cell const numeratorIsZero = denominatorIsZero;
    moveTo(numCopyLow);
    not16Constructive(numCopyHigh, numeratorIsZero, tmp.select<6, 7>());
    moveTo(numeratorIsZero);
    loopOpen(); {
      zeroCell();
      moveTo(loopFlag); setToValue(0);
      moveTo(numeratorIsZero);
    } loopClose();

    moveTo(loopFlag);
  } loopClose();

  // Cleanup
  moveTo(denomCopyLow);  zeroCell();
  moveTo(denomCopyHigh); zeroCell();

  popPtr();
}


void Assembler::divModDestructive(Cell denom, Cell modResult, Temps<5> tmp) {
  pushPtr();

  Cell const divResult = _dp.current();
  Cell const zeroFlag = tmp.get<0>();
  Cell const loopFlag = tmp.get<1>();
  Cell const numCopy = tmp.get<2>();
  Cell const denomCopy = tmp.get<3>();

  moveTo(modResult); setToValue(0);
  moveTo(divResult); moveField(numCopy);
  moveTo(denom);     copyField(denomCopy, tmp.select<4>());
  moveTo(loopFlag);  setToValue(1);

  // Division by 0
  moveTo(denomCopy);
  notConstructive(zeroFlag, tmp.select<4>());
  moveTo(zeroFlag);
  loopOpen(); {
    moveTo(divResult); dec();
    moveTo(loopFlag);  setToValue(0);
    moveTo(zeroFlag);  zeroCell();
  } loopClose();

  // Zero in the numerator
  moveTo(numCopy);
  notConstructive(zeroFlag, tmp.select<4>());
  moveTo(zeroFlag);
  loopOpen(); {
    moveTo(divResult); zeroCell();
    moveTo(loopFlag);  setToValue(0);
    moveTo(zeroFlag);  zeroCell();
  } loopClose();

  // General case
  moveTo(loopFlag);
  loopOpen(); {    

    moveTo(modResult); inc();
    moveTo(numCopy);   dec();
    moveTo(denomCopy); dec();

    moveTo(denomCopy);
    notConstructive(zeroFlag, tmp.select<4>());
    moveTo(zeroFlag);
    loopOpen(); {
      zeroCell();
      // denomCopy was decremented all the way to 0 -> increase div-result, restore numCopy,  and reset mod-result
      moveTo(divResult); inc();
      moveTo(modResult); zeroCell();
      moveTo(denom);     copyField(denomCopy, tmp.select<4>());

      moveTo(zeroFlag);
    } loopClose();

    moveTo(numCopy);
    notConstructive(zeroFlag, tmp.select<4>());
    moveTo(zeroFlag);
    loopOpen(); {
      zeroCell();
      moveTo(loopFlag); setToValue(0);
      moveTo(zeroFlag);
    } loopClose();
    
    moveTo(loopFlag);
  } loopClose();

  // Cleanup
  moveTo(denom);     zeroCell();
  moveTo(denomCopy); zeroCell();

  popPtr();
}

void Assembler::divMod16Destructive(Cell high, Cell denomLow, Cell denomHigh, Cell modResultLow, Cell modResultHigh, Temps<8> tmp) {
  pushPtr();

  Cell const divResultLow  = _dp.current();
  Cell const divResultHigh = high;
  Cell const zeroFlag      = tmp.get<0>();
  Cell const loopFlag      = tmp.get<1>();
  Cell const numCopyLow    = tmp.get<2>();
  Cell const numCopyHigh   = tmp.get<3>();
  Cell const denomCopyLow  = tmp.get<4>();
  Cell const denomCopyHigh = tmp.get<5>();

  moveTo(modResultLow);  setToValue(0);
  moveTo(modResultHigh); setToValue(0);
  moveTo(divResultLow);  moveField(numCopyLow);
  moveTo(divResultHigh); moveField(numCopyHigh);
  moveTo(denomLow);      copyField(denomCopyLow, tmp.select<6>());
  moveTo(denomHigh);     copyField(denomCopyHigh, tmp.select<6>());
  moveTo(loopFlag);      setToValue(1);

  // Division by 0
  moveTo(denomCopyLow);
  not16Constructive(denomCopyHigh, zeroFlag, tmp.select<6, 7>());
  moveTo(zeroFlag);
  loopOpen(); {
    moveTo(divResultLow); dec16(divResultHigh, tmp.select<6, 7>());
    moveTo(loopFlag);     setToValue(0);
    moveTo(zeroFlag);     zeroCell();
  } loopClose();

  // Zero in the numerator
  moveTo(numCopyLow);
  not16Constructive(numCopyHigh, zeroFlag, tmp.select<6, 7>());
  moveTo(zeroFlag);
  loopOpen(); {
    moveTo(divResultLow);  zeroCellPlus();
    moveTo(divResultHigh); zeroCell();
    moveTo(loopFlag);      setToValue(0);
    moveTo(zeroFlag);      zeroCell();
  } loopClose();

  // General case
  moveTo(loopFlag);
  loopOpen(); {    
    // !!!!    
    moveTo(modResultLow); inc16(modResultHigh, tmp.select<6, 7>());
    moveTo(numCopyLow);   dec16(numCopyHigh, tmp.select<6, 7>());
    moveTo(denomCopyLow); dec16(denomCopyHigh, tmp.select<6, 7>());

    moveTo(denomCopyLow);
    not16Constructive(denomCopyHigh, zeroFlag, tmp.select<6, 7>());
    moveTo(zeroFlag);
    loopOpen(); {
      // denomCopy was decremented all the way to 0 -> increase div-result, restore numCopy,  and reset mod-result
      moveTo(divResultLow);  inc16(divResultHigh, tmp.select<6, 7>());
      moveTo(modResultLow);  zeroCell();
      moveTo(modResultHigh); zeroCell();
      moveTo(denomLow);      copyField(denomCopyLow, tmp.select<6>());
      moveTo(denomHigh);     copyField(denomCopyHigh, tmp.select<6>());
      moveTo(zeroFlag);      zeroCell();
    } loopClose();

    moveTo(numCopyLow);
    not16Constructive(numCopyHigh, zeroFlag, tmp.select<6, 7>());
    moveTo(zeroFlag);
    loopOpen(); {
      moveTo(loopFlag); setToValue(0);
      moveTo(zeroFlag); zeroCell();
    } loopClose();
    
    moveTo(loopFlag);
  } loopClose();

  // Cleanup
  moveTo(denomLow);      zeroCell();
  moveTo(denomHigh);     zeroCell();
  moveTo(denomCopyLow);  zeroCell();
  moveTo(denomCopyHigh); zeroCell();

  popPtr();
}


void Assembler::divModConstructive(Cell result, Cell denom, Cell modResult, Temps<6> tmp) {
  Cell const denomCopy = tmp.get<0>();

  pushPtr();
  copyField(result, tmp.get<1>());
  moveTo(denom);
  copyField(denomCopy, tmp.get<1>());
  moveTo(result);
  divModDestructive(denomCopy, modResult, tmp.select<1, 2, 3, 4, 5>());
  popPtr();

}


void Assembler::divMod16Constructive(Cell high,
				    Cell resultLow, Cell resultHigh,
				    Cell denomLow, Cell denomHigh,
				    Cell modResultLow, Cell modResultHigh,
				    Temps<12> tmp) {

  Cell const low            = _dp.current();
  Cell const denomLowCopy   = tmp.get<1>();
  Cell const denomHighCopy  = tmp.get<2>();
  
  pushPtr();
  moveTo(low);       copyField(resultLow, tmp.select<3>());
  moveTo(high);      copyField(resultHigh, tmp.select<3>());
  moveTo(denomLow);  copyField(denomLowCopy, tmp.select<3>());
  moveTo(denomHigh); copyField(denomHighCopy, tmp.select<3>());

  moveTo(resultLow);
  divMod16Destructive(resultHigh,
		      denomLowCopy, denomHighCopy,
		      modResultLow, modResultHigh,
		      tmp.select<4, 5, 6, 7, 8, 9, 10, 11>());
  popPtr();
}

