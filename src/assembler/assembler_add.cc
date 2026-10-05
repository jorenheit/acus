// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::addSlotToSlot(Slot lhs, Slot rhs, bool destroyRhs) {
  destroyRhs = destroyRhs && lhs != rhs;
  pushPtr();
  Slot rhsCopy = destroyRhs ? rhs : getTemp(rhs.type());
  if (!destroyRhs) assignSlot(rhsCopy, rhs);


  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    add16Destructive(ws::promiseClean16(lhs),
                     ws::promiseClean16(rhsCopy));
  } else {
    addDestructive(ws::promiseClean8(lhs),
                   ws::promiseClean8(rhsCopy));
  }
  popPtr();
  if (!destroyRhs) freeTempSlot(rhsCopy);
}

void Assembler::addConstToSlot(Slot lhs, int delta) {
  if (lhs.type()->usesValue1()) {
    add16Const(ws::promiseClean16(lhs), delta);
  } else {
    addConst(ws::promiseClean8(lhs), delta);
  }
}

Assembler::SingleCell Assembler::addConst(Cell lhs, Cell tmp, int delta) {
  int const current = getFieldIndex(lhs);
  int const scratch = getFieldIndex(tmp);
  
  pushPtr();
  moveTo(lhs);
  emit<primitive::ChangeBy>(delta, current, scratch);
  popPtr();
  return lhs;
}

Assembler::SingleCell Assembler::addConst(Cell lhs, int delta) {
  pushPtr();
  moveTo(lhs);
  emit<primitive::ChangeBy>(delta);
  popPtr();
  return lhs;
}

Assembler::SingleCell Assembler::addConst(int delta) {
  return addConst(_dp.current(), delta);
}




