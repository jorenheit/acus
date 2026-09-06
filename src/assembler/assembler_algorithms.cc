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

void Assembler::inc16() {
  // This overload assumes that the high cell is right next to the current (low) cell,
  // followed by (at least) 3 empty scrach cells. If that is not guaranteed, call the
  // overload below.
  assert(_dp.current().field == MacroCell::Value0);
  emit<primitive::Inline>(">>>>+<<<<+[>>]>+>[<-<]>>-<<<<");
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

void Assembler::dec16() {
  // This overload assumes that the high cell is right next to the current (low) cell,
  // followed by (at least) 3 empty scrach cells. If that is not guaranteed, call the
  // overload below.
  emit<primitive::Inline>(">>>>+<<<<[->>]->->[<+<+]>>-<<<<");
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
  // This algorithm was designed with a particular ordering of the macrocell in
  // mind. If that ordering changes, this has to be updated as well. This static
  // assert makes sure that we are notified of this, should that ever happen.

  static_assert(
      MacroCell::Payload1 - MacroCell::Payload0 == 1 &&
      MacroCell::Payload0 - MacroCell::Flag == 1 &&
      MacroCell::Flag - MacroCell::Scratch1 == 1 &&
      MacroCell::Scratch1 - MacroCell::Scratch0 == 1,
      "MacroCell structure has changed; moveToDynamicOffset requires "
      "|Scratch0, Scratch1, Flag, Payload0 and Payload1 to be consecutive "
      "and in that order.");

  // First, copy the offsets into temporary storage of the current cell.
  // offsetLow -> Scratch1 and offsetHigh -> Scratch0
  int const base = _dp.current().offset;
  int const stride = MacroCell::FieldCount;
  pushPtr();

  // Daniel's algorithm (see full explanation below)  
  moveTo(offsetLow);
  copyOrMoveField(mode,
                  Cell{base, MacroCell::Scratch1},
                  Temps<1>::select(base, MacroCell::Scratch0));

  moveTo(offsetHigh);
  copyOrMoveField(mode,
                  Cell{base, MacroCell::Scratch0},
                  Temps<1>::select(base, MacroCell::Flag));

  moveTo(base, MacroCell::Payload1);
  emit<primitive::Inline>("+[<<<[->>]<[->->]>>[<+>[-<<<");
  emit<primitive::MoveData>(stride);
  emit<primitive::Inline>(">>]");
  emit<primitive::MovePointerRelative>(stride);
  emit<primitive::Inline>(">>+<]>]>-");
  popPtr();

  /*
   * Daniel Cristofani's dynamic-offset algorithm.
   *
   * The current macrocell has been prepared as follows:
   *
   *   Scratch0  Scratch1  Flag  Payload0  Payload1
   *      high      low      0       0         1
   *                                           ^
   *                                         pointer
   *
   * Scratch0/Scratch1 are deliberately reversed compared with the usual
   * low/high ordering. Starting from Payload1, <<< reaches the low byte
   * (Scratch1), while one additional < reaches the high byte (Scratch0).
   * The same geometry is later used to transport low and high with the
   * same piece of code.
   *
   * The complete algorithm is:
   *
   *   +[<<<[->>]<[->->]>>[<+>[-<<< MOVE >>] STEP >>+<]>]>-
   *
   * where MOVE destructively moves the current counter byte to the
   * corresponding field of the next macrocell, and STEP moves the data
   * pointer itself there.
   *
   * It works as follows:
   *
   * 1. +[
   *    Payload1 is set to 1 and used to control the outer loop. Each
   *    iteration consumes one unit of the 16-bit offset and, unless the
   *    offset has reached zero, advances by one macrocell.
   *
   * 2. <<<[->>]
   *    Move from Payload1 to Scratch1 (low).
   *
   *    If low != 0, decrement it once and move two fields right to
   *    Payload0. The loop then terminates immediately because Payload0 is
   *    zero.
   *
   *    If low == 0, the loop is skipped and the pointer remains at
   *    Scratch1.
   *
   * 3. <[->->]
   *    This performs the borrow when the low byte was zero.
   *
   *    - If low was nonzero, the preceding < moves from Payload0 to Flag,
   *      which is zero, so this loop is skipped.
   *
   *    - If low was zero, < moves from Scratch1 to Scratch0 (high). If
   *      high != 0, high is decremented and low is decremented from 0 to
   *      255. The pointer then ends at Flag.
   *
   *    - If both high and low were zero, this loop is skipped while the
   *      pointer remains at Scratch0.
   *
   * 4. >>
   *    This is also the zero test for the complete 16-bit offset.
   *
   *    After a successful decrement (either low-- or high--/low=255), the
   *    pointer was at Flag and therefore arrives at Payload1, which is 1.
   *
   *    If high == low == 0, the pointer was at Scratch0 and therefore
   *    arrives at Flag, which is 0.
   *
   *    Consequently, the following loop is entered iff there was still
   *    one unit of offset to consume.
   *
   * 5. [<+>[-<<< MOVE >>] STEP >>+<]
   *    Move the remaining 16-bit counter to the neighbouring macrocell
   *    and follow it with the data pointer.
   *
   *    <+> sets Payload0 to 1 while Payload1 is already 1. The inner loop
   *    therefore executes twice:
   *
   *      first iteration:
   *        Payload1--, <<< -> Scratch1, MOVE the low byte, >> -> Payload0
   *
   *      second iteration:
   *        Payload0--, <<< -> Scratch0, MOVE the high byte, >> -> Flag
   *
   *    Both control cells have now been cleared and both counter bytes
   *    have been transferred to the neighbouring macrocell.
   *
   *    STEP moves from Flag of the old macrocell to Flag of the new one.
   *    >>+< then sets its Payload1 to 1 and leaves the pointer at
   *    Payload0 (0), causing this inner movement loop to terminate.
   *
   * 6. >
   *    After a move, Payload0 -> Payload1, whose value is 1, so the outer
   *    loop continues in the new macrocell.
   *
   *    If the counter was already zero, the movement loop in step 5 was
   *    skipped while the pointer was at Flag; this > therefore reaches
   *    Payload0 (0), causing the outer loop to terminate instead.
   *
   * 7. >-
   *    On termination the pointer is at Payload0 of the destination
   *    macrocell. Move to Payload1 and clear its remaining 1. All helper
   *    fields are now zero again and the pointer ends at Payload1.
   *
   * A notable feature of this algorithm is that the runtime pointer
   * position itself carries control-flow state: at several points the
   * same relative move has a different meaning depending on which branch
   * was taken. This is why the ordering of the five helper fields is part
   * of the algorithm's required layout.
   */  
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
