// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::mulSlotByConst(Slot lhs, int factor) {
  assert(types::isInteger(lhs.type()));

  if (factor == 0) {
    zeroCell();
    return;
  }
  if (factor == 1) return;

  bool negate = false;
  if (factor < 0) {
    factor = -factor;
    negate = true;
  }
  
  Slot factorSlot = getTemp(lhs.type());
  setSlotToValue(factorSlot, factor);
  mulSlotBySlotUnsigned(lhs, factorSlot, true);
  freeSlot(factorSlot);

  if (negate) {
    negateSlot(lhs);
  }
}


void Assembler::mulSlotBySlot(Slot lhs, Slot rhs) {
  assert(types::isInteger(lhs.type()));
  assert(types::isInteger(rhs.type()));

  pushPtr();

  if (types::isSignedInteger(lhs.type())
      && lhs.type()->usesValue1()
      && !rhs.type()->usesValue1()) {

    Slot rhsWide = getTemp(ts::s16());
    assignSlot(rhsWide, rhs);  // existing signed widening -> sign extension

    // rhsWide may be destroyed
    mulSlotBySlotUnsigned(lhs.unsignedView(), rhsWide.unsignedView(), true);

    freeTempSlot(rhsWide);
  }
  else {
    mulSlotBySlotUnsigned(
        lhs.unsignedView(),
        rhs.unsignedView());
  }

  popPtr();
}

void Assembler::mulSlotBySlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs) {
  assert(lhs != rhs);

  auto const withValueWorkspace = [&](Slot slot, auto &&action) {
    if (slot.type()->usesValue1()) {
      auto const work = ws::promiseClean16(slot);
      action(DoubleCell{work}, work);
    } else {
      auto const work = ws::promiseClean8(slot);
      action(SingleCell{work}, work);
    }
  };

  // The non-consumed operand supplies the five clean work cells. Keeping this
  // choice preserves the old implementation's useful property that an
  // ordinary x *= y does not require a temporary copy of y.
  withValueWorkspace(lhs, [&](auto const &lhsValue, auto const &lhsWork) {
    if (destroyRhs) {
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
