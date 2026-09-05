// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

namespace acus {

template <typename Primitive, typename ... Args>
void Assembler::emit(Args&& ... args) {
  assert(_currentSeq != nullptr);
  _currentSeq->emplace<Primitive>(std::forward<Args>(args)...);
  
}

template <typename... Args> requires ((std::convertible_to<Args, Cell>) && ...)
auto Assembler::getFieldIndices(Args... args) {
  return std::make_tuple(getFieldIndex(static_cast<Cell>(args))...);
}

template <typename TrueBranch, typename FalseBranch>
void Assembler::branchOnSignBit(Slot slot, Cell const &flagCell, TrueBranch&& trueBranch, FalseBranch&& falseBranch) {

  pushPtr();
  moveTo(slot, slot.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);    
  signBitConstructive(flagCell,
		      Temps<3>::select(slot, MacroCell::Scratch0,
				       slot, MacroCell::Scratch1,
				       slot, MacroCell::Payload0));
  moveTo(slot, MacroCell::Scratch0);
  setToValue(1);
  moveTo(flagCell);
  loopOpen(); {
    moveTo(slot, MacroCell::Scratch0); zeroCell();
    moveTo(flagCell); zeroCell();
    trueBranch();
    moveTo(flagCell);
  } loopClose();

  moveTo(slot, MacroCell::Scratch0);
  loopOpen(); {
    moveTo(slot, MacroCell::Scratch0);  zeroCell();
    falseBranch();
    moveTo(slot, MacroCell::Scratch0);
  } loopClose();
  popPtr();
}

} // namespace acus
