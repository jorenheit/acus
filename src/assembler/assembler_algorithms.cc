// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::loopOpen(std::string const &tag) {
  emit<primitive::LoopOpen>(tag);
}

void Assembler::loopClose(std::string const &tag) {
  emit<primitive::LoopClose>(tag);
}

void Assembler::switchField(MacroCell::Field field) {
  emit<primitive::MovePointerRelative>(field - _dp.current().field);
  _dp.set(field);
}

void Assembler::moveTo(int offset, MacroCell::Field field) {
  switchField(field);
  moveRel(offset - _dp.current().offset);
}

void Assembler::moveTo(Cell dest) {
  moveTo(dest.offset, dest.field);
}

void Assembler::moveRel(int diff) {
  emit<primitive::MovePointerRelative>(diff * MacroCell::FieldCount);
  _dp.moveRelative(diff);
}

void Assembler::moveToOrigin() {
  moveTo(0);
}

void Assembler::zeroCell() { 
  emit<primitive::ZeroCell>();
}

void Assembler::zeroCellPlus() { 
  emit<primitive::ZeroCellPlus>();
}

void Assembler::setToValue(int value) {
  zeroCell();
  addConst(value & 0xff);
}

void Assembler::setToValue(int value, Temps<1> tmp) {
  auto [cur, scratch] = getFieldIndices(_dp.current(), tmp.get<0>());
  emit<primitive::ConstructConstant>(value, cur, scratch);
}

void Assembler::setToValue16(int value, Cell high) { 
  pushPtr();
  setToValue(value & 0xff);
  moveTo(high);
  setToValue((value >> 8) & 0xff);
  popPtr();
}

void Assembler::setToValue16(int value, Cell high, Temps<1> tmp) { 
  pushPtr();
  setToValue(value & 0xff, tmp);
  moveTo(high);
  setToValue((value >> 8) & 0xff, tmp);
  popPtr();
}

void Assembler::inc() {
  addConst(1);
}

void Assembler::dec() {
  subConst(1);
}

void Assembler::inc16(Cell high, Temps<2> tmp) {
  pushPtr();
  Cell const carry = tmp.get<0>();
  inc();
  notConstructive(carry, tmp.select<1>());
  moveTo(high);
  addDestructive(carry);
  popPtr();
}

void Assembler::dec16(Cell high, Temps<2> tmp) {
  pushPtr();
  Cell const borrow = tmp.get<0>();
  copyField(borrow, tmp.select<1>());
  dec();
  moveTo(borrow);
  notDestructive(tmp.select<1>());
  moveTo(high);
  subDestructive(borrow);
  popPtr();
}

void Assembler::moveField(Cell dest) {
  auto [src, dst] = getFieldIndices(_dp.current(), dest);
  if (src == dst) return;
  emit<primitive::MoveData>(src, dst);
}

void Assembler::copyField(Cell dest, Temps<1> tmp) {
  auto [src, dst, tmp0] = getFieldIndices(_dp.current(), dest, tmp.get<0>());
  emit<primitive::CopyData>(src, dst, tmp0);
}

void Assembler::copyOrMoveField(TransferMode mode, Cell dest, Temps<1> tmp) {
  if (mode == TransferMode::Move) moveField(dest);
  else copyField(dest, tmp);
}

void Assembler::compareToConstDestructive(int value, Temps<1> tmp) {
  auto [cur, tmp0] = getFieldIndices(_dp.current(), tmp.get<0>());
  emit<primitive::Cmp>(value, cur, tmp0);
}

void Assembler::compareToConstConstructive(int value, Cell result, Temps<1> tmp) {
  pushPtr();
  copyField(result, tmp);
  moveTo(result);
  compareToConstDestructive(value, tmp);
  popPtr();
}
    
void Assembler::compare16ToConstDestructive(int value, Cell high, Temps<1> tmp) {
  pushPtr();
  compareToConstDestructive(value & 0xff, tmp);
  moveTo(high);
  compareToConstDestructive((value >> 8) & 0xff, tmp);
  popPtr();

  andDestructive(high, tmp);
}

void Assembler::compare16ToConstConstructive(int value, Cell high, Cell result, Temps<2> tmp) {
  pushPtr();
  copyField(result, tmp.get<1>());
  moveTo(high);
  copyField(tmp.get<0>(), tmp.get<1>());
  moveTo(result);
  compare16ToConstDestructive(value, tmp.get<0>(), tmp.select<1>());
  popPtr();
}

