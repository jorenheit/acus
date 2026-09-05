// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::setSlotToBool(Slot slot, bool value) {
  moveTo(slot, MacroCell::Value0); setToValue(value);
  moveTo(slot, MacroCell::Value1); setToValue(0);
}

void Assembler::slotEqualConst(Slot lhs, int val) {

  pushPtr();

  moveTo(lhs);  
  subConstFromSlot(lhs, val);
  
  if (lhs.type()->usesValue1()) {
    not16Destructive(Cell{lhs, MacroCell::Value1},
		     Temps<1>::select(lhs, MacroCell::Scratch0));
  } else {
    notDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
  }

  popPtr();
}

void Assembler::slotEqualSlot(Slot lhs, Slot rhs) {
  pushPtr();
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  moveTo(lhs);

  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    eq16Destructive(Cell{lhs, MacroCell::Value1},
		    Cell{rhsCopy, MacroCell::Value0},
		    Cell{rhsCopy, MacroCell::Value1},
		    Temps<1>::select(lhs, MacroCell::Scratch0));
  } else {
    eqDestructive(Cell{rhsCopy, MacroCell::Value0},
		  Temps<1>::select(lhs, MacroCell::Scratch0));
  }

  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::slotNotEqualConst(Slot lhs, int val) {
  pushPtr();
  slotEqualConst(lhs, val);
  moveTo(lhs);
  notDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
  popPtr();
}

void Assembler::slotNotEqualSlot(Slot lhs, Slot rhs) {
  pushPtr();
  slotEqualSlot(lhs, rhs);
  moveTo(lhs);
  notDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
  popPtr();
}

void Assembler::slotLessConst(Slot lhs, int val) {
  assert(types::isInteger(lhs.type()));
  if (types::isUnsignedInteger(lhs.type())) return slotLessConstUnsigned(lhs, val);
  if (types::isSignedInteger(lhs.type()))   return slotLessConstSigned(lhs, val);
  std::unreachable();
}

void Assembler::slotLessConstUnsigned(Slot lhs, int val) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(val >= 0);
  
  if (val == 0) {
    pushPtr();
    moveTo(lhs);
    setToValue16(0, Cell{lhs, MacroCell::Value1});
    popPtr();
    return;
  }

  pushPtr();

  Slot valSlot = getTemp(((val >> 8) & 0xff) ? literal::u16(val) : literal::u8(val));
  slotLessSlotUnsigned(lhs, valSlot, true);
  freeTempSlot(valSlot);

  popPtr();
}

void Assembler::slotLessConstSigned(Slot lhs, int val) {
  assert(types::isSignedInteger(lhs.type()));

  if (val == 0) {
    signBitSlot(lhs);
  }
  else if (val > 0) {
    // If sign bit is set, return 1
    // If no sign bit, do normal unsigned comparison
    branchOnSignBit(lhs, Cell{lhs, MacroCell::Flag},
		    [&] /* lhs  < 0 */ { setSlotToBool(lhs, true); },
		    [&] /* lhs >= 0 */ { slotLessConstUnsigned(lhs.unsignedView(), val); });
		    
  }
  else if (val < 0) {
    // If no sign bit, return 0
    // If sign bit is set, take absolute value and do unsigned comparison between absolute values,
    // but use greater-than algorithm.
    branchOnSignBit(lhs, Cell{lhs, MacroCell::Flag},
		    [&] /* lhs < 0 */ {
		      negateSlot(lhs);
		      slotGreaterConstUnsigned(lhs.unsignedView(), std::abs(val));
		    },
		    [&] /* lhs >= 0 */ {
		      setSlotToBool(lhs, false);
		    });
  }
}

void Assembler::slotLessSlot(Slot lhs, Slot rhs) {
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return slotLessSlotUnsigned(lhs, rhs);
  if (types::isSignedInteger(lhs.type()))   return slotLessSlotSigned(lhs, rhs);
  std::unreachable();
}

