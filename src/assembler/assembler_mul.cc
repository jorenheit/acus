// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::mulSlotByConst(Slot lhs, int factor) {
  assert(types::isInteger(lhs.type()));
  bool negate = false;
  if (factor < 0) {
    factor = -factor;
    negate = true;
  }

  // Special cases
  if (factor == 0)  {
    setSlotToValue(lhs, 0);
    return;
  }
  if (factor == 1)  {
    if (negate) negateSlot(lhs);
    return;
  }
  if (factor == 2) {
    twiceSlot(lhs);
    if (negate) negateSlot(lhs);
    return;
  }
  if (util::math::isPowerOfTwo(factor)) {
    mulSlotByPowerOfTwo(lhs, util::math::getPowerOfTwo(factor));
    if (negate) negateSlot(lhs);
    return;
  }
  if (factor > 3 && util::math::isPowerOfTwo(factor - 1)) {
    Slot const copy = getTemp(lhs.type());
    assignSlot(copy, lhs);
    mulSlotByPowerOfTwo(lhs, util::math::getPowerOfTwo(factor - 1));
    addSlotToSlot(lhs, copy, true);
    if (negate) negateSlot(lhs);
    freeTempSlot(copy);
    return;
  }
  if (util::math::isPowerOfTwo(factor + 1)) {
    Slot const copy = getTemp(lhs.type());
    assignSlot(copy, lhs);
    mulSlotByPowerOfTwo(lhs, util::math::getPowerOfTwo(factor + 1));
    subSlotFromSlot(lhs, copy, true);
    if (negate) negateSlot(lhs);
    freeTempSlot(copy);
    return;
  }

  // General multiplication
  Slot factorSlot = getTemp(lhs.type());
  setSlotToValue(factorSlot, factor);
  mulSlotBySlotUnsigned(lhs, factorSlot, true);
  if (negate) negateSlot(lhs);

  freeSlot(factorSlot);
}


void Assembler::mulSlotBySlot(Slot lhs, Slot rhs, bool consumeRhs) {
  consumeRhs = consumeRhs && lhs != rhs;
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));

  if (types::isSignedInteger(lhs.type()) && lhs.type()->usesValue1() && !rhs.type()->usesValue1()) {
    Slot rhsWide = getTemp(ts::s16());
    assignSlot(rhsWide, rhs, consumeRhs ? TransferMode::Move : TransferMode::Copy);  // existing signed widening -> sign extension

    // rhsWide may be destroyed
    mulSlotBySlotUnsigned(lhs.unsignedView(), rhsWide.unsignedView(), true);
    freeTempSlot(rhsWide);
  }
  else {
    mulSlotBySlotUnsigned(lhs.unsignedView(), rhs.unsignedView(), consumeRhs);
  }
}

void Assembler::mulSlotBySlotUnsigned(Slot lhs, Slot rhs, bool consumeRhs) {
  consumeRhs = consumeRhs && lhs != rhs;

  if (lhs == rhs) {
    assert(consumeRhs == false);
    return squareSlot(lhs);
  }

  auto const withValueWorkspace = [&](Slot slot, auto &&action) {
    // Work-area is the scratch space of this slot
    auto const work = ws::promise(Cell{slot, MacroCell::Scratch0}, ws::Layout<ws::ScratchCells<5>>{});
    if (slot.type()->usesValue1()) {
      action(DoubleCell{ws::promiseClean16(slot)}, work);
    } else {
      action(SingleCell{ws::promiseClean8(slot)}, work);
    }
  };

  // The non-consumed operand supplies the five clean work cells. Keeping this
  // choice preserves the old implementation's useful property that an
  // ordinary x *= y does not require a temporary copy of y.
  withValueWorkspace(lhs, [&](auto const &lhsValue, auto const &lhsWork) {
    if (consumeRhs) {
      withValueWorkspace(rhs, [&](auto const &rhsValue, auto const &) {
        multiplyInto(lhsValue, rhsValue, lhsValue, lhsWork);
      });
    } else {
      withValueWorkspace(rhs, [&](auto const &rhsValue, auto const &rhsWork) {
        multiplyInto(lhsValue, lhsValue, rhsValue, rhsWork);
      });
    }
  });
}

void Assembler::squareSlot(Slot slot) {
  if (slot.type()->usesValue1()) {
    square16Destructive(ws::promiseClean16(slot));
  } else {
    squareDestructive(ws::promiseClean8(slot));
  }  
}

void Assembler::twiceSlot(Slot slot) {
  if (slot.type()->usesValue1()) {
    twice16Destructive(ws::promiseClean16(slot));
  } else {
    twiceDestructive(ws::promiseClean8(slot));
  }    
}

void Assembler::mulSlotByPowerOfTwo(Slot slot, size_t power) {
  if (slot.type()->usesValue1()) {
    mulByPowerOfTwo16Destructive(ws::promiseClean16(slot), power);
  } else {
    mulByPowerOfTwoDestructive(ws::promiseClean8(slot), power);
  }    
}

void Assembler::twiceDestructive(Cell x, Cell tmp) {
  loop(x, [&]{
    dec(x);
    inc(tmp, 2);
  });
  addDestructive(SingleCell{x}, tmp);
}

void Assembler::mulByPowerOfTwoDestructive(Cell x, Cell p, Cell tmp) {
  loop(p, [&]{
    dec(p);
    twiceDestructive(x, tmp);
  });
}
