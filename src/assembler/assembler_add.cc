// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::addSlotToSlot(Slot lhs, Slot rhs) {
  pushPtr();
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);


  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    add16Destructive(ws::promiseClean16(lhs),
                     ws::promiseClean16(rhsCopy));
  } else {
    addDestructive(ws::promiseClean8(lhs),
                   ws::promiseClean8(rhsCopy));
  }
  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::addConstToSlot(Slot lhs, int delta) {
  if (lhs.type()->usesValue1()) {
    add16Const(ws::promiseClean16(lhs), delta);
  } else {
    addConst(ws::promiseClean8(lhs), delta);
  }
}

Assembler::SingleCell Assembler::addConst(int delta) {
  return addConst(_dp.current(), delta);
}

Assembler::SingleCell Assembler::addConst(Cell lhs, int delta) {
  return addConst(SingleCell{lhs}, delta);
}




