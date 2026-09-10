// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::addSlotToSlot(Slot lhs, Slot rhs) {
  pushPtr();
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  moveTo(lhs, MacroCell::Value0);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    add16Destructive(Cell{rhsCopy, MacroCell::Value0});
  } else {
    addDestructive(Cell{rhsCopy, MacroCell::Value0});
  }
  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::addConstToSlot(Slot lhs, int delta) {
  Slot const tmp = getTemp(ts::raw(1));
  
  pushPtr();
  moveTo(lhs, MacroCell::Value0);    
  (lhs.type()->usesValue1())
    ? add16Const(delta, Cell{tmp, MacroCell::Value0})
    : addConst(delta);
  popPtr();

  freeSlot(tmp);
}

void Assembler::addConst(int delta) {
  emit<primitive::ChangeBy>(delta);
}

void Assembler::addConstAndCarry(int delta, Cell carry, Temps<2> tmp) {
  pushPtr();

  if (delta == 0) {
    moveTo(carry);
    zeroCell();
    popPtr();
    return;
  }

  Cell const resultCopy = tmp.get<0>();      
  copyField(carry, tmp.select<1>());
  addConst(delta);
  copyField(resultCopy, tmp.select<1>());

  moveTo(carry);
  if (delta > 0) {
    greaterDestructive(resultCopy, tmp.select<1>());
  } else {
    lessDestructive(resultCopy, tmp.select<1>());
  }

  popPtr();
}  

void Assembler::add16Const(int delta, Cell tmp) {
  // Assumes the pointer is currently pointing to the low byte with the high
  // byte right next to it, followed by at least 3 empty scratch cells. The same
  // holds for the tmp cell. It should be a Value0 cell where we can utilize its
  // entire macrocell.
  assert(_dp.current().field == MacroCell::Value0);
  assert(tmp.field == MacroCell::Value0);
  
  if (delta == 0) return;
  if (delta < 0) {
    sub16Const(-delta, tmp);
    return;
  }

  pushPtr();
  Cell const operand = _dp.current();
    
  moveTo(tmp, MacroCell::Value0);
  setToValue16(delta);
  moveTo(operand);
  add16Destructive(tmp);
  popPtr();
}
void Assembler::addDestructive(Cell delta) {
  auto [cur, oth] = getFieldIndices(_dp.current(), delta);
  emit<primitive::Add>(cur, oth);
}

void Assembler::add16Destructive(Cell delta) {
  // This algorithm assumes that the value currently pointed to is the low byte,
  // with the high byte right next to it, followed by at least 3 empty scratch cells.
  // The same constraint holds for the delta-cell. The delta is destroyed.
  assert(_dp.current().field == MacroCell::Value0);
  assert(delta.field == MacroCell::Value0);
  
  Cell const operand = _dp.current();
  pushPtr();

  // Add low byte
  moveTo(delta);
  loopOpen(); {
    dec();
    moveTo(operand);
    inc16();
    moveTo(delta);
  } loopClose();

  // Add high byte
  moveTo(operand, MacroCell::Value1);
  addDestructive(Cell{delta, MacroCell::Value1});
  
  popPtr();
}

