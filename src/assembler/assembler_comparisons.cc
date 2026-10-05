// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::setSlotToBool(Slot slot, bool value) {
  setSlotToValue(slot, value);
}

void Assembler::slotEqualConst(Slot lhs, int val) {
 if (val != 0) subConstFromSlot(lhs, val);
 return notSlot(lhs);
}

void Assembler::slotNotEqualConst(Slot lhs, int val) {
  if (val != 0) subConstFromSlot(lhs, val);
  return boolSlot(lhs);
}

void Assembler::slotEqualSlot(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, true);
  destroyRhs = destroyRhs && lhs != rhs;
  bool const widenRhs = lhs.type()->usesValue1() && !rhs.type()->usesValue1();
  Slot const rhsWork = [&] {
    if (widenRhs) {
      Slot copy = getTemp(types::isSignedInteger(rhs.type()) ? ts::s16() : ts::u16());
      assignSlot(copy, rhs, destroyRhs ? TransferMode::Move : TransferMode::Copy);
      return copy;
    }
    if (destroyRhs) return rhs;
    Slot copy = getTemp(rhs.type());
    assignSlot(copy, rhs);
    return copy;
  }();

  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    eq16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsWork));
  } else {
    eqDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsWork));
  }

  if (widenRhs || !destroyRhs) freeTempSlot(rhsWork);
}


void Assembler::slotNotEqualSlot(Slot lhs, Slot rhs, bool destroyRhs) {
  slotEqualSlot(lhs, rhs, destroyRhs);
  notDestructive(Cell{lhs, MacroCell::Value0}, Cell{lhs, MacroCell::Scratch0});
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
  int const max = lhs.type()->usesValue1() ? 65535 : 255;

  if      (val == 0)  return setSlotToValue(lhs, 0);
  else if (val > max) return setSlotToValue(lhs, 1);
  else if (val == 1) return notSlot(lhs);
  else if (val == max) return slotNotEqualConst(lhs, val);
  
  Slot valSlot = getTemp(((val >> 8) & 0xff) ? literal::u16(val) : literal::u8(val));
  slotLessSlotUnsigned(lhs, valSlot, true);
  freeTempSlot(valSlot);
}

void Assembler::slotLessConstSigned(Slot lhs, int val) {
  assert(types::isSignedInteger(lhs.type()));
  int const min = lhs.type()->usesValue1() ? -32768 : -128;
  int const max = lhs.type()->usesValue1() ?  32767 :  127;
  
  if      (val == 0)   return signBitSlot(lhs);
  else if (val <= min) return setSlotToValue(lhs, 0);
  else if (val > max)  return setSlotToValue(lhs, 1);
  else if (val == min + 1) return slotEqualConst(lhs, min);
  else if (val == max) return slotNotEqualConst(lhs, max);
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

void Assembler::slotLessSlot(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, false);
  destroyRhs = destroyRhs && lhs != rhs;
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return slotLessSlotUnsigned(lhs, rhs, destroyRhs);
  if (types::isSignedInteger(lhs.type()))   return slotLessSlotSigned(lhs, rhs, destroyRhs);
  std::unreachable();
}

void Assembler::slotLessSlotUnsigned(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, false);
  destroyRhs = destroyRhs && lhs != rhs;
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));
  
  Slot const rhsWork = [&] {
    if (destroyRhs) return rhs;
    Slot const copy = getTemp(rhs.type());
    assignSlot(copy, rhs);
    return copy;
  }();

  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    less16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsWork));
  } else {
    lessDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsWork));
  }

  if (!destroyRhs) freeTempSlot(rhsWork);
}

void Assembler::slotLessSlotSigned(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, false);
  destroyRhs = destroyRhs && lhs != rhs;
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
				      Slot rhsCopy = destroyRhs ? rhs : getTemp(rhs.type());
				      if (!destroyRhs) assignSlot(rhsCopy, rhs);
				      negateSlot(rhsCopy);
				      slotGreaterSlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), true);
				      if (!destroyRhs) freeTempSlot(rhsCopy);
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
				      slotLessSlotUnsigned(lhs.unsignedView(), rhs.unsignedView(), destroyRhs);
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

  int const max = lhs.type()->usesValue1() ? 65535 : 255;
  if (val == 0)   return notSlot(lhs);
  if (val >= max) return setSlotToValue(lhs, 1);
  
  Slot valSlot = getTemp(((val >> 8) & 0xff) ? literal::u16(val) : literal::u8(val));
  slotLessEqualSlotUnsigned(lhs, valSlot, true);
  freeTempSlot(valSlot);
}

