// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::andSlotWithConst(Slot const &lhs, int val) {
  pushPtr();
  moveTo(lhs);

  if (val == 0) {
    zeroCell();
    moveTo(lhs, MacroCell::Value1);
    zeroCell();
    popPtr();
    return;
  }

  if (lhs.type->usesValue1()) {
    bool16Destructive(Cell{lhs, MacroCell::Value1},
		      Temps<1>::select(lhs, MacroCell::Scratch0));
  } else {
    boolDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
  }
  
  popPtr();
}

void Assembler::andSlotWithSlot(Slot const &lhs, Slot const &rhs) {
  pushPtr();

  Slot const rhsCopy = getTemp(rhs.type);
  assignSlot(rhsCopy, rhs);
  moveTo(lhs);

  if (lhs.type->usesValue1() || rhs.type->usesValue1()) {
    and16Destructive(Cell{lhs, MacroCell::Value1},
		     Cell{rhsCopy, MacroCell::Value0},
		     Cell{rhsCopy, MacroCell::Value1},
		     Temps<1>::select(lhs, MacroCell::Scratch0));
		     
  } else {
    andDestructive(Cell{rhsCopy, MacroCell::Value0},
		   Temps<1>::select(lhs, MacroCell::Scratch0));
  }

  popPtr();
  freeTempSlot(rhsCopy);
}


void Assembler::nandSlotWithConst(Slot const &lhs, int val) {
  pushPtr();
  moveTo(lhs);

  if (val == 0) {
    setToValue(1);
    moveTo(lhs, MacroCell::Value1);
    zeroCell();
    popPtr();
    return;
  }

  if (lhs.type->usesValue1()) {
    not16Destructive(Cell{lhs, MacroCell::Value1},
		     Temps<1>::select(lhs, MacroCell::Scratch0));
  } else {
    notDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
  }
  
  popPtr();
}

void Assembler::nandSlotWithSlot(Slot const &lhs, Slot const &rhs) {
  pushPtr();

  Slot const rhsCopy = getTemp(rhs.type);
  assignSlot(rhsCopy, rhs);
  moveTo(lhs);

  if (lhs.type->usesValue1() || rhs.type->usesValue1()) {
    nand16Destructive(Cell{lhs, MacroCell::Value1},
		      Cell{rhsCopy, MacroCell::Value0},
		      Cell{rhsCopy, MacroCell::Value1},
		      Temps<1>::select(lhs, MacroCell::Scratch0));
		     
  } else {
    nandDestructive(Cell{rhsCopy, MacroCell::Value0},
		    Temps<1>::select(lhs, MacroCell::Scratch0));
  }

  popPtr();
  freeTempSlot(rhsCopy);
}


void Assembler::orSlotWithConst(Slot const &lhs, int val) {
  pushPtr();
  moveTo(lhs);

  if (val > 0) {
    setToValue(1);
    moveTo(lhs, MacroCell::Value1);
    zeroCell();
    popPtr();
    return;
  }

  if (lhs.type->usesValue1()) {
    bool16Destructive(Cell{lhs, MacroCell::Value1},
		      Temps<1>::select(lhs, MacroCell::Scratch0));
  } else {
    boolDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
  }
  
  popPtr();
}

