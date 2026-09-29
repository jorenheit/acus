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

void Assembler::andSlotWithSlot(Slot lhs, Slot rhs) {
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);

  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    and16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
		     
  } else {
    andDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  freeTempSlot(rhsCopy);
}


void Assembler::nandSlotWithConst(Slot lhs, int val) {
  if (val == 0) return setSlotToValue(lhs, 1);
  if (lhs.type()->usesValue1()) {
    not16Destructive(ws::promiseClean16(lhs));
  } else {
    notDestructive(ws::promiseClean8(lhs));
  }
}

void Assembler::nandSlotWithSlot(Slot lhs, Slot rhs) {
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    nand16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
		     
  } else {
    nandDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  freeTempSlot(rhsCopy);
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

void Assembler::orSlotWithSlot(Slot lhs, Slot rhs) {
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    or16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
  } else {
    orDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  freeTempSlot(rhsCopy);
}

void Assembler::norSlotWithConst(Slot lhs, int val) {
  if (val != 0) return setSlotToValue(lhs, 0);
  if (lhs.type()->usesValue1()) {
    not16Destructive(ws::promiseClean16(lhs));
  } else {
    notDestructive(ws::promiseClean8(lhs));
  }
}

void Assembler::norSlotWithSlot(Slot lhs, Slot rhs) {
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    nor16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));		     
  } else {
    norDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  freeTempSlot(rhsCopy);
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

void Assembler::xorSlotWithSlot(Slot lhs, Slot rhs) {
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    xor16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));		     
  } else {
    xorDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  freeTempSlot(rhsCopy);
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

void Assembler::xnorSlotWithSlot(Slot lhs, Slot rhs) {
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    xnor16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
  } else {
    xnorDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  freeTempSlot(rhsCopy);
}
