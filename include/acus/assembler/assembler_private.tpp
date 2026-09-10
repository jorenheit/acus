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

// template <typename TrueBranch, typename FalseBranch>
// void Assembler::branchOnSignBit(Slot slot, Cell const &flagCell, TrueBranch&& trueBranch, FalseBranch&& falseBranch) {

//   pushPtr();
//   moveTo(slot, slot.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  
//   signBitConstructive(flagCell,
// 		      Temps<3>::select(slot, MacroCell::Scratch0,
// 				       slot, MacroCell::Scratch1,
// 				       slot, MacroCell::Payload0));
  
//   moveTo(slot, MacroCell::Scratch0);
//   zeroCell(); inc();
//   moveTo(flagCell);
//   loopOpen(); {
//     moveTo(slot, MacroCell::Scratch0); zeroCell();
//     moveTo(flagCell); zeroCell();
//     trueBranch();
//     moveTo(flagCell);
//   } loopClose();

//   moveTo(slot, MacroCell::Scratch0);
//   loopOpen(); {
//     moveTo(slot, MacroCell::Scratch0);  zeroCell();
//     falseBranch();
//     moveTo(slot, MacroCell::Scratch0);
//   } loopClose();
//   popPtr();
// }


template <typename TrueBranch, typename FalseBranch>
void Assembler::branchOnSignBit(Slot slot, TrueBranch&& trueBranch, FalseBranch&& falseBranch) {

  pushPtr();

  Slot const tmp = getTemp(ts::s8());
  moveTo(slot, slot.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0);
  copyField(Cell{tmp, MacroCell::Value0}, Temps<1>::select(tmp, MacroCell::Scratch0));
  
  signBitSlot(tmp);
  moveTo(tmp, MacroCell::Value1); inc();
  moveTo(tmp, MacroCell::Value0);
  loopOpen(); {
    trueBranch();
    moveTo(tmp, MacroCell::Value1); zeroCell();
    moveTo(tmp, MacroCell::Value0); zeroCell();
  } loopClose();

  moveTo(tmp, MacroCell::Value1);
  loopOpen(); {
    falseBranch();
    moveTo(tmp, MacroCell::Value1); zeroCell();
  } loopClose();
  popPtr();

  freeSlot(tmp);
}
  

  

  
} // namespace acus
