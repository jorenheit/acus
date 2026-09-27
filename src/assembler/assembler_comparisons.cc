// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::setSlotToBool(Slot slot, bool value) {
  setSlotToValue(slot, value);
}

void Assembler::slotEqualConst(Slot lhs, int val) {

  pushPtr();

  moveTo(lhs);  
  subConstFromSlot(lhs, val);
  
  if (lhs.type()->usesValue1()) {
    not16Destructive(ws::promiseClean16(lhs));
  } else {
    notDestructive(ws::promiseClean8(lhs));
  }

  popPtr();
}

void Assembler::slotEqualSlot(Slot lhs, Slot rhs) {
  pushPtr();
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  moveTo(lhs);

  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    eq16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
  } else {
    eqDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }

  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::slotNotEqualConst(Slot lhs, int val) {
  pushPtr();
  slotEqualConst(lhs, val);
  notDestructive(ws::promiseClean8(lhs));
  popPtr();
}

void Assembler::slotNotEqualSlot(Slot lhs, Slot rhs) {
  pushPtr();
  slotEqualSlot(lhs, rhs);
  notDestructive(ws::promiseClean8(lhs));
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
    setSlotToValue(lhs, 0);
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
    branchOnSignBit(lhs, // Cell{lhs, MacroCell::Flag},
		    [&] /* lhs  < 0 */ { setSlotToBool(lhs, true); },
		    [&] /* lhs >= 0 */ { slotLessConstUnsigned(lhs.unsignedView(), val); });

		    
  }
  else if (val < 0) {
    // If no sign bit, return 0
    // If sign bit is set, take absolute value and do unsigned comparison between absolute values,
    // but use greater-than algorithm.

    branchOnSignBit(lhs, // Cell{lhs, MacroCell::Flag},
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
    less16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
  } else {
    lessDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
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
  
  branchOnSignBit(lhs,// Cell{lhs, MacroCell::Flag},
		  [&] /* lhs < 0 */ { 
		    branchOnSignBit(rhs,// Cell{rhs, MacroCell::Flag},
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
		    branchOnSignBit(rhs, // Cell{rhs, MacroCell::Flag},
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
  
  // If val is maximal, the result must be true
  if ((lhs.type()->usesValue1() && (val & 0xffff) == 0xffff) || (val & 0xff) == 0xff) {
    setSlotToValue(lhs, 1);
    return;
  }

  pushPtr();
  
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
    
    branchOnSignBit(lhs, //Cell{lhs, MacroCell::Flag},
		    [&] /* lhs  < 0 */ { setSlotToBool(lhs, true); },
		    [&] /* lhs >= 0 */ { slotLessEqualConstUnsigned(lhs.unsignedView(), val); });
  }

  if (val < 0) {
    // if sign bit is set -> return abs(lhs) >= abs(val)
    // if not, return 0
    branchOnSignBit(lhs,// Cell{lhs, MacroCell::Flag},
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
    lessOrEqual16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
  } else {
    lessOrEqualDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
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

  branchOnSignBit(lhs,// Cell{lhs, MacroCell::Flag},
		  [&] /* lhs < 0 */ { 
		    branchOnSignBit(rhs, //Cell{rhs, MacroCell::Flag},
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
		    branchOnSignBit(rhs, //Cell{rhs, MacroCell::Flag},
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
  

  // If val is maximal, the result must be false
  if ((lhs.type()->usesValue1() && (val & 0xffff) == 0xffff) || (val & 0xff) == 0xff) {
    setSlotToValue(lhs, 0);
    return;
  }

  pushPtr();
  
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
    greater16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
  } else {
    greaterDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
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

  branchOnSignBit(lhs,// Cell{lhs, MacroCell::Flag},
		  [&] /* lhs < 0 */ { 
		    branchOnSignBit(rhs, //Cell{rhs, MacroCell::Flag},
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
		    branchOnSignBit(rhs, //Cell{rhs, MacroCell::Flag},
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
  

  // If val is 0, the result must be true
  if (val == 0) {
    setSlotToValue(lhs, 1);
    return;
  }

  pushPtr();

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
    greaterOrEqual16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
  } else {
    greaterOrEqualDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
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

  branchOnSignBit(lhs,// Cell{lhs, MacroCell::Flag},
		  [&] /* lhs < 0 */ { 
		    branchOnSignBit(rhs, //Cell{rhs, MacroCell::Flag},
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
		    branchOnSignBit(rhs,// Cell{rhs, MacroCell::Flag},
				    [&] /* rhs < 0 */ {
				      setSlotToBool(lhs, true);
				    },
				    [&] /* rhs >= 0 */ {
				      // Both are positive, so we can use the unsigned version
				      slotGreaterEqualSlotUnsigned(lhs.unsignedView(), rhs.unsignedView());
				    });
		  });   
}
