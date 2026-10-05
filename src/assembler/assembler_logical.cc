// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::andSlotWithConst(Slot lhs, int val) {

  if (val == 0) {
    zeroCell(Cell{lhs, MacroCell::Value0});
    zeroCell(Cell{lhs, MacroCell::Value1});
    return;
  }

  if (lhs.type()->usesValue1()) {
    bool16Destructive(ws::promiseClean16(lhs));
  } else {
    boolDestructive(ws::promiseClean8(lhs));
  }
}

void Assembler::andSlotWithSlot(Slot lhs, Slot rhs, bool consumeRhs) {
  consumeRhs = consumeRhs && lhs != rhs;
  Slot rhsCopy = consumeRhs ? rhs : getTemp(rhs.type());
  if (!consumeRhs) assignSlot(rhsCopy, rhs);

  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    and16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
		     
  } else {
    andDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  if (!consumeRhs) freeTempSlot(rhsCopy);
}


void Assembler::nandSlotWithConst(Slot lhs, int val) {
  if (val == 0) return setSlotToValue(lhs, 1);
  if (lhs.type()->usesValue1()) {
    not16Destructive(ws::promiseClean16(lhs));
  } else {
    notDestructive(ws::promiseClean8(lhs));
  }
}

void Assembler::nandSlotWithSlot(Slot lhs, Slot rhs, bool consumeRhs) {
  consumeRhs = consumeRhs && lhs != rhs;
  Slot rhsCopy = consumeRhs ? rhs : getTemp(rhs.type());
  if (!consumeRhs) assignSlot(rhsCopy, rhs);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    nand16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
		     
  } else {
    nandDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  if (!consumeRhs) freeTempSlot(rhsCopy);
}


void Assembler::orSlotWithConst(Slot lhs, int val) {
  if (val != 0) return setSlotToValue(lhs, 1);

  if (lhs.type()->usesValue1()) {
    bool16Destructive(ws::promiseClean16(lhs));
  } else {
    zeroCell(Cell{lhs, MacroCell::Value1});
    boolDestructive(ws::promise(lhs, ws::Layout<ws::Data<>, ws::Scratch>{}));
  }
}

void Assembler::orSlotWithSlot(Slot lhs, Slot rhs, bool consumeRhs) {
  consumeRhs = consumeRhs && lhs != rhs;
  Slot rhsCopy = consumeRhs ? rhs : getTemp(rhs.type());
  if (!consumeRhs) assignSlot(rhsCopy, rhs);
  
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    or16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
  } else {
    orDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  if (!consumeRhs) freeTempSlot(rhsCopy);
}

void Assembler::norSlotWithConst(Slot lhs, int val) {
  if (val != 0) return setSlotToValue(lhs, 0);
  if (lhs.type()->usesValue1()) {
    not16Destructive(ws::promiseClean16(lhs));
  } else {
    notDestructive(ws::promiseClean8(lhs));
  }
}

void Assembler::norSlotWithSlot(Slot lhs, Slot rhs, bool consumeRhs) {
  consumeRhs = consumeRhs && lhs != rhs;
  Slot rhsCopy = consumeRhs ? rhs : getTemp(rhs.type());
  if (!consumeRhs) assignSlot(rhsCopy, rhs);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    nor16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));		     
  } else {
    norDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  if (!consumeRhs) freeTempSlot(rhsCopy);
}

void Assembler::xorSlotWithConst(Slot lhs, int val) {
  if (val != 0) {
    if (lhs.type()->usesValue1()) {
      not16Destructive(ws::promiseClean16(lhs));
    } else {
      notDestructive(ws::promiseClean8(lhs));
    }
  }
  else {
    if (lhs.type()->usesValue1()) {
      bool16Destructive(ws::promiseClean16(lhs));
    } else {
      boolDestructive(ws::promiseClean8(lhs));
    }
  }
}

