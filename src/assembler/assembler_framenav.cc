// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

void Assembler::pushPtr() {
  _ptrStack.push(_dp.current());
}

void Assembler::popPtr() {
  moveTo(_ptrStack.top());
  _ptrStack.pop();
}

void Assembler::pushFrame() {
  assert(_currentBlock != nullptr);
  assert(_currentFunction != nullptr);
  assert(_currentSeq != nullptr);

  // To push a frame, we need to move the pointer into the cell that marks the start of a fresh
  // frame, starting just beyond the current one. We also increment the FrameMarker set its run-state to 1.

  primitive::DInt currentFrameSize = [caller = _currentFunction->name](primitive::Context const &ctx){
    return ctx.getStackFrameSize(caller) * MacroCell::FieldCount;
  };

  moveTo(0, MacroCell::FrameMarker);
  emit<primitive::MovePointerRelative>(currentFrameSize);
  setToValue(1);
  moveTo(FrameLayout::RunState, MacroCell::Value0);
  setToValue(1);
  moveToOrigin();
}

void Assembler::popFrame() {
  assert(_currentBlock != nullptr);
  assert(_currentFunction != nullptr);
  assert(_currentSeq != nullptr);

  // Check if function is the entry-point of the program. If so, we need to set the run-cell to 0
  // If not, we do a dynamic move left until we hit the next frame marker.

  pushPtr();
  if (_currentFunction->name == _program.entryFunctionName) {
    moveTo(FrameLayout::RunState, MacroCell::Value0);
    zeroCell();
    moveToOrigin();
  }
  else {
    moveToOrigin();
    switchField(MacroCell::FrameMarker);
    zeroCell();
    moveToPreviousFrame();
    // Pointer should now be at the start of the previous frame
  }
  popPtr();
}


void Assembler::seek(MacroCell::Field markerField, primitive::Direction dir, Payload const &payload, bool checkCurrent) {  
  int const stride = MacroCell::FieldCount * ((dir == primitive::Right) ? 1 : -1);

  if (not checkCurrent) {
    switchField(MacroCell::Flag);
    setToValue(1);
  }
  else {
    switchField(markerField);
    notConstructive(Cell{_dp.current().offset, MacroCell::Flag},
		    Temps<1>::select(_dp.current().offset, MacroCell::Scratch0));
    switchField(MacroCell::Flag);
  }

  loopOpen(); {
    zeroCell();
    
    // Move pointer and payload to the next cell
    int const start = (dir == primitive::Right) ? payload.size() - 1 : 0;
    int const diff  = (dir == primitive::Right) ? -1 : 1;
    auto const cmp  = [&](int i)  { return (dir == primitive::Right) ? (i >= 0) : (i != payload.size()); };

    pushPtr();    
    moveRel(start);
    for (int i = start; cmp(i); i += diff) {
      switchField(MacroCell::Payload0);
      emit<primitive::MoveData>(stride);
      if (payload.width(i) == Payload::Width::Double) {
	switchField(MacroCell::Payload1);
	emit<primitive::MoveData>(stride);
      }
      moveRel(diff);
    }
    popPtr();
    emit<primitive::MovePointerRelative>(stride);
    
    // Check if flag was hit by storing NOT(SeekMarker) in Flag. If hit, flag0 becomes 0 and we exit the loop
    switchField(markerField);
    notConstructive(Cell{_dp.current().offset, MacroCell::Flag},
		    Temps<1>::select(_dp.current().offset, MacroCell::Scratch0));
    switchField(MacroCell::Flag);
  } loopClose();
}


void Assembler::setSeekMarker() {
  pushPtr();
  switchField(MacroCell::SeekMarker);
  setToValue(1);
  popPtr();
}

void Assembler::resetSeekMarker() {
  pushPtr();
  switchField(MacroCell::SeekMarker);
  zeroCell();
  popPtr();
}

void Assembler::moveToPreviousFrame(Payload const &payload) {
  moveToOrigin();
  seek(MacroCell::FrameMarker, primitive::Left, payload, false);
}



