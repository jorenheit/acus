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

// TODO: factor out mul and mul16 kernels
void Assembler::mulSlotBySlotUnsigned(Slot lhs, Slot rhs, bool const destroyRhs) {
  // assert(types::isUnsignedInteger(lhs.type()));
  // assert(types::isUnsignedInteger(rhs.type()));

  assert(lhs != rhs);
  
  // TODO: what if lhs and rhs are aliases?
  Slot const &consumed = destroyRhs ? rhs : lhs;
  Slot const &preserved = destroyRhs ? lhs : rhs;
  constexpr auto Low  = static_cast<MacroCell::Field>(MacroCell::Value0);
  constexpr auto High = static_cast<MacroCell::Field>(Low + 1);
  constexpr auto Temp = static_cast<MacroCell::Field>(Low + 2);
  //  constexpr auto Zero = static_cast<MacroCell::Field>(Low + 3);
  constexpr auto ResultLow = static_cast<MacroCell::Field>(Low + 4);
  constexpr auto ResultHigh = static_cast<MacroCell::Field>(Low + 5);
  // Assume: rhs_low | rhs_high | temp | zero | result_low | result_high
  
  pushPtr();
  if (lhs.type()->usesValue1()) {
    
    moveTo(consumed, Low);
    loopOpen(); {
      dec();
      if (preserved.type()->usesValue1()) {
	// TODO: make this idiom a primitive: move/copy to multiple targets
	// Add rhs_high into result_high, ignore overflow
	moveTo(preserved, High);
	loopOpen(); {
	  switchField(Temp); inc();
	  switchField(ResultHigh); inc();
	  switchField(High); dec();
	} loopClose();
	switchField(Temp);
	loopOpen(); {
	  switchField(High); inc();
	  switchField(Temp); dec();
	} loopClose();
      }
      // Add rhs_low into result_low, overflow into result_high,
      // keep a copy in temp.
      moveTo(preserved, Low);
      emit<primitive::Inline>("[>>+>>>+<+[>-<<]<[>]<<<-]");
      // move temp back into rhs_low to reconstruct it
      switchField(Temp);
      loopOpen(); {
	switchField(Low); inc();
	switchField(Temp); dec();
      } loopClose();
      moveTo(consumed, Low);
    } loopClose();

    if (consumed.type()->usesValue1()) {
      moveTo(consumed, High);
      loopOpen(); {
	dec();
      
	// Add rhs_low to result_high, keeping a copy in rhs_temp
	moveTo(preserved, Low);
	loopOpen(); {
	  switchField(Temp);        inc();
	  switchField(ResultHigh);  inc();
	  switchField(Low);         dec();	  
	} loopClose();
	
	// move temp back into rhs_low to reconstruct it
	switchField(Temp);
	loopOpen(); {
	  switchField(Low);  inc();
	  switchField(Temp); dec();
	} loopClose();

	moveTo(consumed, High);
      } loopClose();
    }
    
    moveTo(preserved, ResultLow);
    moveField(Cell{lhs, Low});
    moveTo(preserved, ResultHigh);
    moveField(Cell{lhs, High});
    
  } else {

    moveTo(consumed, Low);
    loopOpen(); {
      dec();
      
      // Add rhs_low to result_high, keeping a copy in rhs_temp
      moveTo(preserved, Low);
      loopOpen(); {
	switchField(Temp);        inc();
	switchField(ResultLow);  inc();
	switchField(Low);         dec();	  
      } loopClose();
	
      // move temp back into rhs_low to reconstruct it
      switchField(Temp);
      loopOpen(); {
	switchField(Low);  inc();
	switchField(Temp); dec();
      } loopClose();

      moveTo(consumed, Low);
    } loopClose();

    moveTo(preserved, ResultLow);
    moveField(Cell{lhs, Low});
  }
  popPtr();
}