void Assembler::slotLessEqualConstSigned(Slot lhs, int val) {
  assert(types::isSignedInteger(lhs.type()));

  int const min = lhs.type()->usesValue1() ? -32768 : -128;
  int const max = lhs.type()->usesValue1() ?  32767 :  127;

  if (val >= max) return setSlotToValue(lhs, 1);
  if (val < min) return setSlotToValue(lhs, 0);
  if (val == min) return slotEqualConst(lhs, min);
  if (val == -1) return signBitSlot(lhs);
  
  if (val >= 0) {
    // if sign bit is set -> return 1
    // if not, use unsigned version
    
    branchOnSignBit(lhs,
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

void Assembler::slotLessEqualSlot(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, true);
  destroyRhs = destroyRhs && lhs != rhs;
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return slotLessEqualSlotUnsigned(lhs, rhs, destroyRhs);
  if (types::isSignedInteger(lhs.type()))   return slotLessEqualSlotSigned(lhs, rhs, destroyRhs);
  std::unreachable();
}

void Assembler::slotLessEqualSlotUnsigned(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, true);
  destroyRhs = destroyRhs && lhs != rhs;
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));

  Slot const rhsWork = [&] {
    if (destroyRhs) return rhs;
    Slot const copy = getTemp(rhs.type());
    assignSlot(copy, rhs);
    return copy;
  }();

  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    lessOrEqual16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsWork));
  } else {
    lessOrEqualDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsWork));
  }

  if (!destroyRhs) freeTempSlot(rhsWork);
}

void Assembler::slotLessEqualSlotSigned(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, true);
  destroyRhs = destroyRhs && lhs != rhs;
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
				      Slot rhsCopy = destroyRhs ? rhs : getTemp(rhs.type());
				      if (!destroyRhs) assignSlot(rhsCopy, rhs);
				      negateSlot(rhsCopy);
				      slotGreaterEqualSlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), true);
				      if (!destroyRhs) freeTempSlot(rhsCopy);
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
				      slotLessEqualSlotUnsigned(lhs.unsignedView(), rhs.unsignedView(), destroyRhs);
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
  
  int const max = lhs.type()->usesValue1() ? 65535 : 255;
  if (val == 0)   return boolSlot(lhs);
  if (val >= max) return setSlotToValue(lhs, 0);
  if (val == max - 1) return slotEqualConst(lhs, max);
  
  Slot valSlot = getTemp(((val >> 8) & 0xff) ? literal::u16(val) : literal::u8(val));
  slotGreaterSlotUnsigned(lhs, valSlot, true);
  freeTempSlot(valSlot);
}

void Assembler::slotGreaterConstSigned(Slot lhs, int val) {
  assert(types::isSignedInteger(lhs.type()));
  
  int const min = lhs.type()->usesValue1() ? -32768 : -128;
  int const max = lhs.type()->usesValue1() ?  32767 :  127;
  if (val == min) return slotNotEqualConst(lhs, min);
  if (val == max - 1) return slotEqualConst(lhs, max);

  slotLessEqualConstSigned(lhs, val);
  notDestructive(Cell{lhs, MacroCell::Value0}, Cell{lhs, MacroCell::Scratch0});
}

void Assembler::slotGreaterSlot(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, false);
  destroyRhs = destroyRhs && lhs != rhs;
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return slotGreaterSlotUnsigned(lhs, rhs, destroyRhs);
  if (types::isSignedInteger(lhs.type()))   return slotGreaterSlotSigned(lhs, rhs, destroyRhs);
  std::unreachable();
}

void Assembler::slotGreaterSlotUnsigned(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, false);
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));
  destroyRhs = destroyRhs && lhs != rhs;
  
  Slot const rhsWork = [&] {
    if (destroyRhs) return rhs;
    Slot const copy = getTemp(rhs.type());
    assignSlot(copy, rhs);
    return copy;
  }();
    
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    greater16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsWork));
  } else {
    greaterDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsWork));
  }

  if (!destroyRhs) freeTempSlot(rhsWork);
}

