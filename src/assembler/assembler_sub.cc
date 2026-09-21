// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::subConstFromSlot(Slot lhs, int delta) {
  Slot const tmp = getTemp(ts::raw(1));
  
  pushPtr();
  moveTo(lhs, MacroCell::Value0);    
  if (lhs.type()->usesValue1()) {
    sub16Const(ws::promiseClean16(lhs),
	       ws::promiseClean16(tmp),
	       delta);
  } else {
    subConst(delta);
  }
  popPtr();

  freeSlot(tmp);
}

// TODO: do these need destroyRhs?
void Assembler::subSlotFromSlot(Slot lhs, Slot rhs) {
  pushPtr();
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  moveTo(lhs, MacroCell::Value0);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    sub16Destructive(ws::promiseClean16(lhs),
		     ws::promiseClean16(rhsCopy));
  } else {
    subDestructive(ws::promiseClean8(lhs),
		   ws::promiseClean8(rhsCopy));
  }
  popPtr();
  freeTempSlot(rhsCopy);
}

Assembler::SingleCell Assembler::subConst(int delta) {
  return subConst(_dp.current(), delta);
}

Assembler::SingleCell Assembler::subConst(SingleCell const &target, int delta) {
  pushPtr();
  moveTo(target);
  emit<primitive::ChangeBy>(-delta);
  popPtr();
  return target;
}

Assembler::Add16Operand Assembler::sub16Const(Add16Operand const &lhs, Add16Operand const &tmp, int delta) {
  
  if (delta == 0) return lhs;
  if (delta < 0) {
    return add16Const(lhs, tmp, -delta);
  }  

  setToValue16(tmp, delta);
  sub16Destructive(lhs, tmp);
  return lhs;
}

Assembler::SingleCell Assembler::subDestructive(SingleCell const &op, SingleCell const &delta) {
  auto [cur, oth] = getFieldIndices(op, delta);

  pushPtr();
  moveTo(op);
  emit<primitive::Subtract>(cur, oth);
  popPtr();
  
  return op;
}

Assembler::Add16Operand Assembler::sub16Destructive(Add16Operand const &op, DoubleCell const &delta) {
  // Subtract low byte
  loop(delta[0], [&]{
    dec(delta[0]);
    dec16(op);
  });
  
  // Subtract high byte
  subDestructive(op[1], delta[1]);
  return op;
}


