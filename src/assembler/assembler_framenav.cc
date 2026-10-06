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
  zeroCell();
  inc();
  moveToOrigin();
}

void Assembler::popFrame() {
  assert(_currentBlock != nullptr);
  assert(_currentFunction != nullptr);
  assert(_currentSeq != nullptr);

  // Check if function is the entry-point of the program. If so, we need to set the
  // TargetBlock's high-byte to 0 to abort the program.
  // If not, we do a dynamic move left until we hit the next frame marker.

  pushPtr();
  if (_currentFunction->name == _program.entryFunctionName) {
    moveTo(FrameLayout::TargetBlock, MacroCell::Value1);
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

// TODO: don't seek from current offset, but accept Cell instead of Field and move from there
void Assembler::seek(MacroCell::Field markerField, primitive::Direction dir, Payload const &payload, bool checkCurrent) {  

  auto step = [&]{
    int const stride = MacroCell::FieldCount * ((dir == primitive::Right) ? 1 : -1);    
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
  };

  bool const usingBinaryMarker = (markerField == MacroCell::SeekMarker ||
                                  markerField == MacroCell::FrameMarker);

  pushPtr();
  if (usingBinaryMarker) {
    // For binary markers, we can use an optimized version of the seek-algorithm.
    // Credits to Daniel. Basically (ignoring payload): -[+>>>>>>>>>-]+
    
    if (not checkCurrent) {
      // If the seek starts at the next cell, skip the current one
      step();
    }

    // Keep stepping until we hit the marker
    Cell const marker = {_dp.current(), markerField};
    dec(marker);
    loop(marker, [&]{
      inc(marker);
      step();
      dec(marker);
    });
    inc(marker);
  
  } else {
    // For other markers that can have values > 1, we need the more general algorithm
    // That does a NOT operation on the marker-fields

    Cell const flag{_dp.current().offset, MacroCell::Flag};
    auto const writeNotMarkerToFlag = [&]{
      Cell const marker{_dp.current().offset, markerField};
      Cell const scratch{_dp.current().offset, MacroCell::Scratch0};

      copyField(marker, flag, scratch, true);
      notDestructive(flag, scratch);
    };
    
    if (not checkCurrent)  setToValue(flag, 1);
    else                   writeNotMarkerToFlag();

    loop(flag, [&]{
      dec(flag);
      step();

      // Store NOT(marker) in Flag. A nonzero marker clears Flag and exits the loop.
      writeNotMarkerToFlag();
    });
  }

  popPtr();
}


void Assembler::setSeekMarker(Cell cell) {
  setToValue(cell, 1);
}

void Assembler::setSeekMarker(int offset) {
  setSeekMarker(Cell{offset, MacroCell::SeekMarker});
}

void Assembler::setSeekMarker() {
  setSeekMarker(Cell{_dp.current(), MacroCell::SeekMarker});
}

void Assembler::resetSeekMarker(Cell cell) {
  zeroCell(cell);  
}

void Assembler::resetSeekMarker(int offset) {
  resetSeekMarker(Cell{offset, MacroCell::SeekMarker});  
}

void Assembler::resetSeekMarker() {
  resetSeekMarker(Cell{_dp.current(), MacroCell::SeekMarker});
}

void Assembler::moveToPreviousFrame(Payload const &payload) {
  moveToOrigin();
  seek(MacroCell::FrameMarker, primitive::Left, payload, false);
}