void Assembler::slotLessSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));
  
  pushPtr();

  bool freeRhsCopy = false;
  Slot rhsCopy = [&] {
    if (destroyRhs) return rhs;
    Slot const tmp = getTemp(rhs.type());
    assignSlot(tmp, rhs);
    freeRhsCopy = true;
    return tmp;
  }();

  moveTo(lhs);  
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    less16Destructive(Cell{lhs, MacroCell::Value1},
		      Cell{rhsCopy, MacroCell::Value0},
		      Cell{rhsCopy, MacroCell::Value1},
		      Temps<4>::select(lhs, MacroCell::Scratch0,
				       lhs, MacroCell::Scratch1,
				       rhsCopy, MacroCell::Scratch0,
				       rhsCopy, MacroCell::Scratch1));
  } else {
    lessDestructive(Cell{rhsCopy, MacroCell::Value0},
		    Temps<1>::select(lhs, MacroCell::Scratch0));
  }

  popPtr();
  if (freeRhsCopy) freeTempSlot(rhsCopy);
}

void Assembler::slotLessSlotSigned(Slot lhs, Slot rhs) {
  assert(types::isSignedInteger(lhs.type()));
  assert(types::isSignedInteger(rhs.type()));

  // Both positive -> use unsigned algorithm
  // lhs negative, rhs positive -> return 1
  // lhs positive, rhs negative -> return 0
  // both negative -> use unsigned greater on absolute values
  
  branchOnSignBit(lhs, Cell{lhs, MacroCell::Flag},
		  [&] /* lhs < 0 */ { 
		    branchOnSignBit(rhs, Cell{rhs, MacroCell::Flag},
				    [&] /* rhs < 0 */ {
				      // Both negative -> negate both and use unsigned greater-than
				      negateSlot(lhs);
				      Slot rhsCopy = getTemp(rhs.type());
				      assignSlot(rhsCopy, rhs);
				      negateSlot(rhsCopy);
				      slotGreaterSlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), true);
				      freeTempSlot(rhsCopy);
				    },
				    [&] /* rhs >= 0 */ {
				      // lhs negative but rhs is not, so lhs is always less
				      setSlotToBool(lhs, true);
				    });
		  },
		  [&] /* lhs >= 0 */ {
		    branchOnSignBit(rhs, Cell{rhs, MacroCell::Flag},
				    [&] /* rhs < 0 */ {
				      // lhs is positive while rhs is negative, so lhs is never less
				      setSlotToBool(lhs, false);
				    },
				    [&] /* rhs >= 0 */ {
				      // Both are positive, so we can use the unsigned version
				      slotLessSlotUnsigned(lhs.unsignedView(), rhs.unsignedView());
				    });
		  }); 
}
  
  

void Assembler::slotLessEqualConst(Slot lhs, int val) {
  assert(types::isInteger(lhs.type()));
  
  if (types::isUnsignedInteger(lhs.type())) return slotLessEqualConstUnsigned(lhs, val);
  if (types::isSignedInteger(lhs.type()))   return slotLessEqualConstSigned(lhs, val);
  std::unreachable();
}


void Assembler::slotLessEqualConstUnsigned(Slot lhs, int val) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(val >= 0);
  
  pushPtr();

  // If val is maximal, the result must be true
  if (lhs.type()->usesValue1() && ((val & 0xffff) == 0xffff)) {
    moveTo(lhs); 
    setToValue16(1, Cell{lhs, MacroCell::Value1});
    popPtr();
    return;
  }
  else if ((val & 0xff) == 0xff) {
    moveTo(lhs);
    setToValue(1);
    popPtr();
    return;
  }
  
  Slot valSlot = getTemp(((val >> 8) & 0xff) ? literal::u16(val) : literal::u8(val));
  slotLessEqualSlotUnsigned(lhs, valSlot, true);
  freeTempSlot(valSlot);

  popPtr();
}