void Assembler::xorSlotWithSlot(Slot lhs, Slot rhs, bool consumeRhs) {
  consumeRhs = consumeRhs && lhs != rhs;
  Slot rhsCopy = consumeRhs ? rhs : getTemp(rhs.type());
  if (!consumeRhs) assignSlot(rhsCopy, rhs);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    xor16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));		     
  } else {
    xorDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  if (!consumeRhs) freeTempSlot(rhsCopy);
}


void Assembler::xnorSlotWithConst(Slot lhs, int val) {
  if (val == 0) {
    if (lhs.type()->usesValue1()) {
      not16Destructive(ws::promiseClean16(lhs));
    } else {
      notDestructive(ws::promiseClean8(lhs));
    }
  }
  else {
    if (lhs.type()->usesValue1()) {
      bool16Destructive(ws::promiseClean16(lhs));
    } else {
      boolDestructive(ws::promiseClean8(lhs));
    }
  }
}

void Assembler::xnorSlotWithSlot(Slot lhs, Slot rhs, bool consumeRhs) {
  consumeRhs = consumeRhs && lhs != rhs;
  Slot rhsCopy = consumeRhs ? rhs : getTemp(rhs.type());
  if (!consumeRhs) assignSlot(rhsCopy, rhs);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    xnor16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
  } else {
    xnorDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  if (!consumeRhs) freeTempSlot(rhsCopy);
}

// Cell level stuff

void Assembler::boolDestructive(Cell target, Cell tmp) {
  loop(target, [&]{
    zeroCell(target);
    inc(tmp);
  });
  moveFieldToZero(tmp, target);
}
  
void Assembler::notDestructive(Cell x, Cell tmp) {
  inc(tmp);
  loop(x, [&]{
    dec(tmp);
    zeroCell(x);
  });
  // x = 0, tmp = not(x)
  moveFieldToZero(tmp, x);
}

void Assembler::orDestructive(Cell lhs, Cell rhs) {
  loop(lhs, [&]{
    zeroCell(lhs);
    setToValue(rhs, 1);
  });
  
  loop(rhs, [&]{
    zeroCell(rhs);
    inc(lhs);
  });
}

void Assembler::orDestructive(Cell lhs, Cell rhs, Cell) {
  orDestructive(lhs, rhs);
}

void Assembler::andDestructive(Cell x, Cell y, Cell tmp) {
  addConst(tmp, 2);  // tmp is scratch -> known 0
  loop(x, [&]{
    zeroCell(x);
    dec(tmp);
  });
  inc(x);

  loop(y, [&]{
    zeroCell(y);
    dec(tmp);
  });

  loop(tmp, [&]{
    zeroCell(tmp);
    dec(x);
  });
}

void Assembler::xorDestructive(Cell x, Cell y, Cell tmp) {
  loop(x, [&]{
    zeroCell(x);
    dec(tmp);
  });

  loop(y, [&]{
    zeroCell(y);
    inc(tmp);
  });

  loop(tmp, [&]{
    inc(tmp); // in case tmp == 255
    zeroCell(tmp);
    inc(x);
  });
}

void Assembler::nandDestructive(Cell x, Cell y, Cell tmp) {
  addConst(tmp, 2); // tmp is scratch -> guaranteed 0
  loop(x, [&]{
    zeroCell(x);
    dec(tmp);
  });

  loop(y, [&]{
    zeroCell(y);
    dec(tmp);
  });

  loop(tmp, [&]{
    zeroCell(tmp);
    inc(x);
  });
}

void Assembler::norDestructive(Cell x, Cell y, Cell) {
  loop(x, [&]{
    zeroCell(x);
    setToValue(y, 1);
  });
  inc(x);

  loop(y, [&]{
    zeroCell(y);
    dec(x);
  });
}

void Assembler::xnorDestructive(Cell x, Cell y, Cell tmp) {
  // tmp may overlap with y
  xorDestructive(x, y, tmp);
  notDestructive(x, y); // use whichever is closest to x (y or tmp)
}
