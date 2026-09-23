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

Assembler::SingleCell Assembler::addConst(SingleCell const &target, int delta) {
  pushPtr();
  moveTo(target);
  emit<primitive::ChangeBy>(delta);
  popPtr();
  return target;
}

Assembler::Add16Operand Assembler::add16Const(Add16Operand const &lhs, int delta) {
  if (delta == 0) return lhs;
  if (delta < 0) {
    return sub16Const(lhs, -delta);
  }

  Slot const tmpSlot = getTemp(ts::raw(1));
  auto const tmp = ws::promiseClean16(tmpSlot);
  setToValue16(tmp, delta);
  add16Destructive(lhs, tmp);
  freeTempSlot(tmpSlot);
  return lhs;
}

Assembler::SingleCell Assembler::addDestructive(SingleCell const &op, SingleCell const &delta) {
  auto [cur, oth] = getFieldIndices(op, delta);

  pushPtr();
  moveTo(op);
  emit<primitive::Add>(cur, oth);
  popPtr();
  
  return op;
}

Assembler::Add16Operand Assembler::add16Destructive(Add16Operand const &op, DoubleCell const &delta) {
  // Add low byte
  loop(delta[0], [&]{
    dec();
    inc16(op);
  });

  // Add high byte
  addDestructive(op[1], delta[1]);
  return op;
}