void Assembler::slotLessEqualConstSigned(Slot lhs, int val) {
  assert(types::isSignedInteger(lhs.type()));

  if (val >= 0) {
    // if sign bit is set -> return 1
    // if not, use unsigned version
    
    branchOnSignBit(lhs, Cell{lhs, MacroCell::Flag},
		    [&] /* lhs  < 0 */ { setSlotToBool(lhs, true); },
		    [&] /* lhs >= 0 */ { slotLessEqualConstUnsigned(lhs.unsignedView(), val); });
  }

  if (val < 0) {
    // if sign bit is set -> return abs(lhs) >= abs(val)
    // if not, return 0
    branchOnSignBit(lhs, Cell{lhs, MacroCell::Flag},
		    [&] /* lhs < 0 */ {
		      negateSlot(lhs);
		      slotGreaterEqualConstUnsigned(lhs.unsignedView(), std::abs(val));
		    },
		    [&] /* lhs >= 0 */ {
		      setSlotToBool(lhs, false);
		    });
  }
}

void Assembler::slotLessEqualSlot(Slot lhs, Slot rhs) {
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return slotLessEqualSlotUnsigned(lhs, rhs);
  if (types::isSignedInteger(lhs.type()))   return slotLessEqualSlotSigned(lhs, rhs);
  std::unreachable();
}

void Assembler::slotLessEqualSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));

  pushPtr();

  bool freeRhsCopy = false;
  Slot rhsCopy = [&] {
    if (destroyRhs) return rhs;
    Slot const tmp = getTemp(rhs.type());
    assignSlot(tmp, rhs);
    freeRhsCopy = true;
    return tmp;
  }();

  moveTo(lhs);  
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    lessOrEqual16Destructive(Cell{lhs, MacroCell::Value1},
			     Cell{rhsCopy, MacroCell::Value0},
			     Cell{rhsCopy, MacroCell::Value1},
			     Temps<3>::select(lhs, MacroCell::Scratch0,
					      lhs, MacroCell::Scratch1,
					      rhsCopy, MacroCell::Scratch0));
  } else {
    lessOrEqualDestructive(Cell{rhsCopy, MacroCell::Value0},
			   Temps<1>::select(lhs, MacroCell::Scratch0));
  }

  popPtr();
  if (freeRhsCopy) freeTempSlot(rhsCopy);
}

void Assembler::slotLessEqualSlotSigned(Slot lhs, Slot rhs) {
  assert(types::isSignedInteger(lhs.type()));
  assert(types::isSignedInteger(rhs.type()));

  // if both are positive, use unsigned version
  // if lhs < 0 and rhs >= 0, return true
  // if lhs >= 0 and rhs < 0, return false
  // if both are negative, negate and use unsigned greaterEqual

  branchOnSignBit(lhs, Cell{lhs, MacroCell::Flag},
		  [&] /* lhs < 0 */ { 
		    branchOnSignBit(rhs, Cell{rhs, MacroCell::Flag},
				    [&] /* rhs < 0 */ {
				      // Both negative -> negate both and use unsigned greater-equal
				      negateSlot(lhs);
				      Slot rhsCopy = getTemp(rhs.type());
				      assignSlot(rhsCopy, rhs);
				      negateSlot(rhsCopy);
				      slotGreaterEqualSlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), true);
				      freeTempSlot(rhsCopy);
				    },
				    [&] /* rhs >= 0 */ {
				      setSlotToBool(lhs, true);
				    });
		  },
		  [&] /* lhs >= 0 */ {
		    branchOnSignBit(rhs, Cell{rhs, MacroCell::Flag},
				    [&] /* rhs < 0 */ {
				      setSlotToBool(lhs, false);
				    },
				    [&] /* rhs >= 0 */ {
				      // Both are positive, so we can use the unsigned version
				      slotLessEqualSlotUnsigned(lhs.unsignedView(), rhs.unsignedView());
				    });
		  }); 
}


