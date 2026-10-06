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
void Assembler::branchOnSignBit(Slot slot, TrueBranch&& trueBranch, FalseBranch&& falseBranch) {

  Slot const tmp = getTemp(ts::s8(), allocHint(slot));
  Cell const signBit = {tmp, MacroCell::Value0};
  Cell const elseBit = {tmp, MacroCell::Value1};

  copyField(Cell{slot, slot.type()->usesValue1() ? MacroCell::Value1 : MacroCell::Value0},
            signBit,
            Cell{slot, MacroCell::Scratch0}, true);
  
  signBitSlot(tmp);
  setToValue(elseBit, 1);
  loop(signBit, [&]{
    dec(signBit);
    dec(elseBit);
    trueBranch();
  });

  loop(elseBit, [&]{
    dec(elseBit);
    falseBranch();
  });
  
  freeSlot(tmp);
}

// Loop on specific cell
void Assembler::loop(Cell flag, auto&& body) {
  pushPtr();
  moveTo(flag);
  loopOpen(); {
    body();
    moveTo(flag);
  } loopClose();
  popPtr();
}

// Loop on current cell
void Assembler::loop(auto&& body) {
  Cell const flag = _dp.current();
  loop(flag, body);
}

} // namespace acus