void Assembler::orSlotWithSlot(Slot const &lhs, Slot const &rhs) {
  pushPtr();

  Slot const rhsCopy = getTemp(rhs.type);
  assignSlot(rhsCopy, rhs);
  moveTo(lhs);

  if (lhs.type->usesValue1() || rhs.type->usesValue1()) {
    or16Destructive(Cell{lhs, MacroCell::Value1},
		    Cell{rhsCopy, MacroCell::Value0},
		    Cell{rhsCopy, MacroCell::Value1},
		    Temps<1>::select(lhs, MacroCell::Scratch0));
		     
  } else {
    orDestructive(Cell{rhsCopy, MacroCell::Value0},
		  Temps<1>::select(lhs, MacroCell::Scratch0));
  }

  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::norSlotWithConst(Slot const &lhs, int val) {
  pushPtr();
  moveTo(lhs);

  if (val > 0) {
    setToValue(0);
    moveTo(lhs, MacroCell::Value1);
    zeroCell();
    popPtr();
    return;
  }

  if (lhs.type->usesValue1()) {
    not16Destructive(Cell{lhs, MacroCell::Value1},
		      Temps<1>::select(lhs, MacroCell::Scratch0));
  } else {
    notDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
  }
  
  popPtr();
}

void Assembler::norSlotWithSlot(Slot const &lhs, Slot const &rhs) {
  pushPtr();

  Slot const rhsCopy = getTemp(rhs.type);
  assignSlot(rhsCopy, rhs);
  moveTo(lhs);

  if (lhs.type->usesValue1() || rhs.type->usesValue1()) {
    nor16Destructive(Cell{lhs, MacroCell::Value1},
		     Cell{rhsCopy, MacroCell::Value0},
		     Cell{rhsCopy, MacroCell::Value1},
		     Temps<1>::select(lhs, MacroCell::Scratch0));
		     
  } else {
    norDestructive(Cell{rhsCopy, MacroCell::Value0},
		   Temps<1>::select(lhs, MacroCell::Scratch0));
  }

  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::xorSlotWithConst(Slot const &lhs, int val) {
  pushPtr();
  moveTo(lhs);

  if (val > 0) {
    if (lhs.type->usesValue1()) {
      not16Destructive(Cell{lhs, MacroCell::Value1},
		       Temps<1>::select(lhs, MacroCell::Scratch0));
    } else {
      notDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
    }
  }
  else {
    if (lhs.type->usesValue1()) {
      bool16Destructive(Cell{lhs, MacroCell::Value1},
		       Temps<1>::select(lhs, MacroCell::Scratch0));
    } else {
      boolDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
    }
  }

  popPtr();
}

void Assembler::xorSlotWithSlot(Slot const &lhs, Slot const &rhs) {
  pushPtr();

  Slot const rhsCopy = getTemp(rhs.type);
  assignSlot(rhsCopy, rhs);
  moveTo(lhs);

  if (lhs.type->usesValue1() || rhs.type->usesValue1()) {
    xor16Destructive(Cell{lhs, MacroCell::Value1},
		     Cell{rhsCopy, MacroCell::Value0},
		     Cell{rhsCopy, MacroCell::Value1},
		     Temps<2>::select(lhs, MacroCell::Scratch0,
				      lhs, MacroCell::Scratch1));
		     
  } else {
    xorDestructive(Cell{rhsCopy, MacroCell::Value0},
		   Temps<2>::select(lhs, MacroCell::Scratch0,
				    lhs, MacroCell::Scratch1));
  }

  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::xnorSlotWithConst(Slot const &lhs, int val) {
  pushPtr();
  moveTo(lhs);

  if (val == 0) {
    if (lhs.type->usesValue1()) {
      not16Destructive(Cell{lhs, MacroCell::Value1},
		       Temps<1>::select(lhs, MacroCell::Scratch0));
    } else {
      notDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
    }
  }
  else {
    if (lhs.type->usesValue1()) {
      bool16Destructive(Cell{lhs, MacroCell::Value1},
		       Temps<1>::select(lhs, MacroCell::Scratch0));
    } else {
      boolDestructive(Temps<1>::select(lhs, MacroCell::Scratch0));
    }
  }

  popPtr();
}

void Assembler::xnorSlotWithSlot(Slot const &lhs, Slot const &rhs) {
  pushPtr();

  Slot const rhsCopy = getTemp(rhs.type);
  assignSlot(rhsCopy, rhs);
  moveTo(lhs);

  if (lhs.type->usesValue1() || rhs.type->usesValue1()) {
    xnor16Destructive(Cell{lhs, MacroCell::Value1},
		      Cell{rhsCopy, MacroCell::Value0},
		      Cell{rhsCopy, MacroCell::Value1},
		      Temps<2>::select(lhs, MacroCell::Scratch0,
				       lhs, MacroCell::Scratch1));
		     
  } else {
    xnorDestructive(Cell{rhsCopy, MacroCell::Value0},
		    Temps<2>::select(lhs, MacroCell::Scratch0,
				     lhs, MacroCell::Scratch1));
  }

  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::orDestructive(Cell other, Temps<1> tmp) {
  auto [cur, oth, tmp0] = getFieldIndices(_dp.current(), other, tmp.get<0>());
  emit<primitive::Or>(cur, oth, tmp0);
}

void Assembler::orConstructive(Cell result, Cell other, Temps<2> tmp) {
  Cell const &otherCopy = tmp.get<0>();
  
  pushPtr();
  copyField(result, tmp.select<1>());  
  moveTo(other);
  copyField(otherCopy, tmp.select<1>());
  moveTo(result);
  orDestructive(otherCopy, tmp.select<1>());
  popPtr();
}

void Assembler::or16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<1> tmp) {

  pushPtr();
  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;

  // Collapse both to bool
  moveTo(currentLow);
  bool16Destructive(currentHigh, tmp);
  moveTo(otherLow);
  bool16Destructive(otherHigh, tmp);

  // And results
  moveTo(currentLow);
  orDestructive(otherLow, tmp);
  popPtr();
}

void Assembler::or16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<4> tmp) { 

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
  or16Destructive(resultHigh, otherCopyLow, otherCopyHigh, tmp.select<3>());
  popPtr();
}

void Assembler::andDestructive(Cell other, Temps<1> tmp) {
  auto [cur, oth, tmp0] = getFieldIndices(_dp.current(), other, tmp.get<0>());
  emit<primitive::And>(cur, oth, tmp0);
}

void Assembler::andConstructive(Cell result, Cell other, Temps<2> tmp) { 
  Cell const &otherCopy = tmp.get<0>();
  pushPtr();
  copyField(result, tmp.select<1>());  
  moveTo(other);
  copyField(otherCopy, tmp.select<1>());
  moveTo(result);
  andDestructive(otherCopy, tmp.select<1>());
  popPtr();
}

void Assembler::and16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<1> tmp) {

  pushPtr();
  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;

  // Collapse both to bool
  moveTo(currentLow);
  bool16Destructive(currentHigh, tmp);
  moveTo(otherLow);
  bool16Destructive(otherHigh, tmp);

  // And results
  moveTo(currentLow);
  andDestructive(otherLow, tmp);
  popPtr();
}


void Assembler::and16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<4> tmp) { 

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
  and16Destructive(resultHigh, otherCopyLow, otherCopyHigh, tmp.select<3>());
  popPtr();
}