void Assembler::slotGreaterConst(Slot lhs, int val) {
  assert(types::isInteger(lhs.type()));
  
  if (types::isUnsignedInteger(lhs.type())) return slotGreaterConstUnsigned(lhs, val);
  if (types::isSignedInteger(lhs.type()))   return slotGreaterConstSigned(lhs, val);
  std::unreachable();
}

void Assembler::slotGreaterConstUnsigned(Slot lhs, int val) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(val >= 0);
  
  pushPtr();

  // If val is maximal, the result must be false
  if (lhs.type()->usesValue1() && ((val & 0xffff) == 0xffff)) {
    moveTo(lhs); 
    setToValue16(0, Cell{lhs, MacroCell::Value1});
    popPtr();
    return;
  }
  else if ((val & 0xff) == 0xff) {
    moveTo(lhs);
    setToValue(0);
    popPtr();
    return;
  }

  Slot valSlot = getTemp(((val >> 8) & 0xff) ? literal::u16(val) : literal::u8(val));
  slotGreaterSlotUnsigned(lhs, valSlot, true);
  freeTempSlot(valSlot);
  
  popPtr();
}

void Assembler::slotGreaterConstSigned(Slot lhs, int val) {
  assert(types::isSignedInteger(lhs.type()));
  
  slotLessEqualConstSigned(lhs, val);
  notSlot(lhs);
}

void Assembler::slotGreaterSlot(Slot lhs, Slot rhs) {
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return slotGreaterSlotUnsigned(lhs, rhs);
  if (types::isSignedInteger(lhs.type()))   return slotGreaterSlotSigned(lhs, rhs);
  std::unreachable();
}

void Assembler::slotGreaterSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));
  
  pushPtr();

  bool freeRhsCopy = false;
  Slot rhsCopy = [&] {
    if (destroyRhs) return rhs;
    Slot const tmp = getTemp(rhs.type());
    assignSlot(tmp, rhs);
    freeRhsCopy = true;
    return tmp;
  }();
    
  moveTo(lhs);  
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    greater16Destructive(Cell{lhs, MacroCell::Value1},
			 Cell{rhsCopy, MacroCell::Value0},
			 Cell{rhsCopy, MacroCell::Value1},
			 Temps<3>::select(lhs, MacroCell::Scratch0,
					  lhs, MacroCell::Scratch1,
					  rhsCopy, MacroCell::Scratch0));
  } else {
    greaterDestructive(Cell{rhsCopy, MacroCell::Value0},
		       Temps<1>::select(lhs, MacroCell::Scratch0));
  }

  popPtr();

  if (freeRhsCopy) freeTempSlot(rhsCopy);
}

void Assembler::slotGreaterSlotSigned(Slot lhs, Slot rhs) {
  assert(types::isSignedInteger(lhs.type()));
  assert(types::isSignedInteger(rhs.type()));

  // if both are positive, use unsigned version
  // if lhs < 0 and rhs >= 0, return false
  // if lhs >= 0 and rhs < 0, return true
  // if both are negative, negate and use unsigned less

  branchOnSignBit(lhs, Cell{lhs, MacroCell::Flag},
		  [&] /* lhs < 0 */ { 
		    branchOnSignBit(rhs, Cell{rhs, MacroCell::Flag},
				    [&] /* rhs < 0 */ {
				      // Both negative -> negate both and use unsigned greater-equal
				      negateSlot(lhs);
				      Slot rhsCopy = getTemp(rhs.type());
				      assignSlot(rhsCopy, rhs);
				      negateSlot(rhsCopy);
				      slotLessSlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), true);
				      freeTempSlot(rhsCopy);
				    },
				    [&] /* rhs >= 0 */ {
				      setSlotToBool(lhs, false);
				    });
		  },
		  [&] /* lhs >= 0 */ {
		    branchOnSignBit(rhs, Cell{rhs, MacroCell::Flag},
				    [&] /* rhs < 0 */ {
				      setSlotToBool(lhs, true);
				    },
				    [&] /* rhs >= 0 */ {
				      // Both are positive, so we can use the unsigned version
				      slotGreaterSlotUnsigned(lhs.unsignedView(), rhs.unsignedView());
				    });
		  }); 
}
  
