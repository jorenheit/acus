// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::subConstFromSlot(Slot lhs, int delta) {
  Slot const tmp = getTemp(ts::raw(1));
  
  pushPtr();
  moveTo(lhs, MacroCell::Value0);    
  (lhs.type()->usesValue1())
    ? sub16Const(delta, Cell{tmp, MacroCell::Value0})
    : subConst(delta);
  popPtr();

  freeSlot(tmp);
}

void Assembler::subSlotFromSlot(Slot lhs, Slot rhs) {
  pushPtr();
  Slot rhsCopy = getTemp(rhs.type());
  assignSlot(rhsCopy, rhs);
  moveTo(lhs, MacroCell::Value0);
  if (lhs.type()->usesValue1() || rhs.type()->usesValue1()) {
    sub16Destructive(Cell{rhsCopy, MacroCell::Value0});
  } else {
    subDestructive(Cell{rhsCopy, MacroCell::Value0});
  }
  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::subConst(int delta) {
  addConst(-delta);
}

void Assembler::sub16Const(int delta, Cell tmp) {
  // Assumes the pointer is currently pointing to the low byte with the high
  // byte right next to it, followed by at least 3 empty scratch cells. The same
  // holds for the tmp cell. It should be a Value0 cell where we can utilize its
  // entire macrocell.
  assert(tmp.field == MacroCell::Value0);
  assert(_dp.current().field == MacroCell::Value0);
  
  if (delta == 0) return;
  if (delta < 0) {
    add16Const(-delta, tmp);
    return;
  }  

  pushPtr();
  Cell const operand = _dp.current();
    
  moveTo(tmp, MacroCell::Value0);
  setToValue16(delta);
  moveTo(operand);
  sub16Destructive(tmp);
  popPtr();
}

void Assembler::subDestructive(Cell other) {
  auto [cur, oth] = getFieldIndices(_dp.current(), other);
  emit<primitive::Subtract>(cur, oth);
}

void Assembler::sub16Destructive(Cell delta) {
  // This algorithm assumes that the value currently pointed to is the low byte,
  // with the high byte right next to it, followed by at least 3 empty scratch cells.
  // The same constraint holds for the delta-cell. The delta is destroyed.
  assert(_dp.current().field == MacroCell::Value0);
  assert(delta.field == MacroCell::Value0);
  
  Cell const operand = _dp.current();
  pushPtr();

  // Subtract low byte
  moveTo(delta);
  loopOpen(); {
    dec();
    moveTo(operand);
    dec16();
    moveTo(delta);
  } loopClose();

  // Subtract high byte
  moveTo(operand, MacroCell::Value1);
  subDestructive(Cell{delta, MacroCell::Value1});
  
  popPtr();
}

