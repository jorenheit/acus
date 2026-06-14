// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::addSlotToSlot(Slot const &lhs, Slot const &rhs) {
  pushPtr();
  Slot const rhsCopy = getTemp(rhs.type);
  assignSlot(rhsCopy, rhs);
  moveTo(lhs, MacroCell::Value0);
  if (lhs.type->usesValue1() || rhs.type->usesValue1()) {
    add16Destructive(Cell{lhs, MacroCell::Value1},
		     Cell{rhsCopy, MacroCell::Value0},
		     Cell{rhsCopy, MacroCell::Value1},
		     Temps<4>::select(lhs, MacroCell::Scratch0,
				      lhs, MacroCell::Scratch1,
				      rhsCopy, MacroCell::Scratch0,
				      rhsCopy, MacroCell::Scratch1));
  } else {
    addDestructive(Cell{rhsCopy, MacroCell::Value0});
  }
  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::addConstToSlot(Slot const &lhs, int delta) {
  pushPtr();
  moveTo(lhs, MacroCell::Value0);    
  (lhs.type->usesValue1())
    ? add16Const(delta, Cell{lhs, MacroCell::Value1},
		 Temps<4>::select(lhs, MacroCell::Scratch0,
				  lhs, MacroCell::Scratch1,
				  lhs, MacroCell::Payload0,
				  lhs, MacroCell::Payload1))
    : addConst(delta);
  popPtr();
}


void Assembler::addConst(int delta) {
  emit<primitive::ChangeBy>(delta);
}

void Assembler::addConstAndCarry(int delta, Cell carry, Temps<3> tmp) {
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
    greaterDestructive(resultCopy, tmp.select<1, 2>());
  } else {
    lessDestructive(resultCopy, tmp.select<1, 2>());
  }

  popPtr();
}  

void Assembler::addAndCarryDestructive(Cell carry, Cell other, Temps<3> tmp) {
  Cell resultCopy = tmp.get<0>();

  pushPtr();
  copyField(carry, tmp.get<1>());  
  addDestructive(other);
  copyField(resultCopy, tmp.get<1>());
  moveTo(carry); // contains old value
  greaterDestructive(resultCopy, tmp.select<1, 2>());
  popPtr();
}

void Assembler::addAndCarryConstructive(Cell result, Cell carry, Cell other, Temps<4> tmp) {
  pushPtr();
  copyField(result, tmp.get<0>());
  moveTo(other);
  copyField(tmp.get<0>(), tmp.get<1>());
  moveTo(result);
  addAndCarryDestructive(carry, tmp.get<0>(), tmp.select<1, 2, 3>()); 
  popPtr();
}

void Assembler::add16Const(int delta, Cell high, Temps<4> tmp) {
  if (delta == 0) return;
  if (delta < 0) {
    sub16Const(-delta, high, tmp);
    return;
  }
  
  int const lowDelta  = delta & 0xff;
  int const highDelta = (delta >> 8) & 0xff;
  Cell const carry = tmp.get<0>();

  pushPtr();
  if (lowDelta != 0)  addConstAndCarry(lowDelta, carry, tmp.select<1, 2, 3>());
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

void Assembler::add16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<4> tmp) {

  pushPtr();
  Cell const &low   = _dp.current();
  Cell const &carry = tmp.get<0>();
  
  // add low bytes and get the carry
  moveTo(low);
  addAndCarryDestructive(carry, otherLow, tmp.select<1, 2, 3>());

  // if carry -> increment high byte
  moveTo(carry);
  loopOpen(); {
    zeroCell(); // reset carry
    moveTo(high);
    addConst(1);
    moveTo(carry);
  } loopClose();
  
  // add high bytes, ignore carry
  moveTo(high);  
  addDestructive(otherHigh);

  popPtr();
}

void Assembler::add16Constructive(Cell high, Cell resultLow, Cell resultHigh, Cell otherLow, Cell otherHigh, Temps<6> tmp) {

  Cell const & low      = _dp.current();
  Cell const & otherLowCopy  = tmp.get<0>();
  Cell const & otherHighCopy = tmp.get<1>();
  
  pushPtr();
  moveTo(low);  copyField(resultLow, tmp.select<2>());
  moveTo(high); copyField(resultHigh, tmp.select<2>());
  moveTo(otherLow);  copyField(otherLowCopy, tmp.select<2>());
  moveTo(otherHigh); copyField(otherHighCopy, tmp.select<2>());

  moveTo(resultLow);
  add16Destructive(resultHigh, otherLowCopy, otherHighCopy, tmp.select<2, 3, 4, 5>());
  popPtr();
}