void Assembler::slotGreaterEqualConst(Slot lhs, int val) {
  assert(types::isInteger(lhs.type()));
  
  if (types::isUnsignedInteger(lhs.type())) return slotGreaterEqualConstUnsigned(lhs, val);
  if (types::isSignedInteger(lhs.type()))   return slotGreaterEqualConstSigned(lhs, val);
  std::unreachable();
}

void Assembler::slotGreaterEqualConstUnsigned(Slot lhs, int val) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(val >= 0);
  
  pushPtr();

  // If val is 0, the result must be true
  if (lhs.type()->usesValue1() && (val == 0)) {
    moveTo(lhs); 
    setToValue16(1, Cell{lhs, MacroCell::Value1});
    popPtr();
    return;
  }
  else if (val == 0) {
    moveTo(lhs);
    setToValue(1);
    popPtr();
    return;
  }

  Slot valSlot = getTemp(((val >> 8) & 0xff) ? literal::u16(val) : literal::u8(val));
  slotGreaterEqualSlotUnsigned(lhs, valSlot, true);
  freeTempSlot(valSlot);

  popPtr();
}

void Assembler::slotGreaterEqualConstSigned(Slot lhs, int val) {
  assert(types::isSignedInteger(lhs.type()));

  slotLessConstSigned(lhs, val);
  notSlot(lhs);
}

void Assembler::slotGreaterEqualSlot(Slot lhs, Slot rhs) {
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return slotGreaterEqualSlotUnsigned(lhs, rhs);
  if (types::isSignedInteger(lhs.type()))   return slotGreaterEqualSlotSigned(lhs, rhs);
  std::unreachable();
}


void Assembler::slotGreaterEqualSlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs) {
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));
  
  pushPtr();

  bool freeRhsCopy = false;
  Slot rhsCopy = [&] {
    if (destroyRhs) return rhs;
    Slot const tmp = getTemp(rhs.type());
    assignSlot(tmp, rhs);
    freeRhsCopy = true;
    return tmp;
  }();

  moveTo(lhs);  
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    greaterOrEqual16Destructive(Cell{lhs, MacroCell::Value1},
				Cell{rhsCopy, MacroCell::Value0},
				Cell{rhsCopy, MacroCell::Value1},
				Temps<4>::select(lhs, MacroCell::Scratch0,
						 lhs, MacroCell::Scratch1,
						 rhsCopy, MacroCell::Scratch0,
						 rhsCopy, MacroCell::Scratch1));
  } else {
    greaterOrEqualDestructive(Cell{rhsCopy, MacroCell::Value0},
			      Temps<1>::select(lhs, MacroCell::Scratch0));
  }

  popPtr();

  if (freeRhsCopy) freeTempSlot(rhsCopy);
}

