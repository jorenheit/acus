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
  // assert(types::isUnsignedInteger(lhs.type()));
  // assert(types::isUnsignedInteger(rhs.type()));

  assert(lhs != rhs);
  
  // TODO: what if lhs and rhs are aliases?
  Slot const &consumed = destroyRhs ? rhs : lhs;
  Slot const &preserved = destroyRhs ? lhs : rhs;
  constexpr auto Low  = static_cast<MacroCell::Field>(MacroCell::Value0);
  constexpr auto High = static_cast<MacroCell::Field>(Low + 1);
  constexpr auto Temp = static_cast<MacroCell::Field>(Low + 2);
  constexpr auto Zero = static_cast<MacroCell::Field>(Low + 3);
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


// Implementations of mul algorithms



void Assembler::mulConst(int factor, Temps<3> tmp) {
  // TODO: optimize for powers of 2
  // TODO: big factors should have runtime implementation
  
  if (factor == 0) {
    zeroCell();
    return;
  }
  if (factor == 1) return;

  pushPtr();
  Cell const current = _dp.current();
  Cell const copy1 = tmp.get<0>();
  Cell const copy2 = tmp.get<1>();
  
  copyField(copy1, tmp.select<2>());
  copyField(copy2, tmp.select<2>());

  // TODO: rewrite. Don't unroll always
  for (int i = 0; i != std::abs(factor) - 1; ++i) {
    moveTo(current);    
    addDestructive(copy1);
    moveTo(copy2);
    copyField(copy1, tmp.select<2>());
  }

  // Clear temporary copies
  moveTo(copy1); zeroCell();
  moveTo(copy2); zeroCell();
  
  // All temps have been cleared by this point
  if (factor < 0) {
    moveTo(current);
    negateDestructive(tmp.select<0, 1>());
  }
  
  popPtr();
}

// void Assembler::mul16Const(int factor, Temps<3> tmp) {
//   assert(_dp.current().field == MacroCell::Value0);
//   if (factor == 0) {
//     pushPtr();
//     zeroCell();
//     moveTo(Cell{_dp.current(), MacroCell::Value1});
//     zeroCell();
//     popPtr();
//     return;
//   }

//   if (factor == 1) return;

//   Cell const operand = _dp.current();
//   Cell const operandCopy1 = tmp.get<0>();
//   Cell const operandCopy2 = tmp.get<1>();
//   Cell const factorCell = tmp.get<2>();
//   assert(operand.field == MacroCell::Value0);
//   assert(operandCopy1.field == MacroCell::Value0);
//   assert(operandCopy2.field == MacroCell::Value0);
//   assert(factorCell.field == MacroCell::Value0);

//   pushPtr();
  
//   // Initialize first copy of the operand
//   moveTo(operand, MacroCell::Value0);
//   copyField(Cell{operandCopy1, MacroCell::Value0}, Temps<1>::select(operandCopy1, MacroCell::Scratch0));
//   moveTo(operand, MacroCell::Value1);
//   copyField(Cell{operandCopy1, MacroCell::Value1}, Temps<1>::select(operandCopy1, MacroCell::Scratch0));
//   int const count = std::abs(factor) - 1;
//   int const lowCount  = count & 0xff;
//   int const highCount = (count >> 8) & 0xff;

//   // ------------------------------------------------
//   // lowCount * operand
//   // ------------------------------------------------

//   moveTo(factorCell, MacroCell::Value0);
  
//   setToValue(lowCount);

//   loopOpen(); {
//     // Make destructive copy of original operand
//     moveTo(operandCopy1, MacroCell::Value0);
//     copyField(Cell{operandCopy2, MacroCell::Value0}, Temps<1>::select(operandCopy2, MacroCell::Scratch0));

//     moveTo(operandCopy1, MacroCell::Value1);
//     copyField(Cell{operandCopy2, MacroCell::Value1}, Temps<1>::select(operandCopy2, MacroCell::Scratch0));

//     moveTo(operand);
//     add16Destructive(operandCopy2);

//     moveTo(factorCell, MacroCell::Value0);
//     dec();
//   } loopClose();

//   // ------------------------------------------------
//   // highCount * (256 * operand)
//   //
//   // 256 * operand mod 65536 == operand.low << 8
//   // ------------------------------------------------

//   moveTo(factorCell, MacroCell::Value1);
//   setToValue(highCount);

//   loopOpen(); {
//     // We only need a copy of the original low byte
//     moveTo(operandCopy1, MacroCell::Value0);
//     copyField(Cell{operandCopy2, MacroCell::Value0}, Temps<1>::select(operandCopy2, MacroCell::Scratch0));

//     moveTo(operand, MacroCell::Value1);
//     addDestructive(Cell{operandCopy2, MacroCell::Value0});

//     moveTo(factorCell, MacroCell::Value1);
//     dec();    
//   } loopClose();

//   if (factor < 0) {
//     moveTo(operand);
//     negate16Destructive(Cell{operand, MacroCell::Value1},
// 			Temps<5>::select( operand, MacroCell::Scratch0,
// 					  operand, MacroCell::Scratch1,
// 					  operand, MacroCell::Flag,
// 					  operand, MacroCell::Payload0,
// 					  operand, MacroCell::Payload1));
//   }

//   popPtr();
// }


void Assembler::mulDestructive(Cell factor, Temps<3> tmp) {
  pushPtr();
  
  Cell const current = _dp.current();
  Cell const copy1 = tmp.get<0>();
  Cell const copy2 = tmp.get<1>();
  
  copyField(copy1, tmp.select<2>());
  copyField(copy2, tmp.select<2>());
  zeroCell();
  
  moveTo(factor);
  loopOpen(); {
    dec();
    moveTo(current);
    addDestructive(copy1);
    moveTo(copy2);
    copyField(copy1, tmp.select<2>());
    moveTo(factor);
  } loopClose();

  moveTo(copy1); zeroCell();
  moveTo(copy2); zeroCell();

  popPtr();
}