void Assembler::slotGreaterSlotSigned(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, false);
  assert(types::isSignedInteger(lhs.type()));
  assert(types::isSignedInteger(rhs.type()));
  destroyRhs = destroyRhs && lhs != rhs;

  // if both are positive, use unsigned version
  // if lhs < 0 and rhs >= 0, return false
  // if lhs >= 0 and rhs < 0, return true
  // if both are negative, negate and use unsigned less

  branchOnSignBit(lhs,// Cell{lhs, MacroCell::Flag},
		  [&] /* lhs < 0 */ { 
		    branchOnSignBit(rhs, //Cell{rhs, MacroCell::Flag},
				    [&] /* rhs < 0 */ {
				      // Both negative -> negate both and use unsigned less-than
				      negateSlot(lhs);
				      Slot rhsCopy = destroyRhs ? rhs : getTemp(rhs.type());
				      if (!destroyRhs) assignSlot(rhsCopy, rhs);
				      negateSlot(rhsCopy);
				      slotLessSlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), true);
				      if (!destroyRhs) freeTempSlot(rhsCopy);
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
				      slotGreaterSlotUnsigned(lhs.unsignedView(), rhs.unsignedView(), destroyRhs);
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
  
  int const max = lhs.type()->usesValue1() ?  65535 : 255;
  if (val == 0)  return setSlotToValue(lhs, 1);
  if (val > max) return setSlotToValue(lhs, 0);
  if (val == 1) return boolSlot(lhs);
  if (val == max) return slotEqualConst(lhs, max);

  Slot valSlot = getTemp(((val >> 8) & 0xff) ? literal::u16(val) : literal::u8(val));
  slotGreaterEqualSlotUnsigned(lhs, valSlot, true);
  freeTempSlot(valSlot);
}

void Assembler::slotGreaterEqualConstSigned(Slot lhs, int val) {
  assert(types::isSignedInteger(lhs.type()));

  slotLessConstSigned(lhs, val);
  notDestructive(Cell{lhs, MacroCell::Value0}, Cell{lhs, MacroCell::Scratch0});
}

void Assembler::slotGreaterEqualSlot(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, true);
  destroyRhs = destroyRhs && lhs != rhs;
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));
  assert(types::cast<types::IntegerType>(lhs.type())->signedness() ==
	 types::cast<types::IntegerType>(rhs.type())->signedness());
    
  if (types::isUnsignedInteger(lhs.type())) return slotGreaterEqualSlotUnsigned(lhs, rhs, destroyRhs);
  if (types::isSignedInteger(lhs.type()))   return slotGreaterEqualSlotSigned(lhs, rhs, destroyRhs);
  std::unreachable();
}


void Assembler::slotGreaterEqualSlotUnsigned(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, true);
  destroyRhs = destroyRhs && lhs != rhs;
  assert(types::isUnsignedInteger(lhs.type()));
  assert(types::isUnsignedInteger(rhs.type()));
  
  Slot const rhsWork = [&] {
    if (destroyRhs) return rhs;
    Slot const copy = getTemp(rhs.type());
    assignSlot(copy, rhs);
    return copy;
  }();

  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    greaterOrEqual16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsWork));
  } else {
    greaterOrEqualDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsWork));
  }
  if (!destroyRhs) freeTempSlot(rhsWork);
}

void Assembler::slotGreaterEqualSlotSigned(Slot lhs, Slot rhs, bool destroyRhs) {
  if (lhs == rhs) return setSlotToBool(lhs, true);
  destroyRhs = destroyRhs && lhs != rhs;
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
				      // Both negative -> negate both and use unsigned less-equal
				      negateSlot(lhs);
				      Slot rhsCopy = destroyRhs ? rhs : getTemp(rhs.type());
				      if (!destroyRhs) assignSlot(rhsCopy, rhs);
				      negateSlot(rhsCopy);
				      slotLessEqualSlotUnsigned(lhs.unsignedView(), rhsCopy.unsignedView(), true);
				      if (!destroyRhs) freeTempSlot(rhsCopy);
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
				      slotGreaterEqualSlotUnsigned(lhs.unsignedView(), rhs.unsignedView(), destroyRhs);
				    });
		  });   
}

void Assembler::eqDestructive(Cell x, Cell y) {
  loop(x, [&] {
    dec(x);
    dec(y);
  });
  inc(x);

  loop(y, [&] {
    zeroCell(y);
    dec(x);
  });
}

void Assembler::lessDestructive(Cell x, Cell y, Cell tmp) {
  // Decrement x and y in lockstep. If y is still non-zero when x reaches
  // zero, x < y. tmp temporarily holds y while forcing the inner loop to run
  // only once per x decrement.
  loop(x, [&] {
    dec(x);
    loop(y, [&] {
      dec(y);
      moveField(y, tmp);
    });
    moveField(tmp, y);
  });

  loop(y, [&] {
    zeroCell(y);
    inc(x);
  });
}

void Assembler::greaterDestructive(Cell x, Cell y, Cell tmp) {
  // Decrement y while parking the remainder of x in tmp. If x has anything
  // left when y reaches zero, x > y. Normalize the result even when y
  // started at zero and the main loop did not execute.
  loop(y, [&] {
    dec(y);

    // Restore the remainder from the previous iteration. x is known zero
    // here after the first iteration; on the first iteration tmp is zero.
    loop(tmp, [&] {
      dec(tmp);
      inc(x);
    });

    loop(x, [&] {
      dec(x);
      moveField(x, tmp);
    });
  });

  loop(tmp, [&] {
    zeroCell(tmp);
    inc(x);
  });
  boolDestructive(x, closestTo(x, {y, tmp}));
}

void Assembler::lessOrEqualDestructive(Cell x, Cell y, Cell tmp) {
  greaterDestructive(x, y, tmp);
  notDestructive(x, closestTo(x, {y, tmp}));
}

void Assembler::greaterOrEqualDestructive(Cell x, Cell y, Cell tmp) {
  lessDestructive(x, y, tmp);
  notDestructive(x, closestTo(x, {y, tmp}));
}