void Assembler::slotGreaterEqualSlotSigned(Slot lhs, Slot rhs) {
  assert(types::isSignedInteger(lhs.type()));
  assert(types::isSignedInteger(rhs.type()));

  // if both are positive, use unsigned version
  // if lhs < 0 and rhs >= 0, return false
  // if lhs >= 0 and rhs < 0, return true
  // if both are negative, negate and use unsigned less-equal

  branchOnSignBit(lhs, Cell{lhs, MacroCell::Flag},
		  [&] /* lhs < 0 */ { 
		    branchOnSignBit(rhs, Cell{rhs, MacroCell::Flag},
				    [&] /* rhs < 0 */ {
				      // Both negative -> negate both and use unsigned greater-equal
				      negateSlot(lhs);
				      Slot rhsCopy = getTemp(rhs.type());
				      assignSlot(rhsCopy, rhs);
				      negateSlot(rhsCopy);
				      slotLessEqualSlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), true);
				      freeTempSlot(rhsCopy);
				    },
				    [&] /* rhs >= 0 */ {
				      setSlotToBool(lhs, false);
				    });
		  },
		  [&] /* lhs >= 0 */ {
		    branchOnSignBit(rhs, Cell{rhs, MacroCell::Flag},
				    [&] /* rhs < 0 */ {
				      setSlotToBool(lhs, true);
				    },
				    [&] /* rhs >= 0 */ {
				      // Both are positive, so we can use the unsigned version
				      slotGreaterEqualSlotUnsigned(lhs.unsignedView(), rhs.unsignedView());
				    });
		  });   
}

void Assembler::eqDestructive(Cell other, Temps<1> tmp) {
  auto [cur, oth] = getFieldIndices(_dp.current(), other);
  emit<primitive::Equal>(cur, oth);
}

void Assembler::eqConstructive(Cell result, Cell other, Temps<1> tmp) {
  pushPtr();
  copyField(result, tmp);
  moveTo(result);
  eqDestructive(other, tmp);
  popPtr();
}

void Assembler::eq16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<1> tmp) {
  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;

  pushPtr();
  moveTo(currentLow);
  eqDestructive(otherLow, tmp.select<0>());
  moveTo(currentHigh);
  eqDestructive(otherHigh, tmp.select<0>());
  moveTo(currentLow);
  andDestructive(currentHigh, tmp.select<0>());
  popPtr();
}

void Assembler::eq16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<4> tmp) { 

  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;
  Cell const otherCopyLow = tmp.get<0>();
  Cell const otherCopyHigh = tmp.get<1>();
  Cell const resultHigh = tmp.get<2>();
  
  pushPtr();

  moveTo(currentLow);  copyField(result, tmp.select<3>());
  moveTo(currentHigh); copyField(resultHigh, tmp.select<3>());
  moveTo(otherLow);    copyField(otherCopyLow, tmp.select<3>());
  moveTo(otherHigh);   copyField(otherCopyHigh, tmp.select<3>());

  moveTo(result);
  eq16Destructive(resultHigh, otherCopyLow, otherCopyHigh, tmp.select<3>());
  popPtr();
}


void Assembler::lessDestructive(Cell other, Temps<1> tmp) { 
  auto [cur, oth, scratch] = getFieldIndices(_dp.current(), other, tmp.get<0>());
  emit<primitive::Less>(cur, oth, scratch);
}

void Assembler::lessConstructive(Cell result, Cell other, Temps<2> tmp) {
  Cell const &otherCopy = tmp.get<0>();
  pushPtr();
  copyField(result, tmp.select<1>());
  moveTo(other);
  copyField(otherCopy, tmp.select<1>());
  moveTo(result);
  lessDestructive(otherCopy, tmp.select<1>());
  popPtr();
}

void Assembler::less16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<4> tmp) {
  // xH < yH || (xH == yH && xL < yL)

  pushPtr();
  
  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;
  Cell const currentCopyHigh = tmp.get<0>();
  Cell const otherCopyHigh = tmp.get<1>();

  moveTo(otherHigh);   copyField(otherCopyHigh, tmp.select<2>());
  moveTo(currentHigh); copyField(currentCopyHigh, tmp.select<2>());
  
  moveTo(currentHigh);
  lessDestructive(otherHigh, tmp.select<2>());
  Cell const highByteLess = currentHigh;
  // otherHigh cleared
  // currentHigh: xH < yH

  moveTo(currentCopyHigh);
  eqDestructive(otherCopyHigh, tmp.select<2>());
  Cell const highByteEqual = currentCopyHigh;
  // otherCopyHigh cleared
  // currentCopyHigh: xH == yH
  
  moveTo(currentLow);
  lessDestructive(otherLow, tmp.select<2>()); 
  Cell const lowByteLess = currentLow;
  // otherLow cleared
  // currentLow: xL < yL

  moveTo(highByteEqual); 
  andDestructive(lowByteLess, tmp.select<2>()); 
  // currentLow cleared
  // currentCopyHigh: xH == yH && xL < yL
  
  orDestructive(highByteLess);
  // currentHigh cleared
  // currentCopyHigh: xH < yH || (xH == yH && xL < yL)
  
  moveField(currentLow);
  // currentCopyHigh cleared
  // currentLow contains result

  popPtr();
  
}