void Assembler::xorDestructive(Cell other, Temps<2> tmp) {
  auto [cur, oth, tmp0, tmp1] = getFieldIndices(_dp.current(), other, tmp.get<0>(), tmp.get<1>());
  emit<primitive::Xor>(cur, oth, tmp0, tmp1);
}

void Assembler::xorConstructive(Cell result, Cell other, Temps<3> tmp) { 
  Cell const &otherCopy = tmp.get<0>();
  pushPtr();
  copyField(result, tmp.select<1>());  
  moveTo(other);
  copyField(otherCopy, tmp.select<1>());
  moveTo(result);
  xorDestructive(otherCopy, tmp.select<1, 2>());
  popPtr();
}

void Assembler::xor16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<2> tmp) {
  pushPtr();
  Cell const currentLow = _dp.current();
  Cell const currentHigh = high;

  // Collapse both to bool
  moveTo(currentLow);
  bool16Destructive(currentHigh, tmp.select<0>());
  moveTo(otherLow);
  bool16Destructive(otherHigh, tmp.select<0>());

  // And results
  moveTo(currentLow);
  xorDestructive(otherLow, tmp);
  popPtr();
}


void Assembler::xor16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<5> tmp) { 

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
  xor16Destructive(resultHigh, otherCopyLow, otherCopyHigh, tmp.select<3, 4>());
  popPtr();
}

void Assembler::nandDestructive(Cell other, Temps<1> tmp) {
  andDestructive(other, tmp);
  notDestructive(tmp);
}

void Assembler::nandConstructive(Cell result, Cell other, Temps<2> tmp) {
  andConstructive(result, other, tmp);
  notDestructive(tmp.select<0>());
}

void Assembler::nand16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<1> tmp) {
  and16Destructive(high, otherLow, otherHigh, tmp);
  notDestructive(tmp);
}


void Assembler::nand16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<4> tmp) {
  and16Constructive(high, result, otherLow, otherHigh, tmp);
  notDestructive(tmp.select<0>());
}

void Assembler::norDestructive(Cell other, Temps<1> tmp) {
  orDestructive(other, tmp);
  notDestructive(tmp);
}

void Assembler::norConstructive(Cell result, Cell other, Temps<2> tmp) {
  orConstructive(result, other, tmp);
  notDestructive(tmp.select<0>());
}

void Assembler::nor16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<1> tmp) {
  or16Destructive(high, otherLow, otherHigh, tmp);
  notDestructive(tmp);
}

void Assembler::nor16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<4> tmp) {
  or16Constructive(high, result, otherLow, otherHigh, tmp);
  notDestructive(tmp.select<0>());
}

void Assembler::xnorDestructive(Cell other, Temps<2> tmp) {
  xorDestructive(other, tmp);
  notDestructive(tmp.select<1>());
}

void Assembler::xnorConstructive(Cell result, Cell other, Temps<3> tmp) {
  xorConstructive(result, other, tmp);
  notDestructive(tmp.select<0>());
}

void Assembler::xnor16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<2> tmp) {
  xor16Destructive(high, otherLow, otherHigh, tmp);
  notDestructive(tmp.select<0>());
}

void Assembler::xnor16Constructive(Cell high, Cell result, Cell otherLow, Cell otherHigh, Temps<5> tmp) {
  xor16Constructive(high, result, otherLow, otherHigh, tmp);
  notDestructive(tmp.select<0>());
}
