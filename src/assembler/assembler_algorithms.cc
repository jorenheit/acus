// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::literalBf(std::string const &bf) {
  emit<primitive::Inline>(bf);
}

void Assembler::literalBf(Cell start, std::string const &bf) {
  pushPtr();
  moveTo(start);
  literalBf(bf);
  popPtr();
}

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

void Assembler::zeroCell(Cell target) {
  pushPtr();
  moveTo(target);
  emit<primitive::ZeroCell>();
  popPtr();
}

void Assembler::zeroCell() { 
  emit<primitive::ZeroCell>();
}

void Assembler::zeroCellPlus(Cell target) {
  pushPtr();
  moveTo(target);
  emit<primitive::ZeroCellPlus>();
  popPtr();
}

void Assembler::zeroCellPlus() { 
  emit<primitive::ZeroCellPlus>();
}

void Assembler::setSlotToValue(Slot slot, int value) {
  assert(types::isInteger(slot.type()));

  if (slot.type()->usesValue1()) {
    setToValue16(ws::promiseClean16(slot), value);
  } else {
    setToValue(ws::promiseClean8(slot), value);
    zeroCell(Cell{slot, MacroCell::Value1});
  }
}

Assembler::SingleCell Assembler::setToValue(int value) {
  return setToValue(_dp.current(), value);
}

Assembler::SingleCell Assembler::setToValue(Cell target, int value) {
  return setToValue(SingleCell{target}, value);
}

Assembler::SingleCell Assembler::inc(size_t n) {
  return inc(_dp.current(), n);
}

Assembler::SingleCell Assembler::inc(Cell target, size_t n) {
  return inc(SingleCell{target}, n);
}

Assembler::SingleCell Assembler::dec(size_t n) {
  return dec(_dp.current(), n);
}

Assembler::SingleCell Assembler::dec(Cell target, size_t n) {
  return dec(SingleCell{target}, n);
}

void Assembler::moveField(Cell from, Cell to) {
  if (from.offset == to.offset && from.field == to.field) return;
  zeroCell(to);
  moveFieldToZero(from, to);
}

void Assembler::moveField(Cell from, std::vector<Cell> const &to) {
  for (Cell const &c: to) zeroCell(c);
  moveFieldToZero(from, to);
}

void Assembler::moveFieldToZero(Cell from, Cell to) {
  if (from.offset == to.offset && from.field == to.field) return;
  loop(from, [&]{
    dec(from);
    inc(to);
  });
}

void Assembler::moveFieldToZero(Cell from, std::vector<Cell> const &to) {
  loop(from, [&]{
    dec(from);
    for (Cell const &c: to) inc(c);
  });
}

void Assembler::copyField(Cell from, Cell to, Cell tmp, bool tmpKnownZero) {
  if (from.offset == to.offset && from.field == to.field) return;
  zeroCell(to);
  copyFieldToZero(from, to, tmp, tmpKnownZero);
}

void Assembler::copyField(Cell from, std::vector<Cell> const &to, Cell tmp, bool tmpKnownZero) {
  for (Cell const &c: to) zeroCell(c);
  copyFieldToZero(from, to, tmp, tmpKnownZero);
}

void Assembler::copyFieldToZero(Cell from, Cell to, Cell tmp, bool tmpKnownZero) {
  if (from.offset == to.offset && from.field == to.field) return;
  copyFieldToZero(from, std::vector<Cell>{to}, tmp, tmpKnownZero);
}

void Assembler::copyFieldToZero(Cell from, std::vector<Cell> const &to, Cell tmp, bool tmpKnownZero) {
  if (!tmpKnownZero) zeroCell(tmp);
  auto targets = to;
  targets.push_back(tmp);
  moveFieldToZero(from, targets);
  moveFieldToZero(tmp, from);
}

void Assembler::copyOrMoveField(TransferMode mode, Cell from, Cell to, Cell tmp, bool tmpKnownZero) {
  if (mode == TransferMode::Move) moveField(from, to);
  else copyField(from, to, tmp, tmpKnownZero);
}

void Assembler::copyOrMoveField(TransferMode mode, Cell from, std::vector<Cell> const &to, Cell tmp, bool tmpKnownZero) {
  if (mode == TransferMode::Move) moveField(from, to);
  else copyField(from, to, tmp, tmpKnownZero);
}

void Assembler::copyOrMoveFieldToZero(TransferMode mode, Cell from, Cell to, Cell tmp, bool tmpKnownZero) {
  if (mode == TransferMode::Move) moveFieldToZero(from, to);
  else copyFieldToZero(from, to, tmp, tmpKnownZero);
}

void Assembler::copyOrMoveFieldToZero(TransferMode mode, Cell from, std::vector<Cell> const &to, Cell tmp, bool tmpKnownZero) {
  if (mode == TransferMode::Move) moveFieldToZero(from, to);
  else copyFieldToZero(from, to, tmp, tmpKnownZero);
}