void Assembler::less16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<8> tmp) {

  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;
  Cell const otherCopyLow = tmp.get<0>();
  Cell const otherCopyHigh = tmp.get<1>();
  Cell const resultHigh = tmp.get<2>();
  
  pushPtr();

  moveTo(currentLow);  copyField(result, tmp.select<3>());
  moveTo(currentHigh); copyField(resultHigh, tmp.select<3>());
  moveTo(otherLow);    copyField(otherCopyLow, tmp.select<3>());
  moveTo(otherHigh);   copyField(otherCopyHigh, tmp.select<3>());

  moveTo(result);
  less16Destructive(resultHigh, otherCopyLow, otherCopyHigh, tmp.select<4, 5, 6, 7>());
  popPtr();  
}


void Assembler::lessOrEqualDestructive(Cell other, Temps<1> tmp) {
  auto [cur, oth, scratch] = getFieldIndices(_dp.current(), other, tmp.get<0>());
  emit<primitive::LessOrEqual>(cur, oth, scratch);
}

void Assembler::lessOrEqualConstructive(Cell result, Cell other, Temps<2> tmp) {
  Cell const &otherCopy = tmp.get<0>();
  pushPtr();
  copyField(result, tmp.select<1>());
  moveTo(other);
  copyField(otherCopy, tmp.select<1>());
  moveTo(result);
  lessOrEqualDestructive(otherCopy, tmp.select<1>());
  popPtr();
}

void Assembler::lessOrEqual16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<3> tmp) {
  greater16Destructive(high, otherLow, otherHigh, tmp);
  notDestructive(tmp.select<0>());
}

void Assembler::lessOrEqual16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<6> tmp) {

  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;
  Cell const otherCopyLow = tmp.get<0>();
  Cell const otherCopyHigh = tmp.get<1>();
  Cell const resultHigh = tmp.get<2>();
  
  pushPtr();

  moveTo(currentLow);  copyField(result, tmp.select<3>());
  moveTo(currentHigh); copyField(resultHigh, tmp.select<3>());
  moveTo(otherLow);    copyField(otherCopyLow, tmp.select<3>());
  moveTo(otherHigh);   copyField(otherCopyHigh, tmp.select<3>());

  moveTo(result);
  lessOrEqual16Destructive(resultHigh, otherCopyLow, otherCopyHigh, tmp.select<3, 4, 5>());
  popPtr();  

}

void Assembler::greaterDestructive(Cell other, Temps<1> tmp) {
  auto [cur, oth, scratch] = getFieldIndices(_dp.current(), other, tmp.get<0>());
  emit<primitive::Greater>(cur, oth, scratch);
}

void Assembler::greaterConstructive(Cell result, Cell other, Temps<2> tmp) {
  Cell const &otherCopy = tmp.get<0>();
  pushPtr();
  copyField(result, tmp.select<1>());
  moveTo(other);
  copyField(otherCopy, tmp.select<1>());
  moveTo(result);
  greaterDestructive(otherCopy, tmp.select<1>());
  popPtr();
}