void Assembler::moveToDynamicOffset(Cell offsetLow, Cell offsetHigh, TransferMode mode) {

  // This algorithm was designed with a particular ordering of the macrocell in mind.
  // If that ordering changes, this has to be updated as well. This static assert makes
  // sure that we are notified of this, should that ever happen.
  static_assert(MacroCell::Payload1 - MacroCell::Payload0 == 1 &&
		MacroCell::Payload0 - MacroCell::Flag     == 1 &&
		MacroCell::Flag     - MacroCell::Scratch1 == 1 &&
		MacroCell::Scratch1 - MacroCell::Scratch0 == 1,
		"MacroCell structure has changed; moveToDynamicOffset requires "
		"Scratch0, Scratch1, Flag, Payload0 and Payload1 to be consecutive "
		"and in that order.");  

  // First, copy the offsets into temporary storage of the current cell.
  // offsetLow -> Scratch1 and offsetHigh -> Scratch0 (why reverse order?)
  int const base = _dp.current().offset;
  int const stride = MacroCell::FieldCount; 

  pushPtr();

  // Prepare current macrocell to be in this state:
  // Current state:
  // Scratch0 = high 
  // Scratch1 = low
  // Flag     = 0
  // Payload0 = 0
  // Payload1 = 1   <- pointer here

  moveTo(offsetLow);
  copyOrMoveField(mode, Cell{base, MacroCell::Scratch1}, Temps<1>::select(base, MacroCell::Scratch0));
  moveTo(offsetHigh);
  copyOrMoveField(mode, Cell{base, MacroCell::Scratch0}, Temps<1>::select(base, MacroCell::Flag));
  moveTo(base, MacroCell::Payload1);
  inc();

  // From hereon, we use raw moves instead of switchField, because pointer position
  // is not preserved within loops.
  auto left  = [&](int n = 1) { assert(n >= 0); emit<primitive::MovePointerRelative>(-n); };
  auto right = [&](int n = 1) { assert(n >= 0); emit<primitive::MovePointerRelative>(n);  };

  // Daniel's algorithm  
  loopOpen(); {
    // <<<[->>]
    //
    // If low != 0:
    //   --low
    //   jump to Payload0 (known zero)
    left(3);                      // Payload1 -> Scratch1
    loopOpen(); {
      dec();
      right(2);                   // Scratch1 -> Payload0 (guaranteed 0)
    } loopClose();

    // If low was nonzero, the pointer is Payload0 and low was decremented.
    // If low was zero, we're still at low.
    
    // <[->->]
    //
    // Reached only on high when low was zero.
    // If high != 0:
    //   --high
    //   low = 255
    //
    // Both paths finish such that the following +2 reaches Payload1
    // iff the original 16-bit counter was nonzero.

    left();
    loopOpen(); {
      dec();
      right();
      dec();
      right();
    } loopClose();

    right(2);

    // If either byte was nonzero, we're now on Payload1 == 1.
    // If both were zero, we're on Flag == 0 and this is skipped.
    loopOpen(); {

      // <+>
      // Payload0 becomes the second iteration flag.
      left();
      inc();
      right();

      // [-<<<[...move one byte...]>>]
      //
      // First iteration: move low
      // Second iteration: move high
      loopOpen(); {
	dec();
	left(3);

	emit<primitive::MoveData>(stride);

	right(2);
      } loopClose();

      // We're now on Flag in the old macrocell.
      // Move to Flag in the next macrocell.
      right(stride);

      // >>+<
      // Set Payload1 of the new macrocell to 1,
      // ending on Payload0 == 0.
      right(2);
      inc();
      left();

    } loopClose();

    // If we moved:
    //   Payload0(new) -> Payload1(new) == 1
    //
    // If counter was already zero:
    //   Flag(old) -> Payload0(old) == 0
    right();
  
  } loopClose();

  // We terminated on Payload0 of the final macrocell.
  // Move to Payload1 and clear the flag.
  right();
  dec();

  popPtr();
}

void Assembler::fetchFromDynamicOffset(Cell offsetLow, Cell offsetHigh, Payload const &payload, primitive::Direction seekDir,
				       TransferMode dataTransferMode, TransferMode offsetTransferMode) {
  assert(payload);

  int const base = _dp.current().offset;
  pushPtr();
  moveToDynamicOffset(offsetLow, offsetHigh, offsetTransferMode);
  
  // Base is now the cell we arrived at (at offset).
  // Load values into payload
  for (int i = 0; i != payload.size(); ++i) {
    moveTo(base + i, MacroCell::Value0);
    copyOrMoveField(dataTransferMode,
		    Cell{base + i, MacroCell::Payload0},
		    Temps<1>::select(base + i, MacroCell::Scratch0));
    
    if (payload.width(i) == Payload::Width::Double) {
      moveTo(base + i, MacroCell::Value1);
      copyOrMoveField(dataTransferMode,
		      Cell{base + i, MacroCell::Payload1},
		      Temps<1>::select(base + i, MacroCell::Scratch0));
    }
  }
  
  // Bring payload back to cell that contains the SeekMarker
  moveTo(base);
  seek(MacroCell::SeekMarker, seekDir, payload, true);
  popPtr();

  // Transfer complete: payload now in Payload-fields of the base
}
