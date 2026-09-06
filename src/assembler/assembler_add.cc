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

void Assembler::addAndCarryDestructive(Cell carry, Cell other, Temps<2> tmp) {
  Cell resultCopy = tmp.get<0>();

  pushPtr();
  copyField(carry, tmp.get<1>());  
  addDestructive(other);
  copyField(resultCopy, tmp.get<1>());
  moveTo(carry); // contains old value
  greaterDestructive(resultCopy, tmp.select<1>());
  popPtr();
}

void Assembler::addAndCarryConstructive(Cell result, Cell carry, Cell other, Temps<3> tmp) {
  pushPtr();
  copyField(result, tmp.get<0>());
  moveTo(other);
  copyField(tmp.get<0>(), tmp.get<1>());
  moveTo(result);
  addAndCarryDestructive(carry, tmp.get<0>(), tmp.select<1, 2>()); 
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
  setToValue16(delta, Cell{tmp, static_cast<MacroCell::Field>(tmp.field + 1)}, 
	       Temps<1>::select(tmp, MacroCell::Scratch0));

  moveTo(operand);
  add16Destructive(tmp);
  popPtr();
}

[[deprecated]] void Assembler::add16Const(int delta, Cell high, Temps<3> tmp) {
  if (delta == 0) return;
  if (delta < 0) {
    sub16Const(-delta, high, tmp);
    return;
  }
  
  int const lowDelta  = delta & 0xff;
  int const highDelta = (delta >> 8) & 0xff;
  Cell const carry = tmp.get<0>();

  pushPtr();
  if (lowDelta != 0)  addConstAndCarry(lowDelta, carry, tmp.select<1, 2>());
  moveTo(high);
  if (highDelta != 0) addConst(highDelta);
  addDestructive(carry);
  popPtr();
}

void Assembler::addDestructive(Cell other) {
  auto [cur, oth] = getFieldIndices(_dp.current(), other);
  emit<primitive::Add>(cur, oth);
}

void Assembler::addConstructive(Cell result, Cell other, Temps<2> tmp) {
  pushPtr();
  copyField(result, tmp.get<0>());
  moveTo(other);
  copyField(tmp.get<0>(), tmp.get<1>());
  moveTo(result);
  addDestructive(tmp.get<0>());
  popPtr();
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

[[deprecated]] void Assembler::add16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<3> tmp) {

  pushPtr();
  Cell const &low   = _dp.current();
  Cell const &carry = tmp.get<0>();
  
  // add low bytes and get the carry
  moveTo(low);
  addAndCarryDestructive(carry, otherLow, tmp.select<1, 2>());

  // if carry -> increment high byte
  moveTo(carry);
  loopOpen(); {
    zeroCell(); // reset carry
    moveTo(high);
    inc();
    moveTo(carry);
  } loopClose();
  
  // add high bytes, ignore carry
  moveTo(high);  
  addDestructive(otherHigh);

  popPtr();
}

void Assembler::add16Constructive(Cell delta, Cell result, Cell tmp) {
  assert(_dp.current().field == MacroCell::Value0);
  assert(result.field == MacroCell::Value0);
  assert(tmp.field == MacroCell::Value0);

  pushPtr();
  // Copy current into result
  copyField(Cell{result, MacroCell::Value0}, Temps<1>::select(tmp, MacroCell::Scratch0));
  switchField(MacroCell::Value1);
  copyField(Cell{result, MacroCell::Value1}, Temps<1>::select(tmp, MacroCell::Scratch0));
  
  // Copy delta into tmp
  moveTo(delta);
  copyField(Cell{tmp, MacroCell::Value0}, Temps<1>::select(tmp, MacroCell::Scratch0));
  moveTo(delta, MacroCell::Value1);
  copyField(Cell{tmp, MacroCell::Value1}, Temps<1>::select(tmp, MacroCell::Scratch0));

  // Perform destructive algorithm
  moveTo(result);
  add16Destructive(tmp);
  popPtr();
}

[[deprecated]] void Assembler::add16Constructive(Cell high, Cell resultLow, Cell resultHigh, Cell otherLow, Cell otherHigh, Temps<5> tmp) {

  Cell const & low      = _dp.current();
  Cell const & otherLowCopy  = tmp.get<0>();
  Cell const & otherHighCopy = tmp.get<1>();
  
  pushPtr();
  moveTo(low);  copyField(resultLow, tmp.select<2>());
  moveTo(high); copyField(resultHigh, tmp.select<2>());
  moveTo(otherLow);  copyField(otherLowCopy, tmp.select<2>());
  moveTo(otherHigh); copyField(otherHighCopy, tmp.select<2>());

  moveTo(resultLow);
  add16Destructive(resultHigh, otherLowCopy, otherHighCopy, tmp.select<2, 3, 4>());
  popPtr();
}

