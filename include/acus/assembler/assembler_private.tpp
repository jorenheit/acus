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


  template <size_t ScratchOffset> requires (ScratchOffset > 0)
  Assembler::SingleAndScratch<ScratchOffset> Assembler::setToValue(SingleAndScratch<ScratchOffset> const &target, int value) {

    pushPtr();
    moveTo(target[0]);
    emit<primitive::ConstructConstant>(value & 0xff, 0, ScratchOffset);
    popPtr();
    return target;
  }


  template <size_t ScratchOffset> requires (ScratchOffset > 0)
  Assembler::DoubleAndScratch<ScratchOffset> Assembler::setToValue16(DoubleAndScratch<ScratchOffset> const &target, int value) {

    setToValue(ws::promise(target[0], ws::Layout<ws::Data, ws::Data, ws::Zero>{}), value & 0xff);
    setToValue(ws::promise(target[1], ws::Layout<ws::Data, ws::Zero>{}),           value >> 8);
    return target;
  }  

  template <size_t ScratchOffset> requires (ScratchOffset > 0)
  Assembler::SingleAndScratch<ScratchOffset> Assembler::addConst(SingleAndScratch<ScratchOffset> const &lhs, int delta) {
    pushPtr();
    moveTo(lhs[0]);
    emit<primitive::ChangeBy>(delta, 0, ScratchOffset);
    popPtr();
    return lhs;
  }


  template <size_t ScratchOffset> requires (ScratchOffset > 0)
  Assembler::SingleAndScratch<ScratchOffset> Assembler::subConst(SingleAndScratch<ScratchOffset> const &lhs, int delta) {
    return addConst(lhs, -delta);
  }
  
  void Assembler::loop(Cell flag, auto&& body) {
    pushPtr();
    moveTo(flag);
    loopOpen(); {
      body();
      moveTo(flag);
    } loopClose();
    popPtr();
  }
  
  template <size_t ScratchOffset> requires (ScratchOffset > 0)
  Assembler::SingleAndScratch<ScratchOffset> Assembler::boolDestructive(SingleAndScratch<ScratchOffset> const &op) {
    loop(op[0], [&]{
      zeroCell(op[0]);
      inc(op[ScratchOffset]);
    });
    addDestructive(op[0], op[ScratchOffset]);
    return op;
  }

  

  
} // namespace acus
