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
  Slot const tmp = getTemp(ts::raw(1));
  
  pushPtr();
  moveTo(lhs, MacroCell::Value0); // TODO : remove when addConst takes a ws
  if (lhs.type()->usesValue1()) {
    add16Const(ws::promiseClean16(lhs),
	       ws::promiseClean16(tmp),
	       delta);
  } else {
    addConst(ws::promiseClean8(lhs), delta);
  }
  
  popPtr();

  freeSlot(tmp);
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

Assembler::Add16Operand Assembler::add16Const(Add16Operand const &lhs, Add16Operand const &tmp, int delta) {
  if (delta == 0) return lhs;
  if (delta < 0) {
    return sub16Const(lhs, tmp, -delta);
  }

  setToValue16(tmp, delta);
  add16Destructive(lhs, tmp);
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
  // This algorithm assumes that the value currently pointed to is the low byte,
  // with the high byte right next to it, followed by at least 3 empty scratch cells.
  // The same constraint holds for the delta-cell. The delta is destroyed.
  
  pushPtr();

  // Add low byte
  moveTo(delta[0]);
  loopOpen(); {
    dec();
    inc16(op);
    moveTo(delta[0]);
  } loopClose();

  // Add high byte
  addDestructive(op[1], delta[1]);
  
  popPtr();
  return op;
}

