// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::subConstFromSlot(Slot lhs, int delta) {
  
  if (lhs.type()->usesValue1()) {
    sub16Const(ws::promiseClean16(lhs), delta);
  } else {
    subConst(ws::promiseClean8(lhs), delta);
  }
}

// TODO: do these need destroyRhs? Even if that means API consistency this should be the case.
// check after new API has converged.
void Assembler::subSlotFromSlot(Slot lhs, Slot rhs) {
  pushPtr();
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  moveTo(lhs, MacroCell::Value0);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    sub16Destructive(ws::promiseClean16(lhs), ws::promiseClean16(rhsCopy));
  } else {
    subDestructive(ws::promiseClean8(lhs), ws::promiseClean8(rhsCopy));
  }
  popPtr();
  freeTempSlot(rhsCopy);
}

Assembler::SingleCell Assembler::subConst(int delta) {
  return subConst(_dp.current(), delta);
}







Assembler::SingleCell Assembler::subConst(Cell lhs, int delta) {
  return subConst(SingleCell{lhs}, delta);
}