void Assembler::greater16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<3> tmp) {
  // xH > yH || (xH == yH && xL > yL)

  pushPtr();
  
  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;
  Cell const currentCopyHigh = tmp.get<0>();
  Cell const otherCopyHigh = tmp.get<1>();

  moveTo(otherHigh);   copyField(otherCopyHigh, tmp.select<2>());
  moveTo(currentHigh); copyField(currentCopyHigh, tmp.select<2>());
  
  moveTo(currentHigh);
  greaterDestructive(otherHigh, tmp.select<2>());
  Cell const highByteGreater = currentHigh;
  // otherHigh cleared
  // currentHigh: xH > yH
  
  moveTo(currentCopyHigh);
  eqDestructive(otherCopyHigh, tmp.select<2>());
  Cell const highByteEqual = currentCopyHigh;
  // otherCopyHigh cleared
  // currentCopyHigh: xH == yH
  
  moveTo(currentLow);
  greaterDestructive(otherLow, tmp.select<2>()); 
  Cell const lowByteGreater = currentLow;
  // otherLow cleared
  // currentLow: xL > yL

  moveTo(highByteEqual); 
  andDestructive(lowByteGreater, tmp.select<2>()); 
  // currentLow cleared
  // currentCopyHigh: xH == yH && xL > yL
  
  orDestructive(highByteGreater);
  // currentHigh cleared
  // currentCopyHigh: xH > yH || (xH == yH && xL > yL)
  
  moveField(currentLow);
  // currentCopyHigh cleared
  // currentLow contains result

  popPtr();
  
}

void Assembler::greater16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<6> tmp) {

  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;
  Cell const otherCopyLow = tmp.get<0>();
  Cell const otherCopyHigh = tmp.get<1>();
  Cell const resultHigh = tmp.get<2>();
  
  pushPtr();

  moveTo(currentLow);  copyField(result, tmp.select<3>());
  moveTo(currentHigh); copyField(resultHigh, tmp.select<3>());
  moveTo(otherLow);    copyField(otherCopyLow, tmp.select<3>());
  moveTo(otherHigh);   copyField(otherCopyHigh, tmp.select<3>());

  moveTo(result);
  greater16Destructive(resultHigh, otherCopyLow, otherCopyHigh, tmp.select<3, 4, 5>());
  popPtr();  
}



void Assembler::greaterOrEqualDestructive(Cell other, Temps<1> tmp) {
  auto [cur, oth, scratch] = getFieldIndices(_dp.current(), other, tmp.get<0>());
  emit<primitive::GreaterOrEqual>(cur, oth, scratch);
}

void Assembler::greaterOrEqualConstructive(Cell result, Cell other, Temps<2> tmp) {
  Cell const &otherCopy = tmp.get<0>();
  pushPtr();
  copyField(result, tmp.select<1>());
  moveTo(other);
  copyField(otherCopy, tmp.select<1>());
  moveTo(result);
  greaterOrEqualDestructive(otherCopy, tmp.select<1>());
  popPtr();
}

void Assembler::greaterOrEqual16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<4> tmp) {
  less16Destructive(high, otherLow, otherHigh, tmp);
  notDestructive(tmp.select<0>());
}

void Assembler::greaterOrEqual16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<8> tmp) {

  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;
  Cell const otherCopyLow = tmp.get<0>();
  Cell const otherCopyHigh = tmp.get<1>();
  Cell const resultHigh = tmp.get<2>();\
  
  pushPtr();

  moveTo(currentLow);  copyField(result, tmp.select<3>());
  moveTo(currentHigh); copyField(resultHigh, tmp.select<3>());
  moveTo(otherLow);    copyField(otherCopyLow, tmp.select<3>());
  moveTo(otherHigh);   copyField(otherCopyHigh, tmp.select<3>());

  moveTo(result);
  greaterOrEqual16Destructive(resultHigh, otherCopyLow, otherCopyHigh, tmp.select<4, 5, 6, 7>());
  popPtr();  

}
