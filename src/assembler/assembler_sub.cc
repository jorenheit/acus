// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

// void Assembler::subConstFromSlot(Slot lhs, int delta) {
//   pushPtr();
//   moveTo(lhs, MacroCell::Value0);    
//   (lhs.type()->usesValue1())
//     ? sub16Const(delta,
// 		 Cell{lhs, MacroCell::Value1},
// 		 Temps<3>::select(lhs, MacroCell::Scratch0,
// 				  lhs, MacroCell::Scratch1,
// 				  lhs, MacroCell::Payload0))
//     : subConst(delta);
//   popPtr();
// }

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
    sub16Destructive(Cell{lhs, MacroCell::Value1},
		     Cell{rhsCopy, MacroCell::Value0},
		     Cell{rhsCopy, MacroCell::Value1},
		     Temps<3>::select(lhs, MacroCell::Scratch0,
				      lhs, MacroCell::Scratch1,
				      rhsCopy, MacroCell::Scratch0));
  } else {
    subDestructive(Cell{rhsCopy, MacroCell::Value0});
  }
  popPtr();
  freeTempSlot(rhsCopy);
}

void Assembler::subConst(int delta) {
  addConst(-delta);
}

[[deprecated]] void Assembler::subConstAndCarry(int delta, Cell carry, Temps<2> tmp) {
  addConstAndCarry(-delta, carry, tmp);
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
    
  moveTo(tmp);
  setToValue16(delta, Cell{tmp, MacroCell::Value1}, 
	       Temps<1>::select(tmp, MacroCell::Scratch0));

  moveTo(operand);
  sub16Destructive(tmp);
  popPtr();
}

[[deprecated]] void Assembler::sub16Const(int delta, Cell high, Temps<3> tmp) {
  if (delta == 0) return;
  if (delta < 0) {
    add16Const(-delta, high, tmp);
    return;
  }  
  
  int const lowDelta  = delta & 0xff;
  int const highDelta = (delta >> 8) & 0xff;
  Cell const carry = tmp.get<0>();

  pushPtr();
  if (lowDelta != 0)  subConstAndCarry(lowDelta, carry, tmp.select<1, 2>());
  moveTo(high);
  if (highDelta != 0) subConst(highDelta);
  subDestructive(carry);
  popPtr();
}


void Assembler::subDestructive(Cell other) {
  auto [cur, oth] = getFieldIndices(_dp.current(), other);
  emit<primitive::Subtract>(cur, oth);
}

void Assembler::subConstructive(Cell result, Cell other, Temps<2> tmp) {
  pushPtr();
  copyField(result, tmp.get<0>());
  moveTo(other);
  copyField(tmp.get<0>(), tmp.get<1>());
  moveTo(result);
  subDestructive(tmp.get<0>());
  popPtr();
}


void Assembler::subAndCarryDestructive(Cell carry, Cell other, Temps<2> tmp) {
  Cell resultCopy = tmp.get<0>();

  pushPtr();
  copyField(carry, tmp.get<1>());  
  subDestructive(other);
  copyField(resultCopy, tmp.get<1>());
  moveTo(carry); // contains old value
  lessDestructive(resultCopy, tmp.select<1>());
  popPtr();
}

void Assembler::subAndCarryConstructive(Cell result, Cell carry, Cell other, Temps<3> tmp) {
  pushPtr();
  copyField(result, tmp.get<0>());
  moveTo(other);
  copyField(tmp.get<0>(), tmp.get<1>());
  moveTo(result);
  subAndCarryDestructive(carry, tmp.get<0>(), tmp.select<1, 2>()); 
  popPtr();
}

void Assembler::sub16Destructive(Cell delta) {
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
    dec16();
    moveTo(delta);
  } loopClose();

  // Add high byte
  moveTo(operand, MacroCell::Value1);
  subDestructive(Cell{delta, MacroCell::Value1});
  
  popPtr();
}


[[deprecated]] void Assembler::sub16Destructive(Cell high, Cell otherLow, Cell otherHigh, Temps<3> tmp) {

  Cell const &low = _dp.current();
  Cell const &carry = tmp.get<0>();

  pushPtr();

  // subtract low bytes and get the carry
  moveTo(low);
  subAndCarryDestructive(carry, otherLow, tmp.select<1, 2>());

  // if carry -> increment high byte
  moveTo(carry);
  loopOpen(); {
    zeroCell(); // reset carry
    moveTo(high);
    dec();
    moveTo(carry);
  } loopClose();
  
  // subtract high bytes, ignore carry
  moveTo(high);  
  subDestructive(otherHigh);

  popPtr();
}

void Assembler::sub16Constructive(Cell delta, Cell result, Cell tmp) {
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
  sub16Destructive(tmp);
  popPtr();
}


[[deprecated]] void Assembler::sub16Constructive(Cell high, Cell resultLow, Cell resultHigh, Cell otherLow, Cell otherHigh, Temps<5> tmp) {

  Cell const &low = _dp.current();
  Cell const &otherLowCopy  = tmp.get<0>();
  Cell const &otherHighCopy = tmp.get<1>();
  
  pushPtr();
  moveTo(low);  copyField(resultLow, tmp.select<2>());
  moveTo(high); copyField(resultHigh, tmp.select<2>());
  moveTo(otherLow);  copyField(otherLowCopy, tmp.select<2>());
  moveTo(otherHigh); copyField(otherHighCopy, tmp.select<2>());

  moveTo(resultLow);
  sub16Destructive(resultHigh, otherLowCopy, otherHighCopy, tmp.select<2, 3, 4>());
  popPtr();
}
