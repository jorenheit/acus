// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"


void Assembler::fetchGlobal(Slot globalSlot, Slot localSlot) {
  assert(globalSlot.type() == localSlot.type());
  assert(globalSlot.size() == localSlot.size());

  pushPtr();
  int const size = globalSlot.type()->size();
  bool const useValue1 = globalSlot.type()->usesValue1();

  
  // Move to global frame 
  moveToOrigin();
  setSeekMarker();
  seek(MacroCell::SeekMarker, primitive::Left, {}, false);

  for (int i = 0; i != size; ++i) {
    moveTo(globalSlot + i, MacroCell::Value0);
    copyField(Cell{globalSlot + i, MacroCell::Payload0},
	      Temps<1>::select(globalSlot + i, MacroCell::Scratch0));
    
    if (useValue1) {
      moveTo(globalSlot + i, MacroCell::Value1); 
      copyField(Cell{globalSlot + i, MacroCell::Payload1},
		Temps<1>::select(globalSlot + i, MacroCell::Scratch0));
    }
  }

  // Bring payload back to origin-frame
  Payload payload(size,
		  useValue1 ? Payload::Width::Double : Payload::Width::Single);

  moveTo(globalSlot);
  seek(MacroCell::SeekMarker, primitive::Right, payload, false);
  resetSeekMarker();
  switchField(static_cast<MacroCell::Field>(0));
  resetOrigin();
    
  // Move the payload into the local slot
  for (int i = 0; i != size; ++i) {
    moveTo(i, MacroCell::Payload0);
    moveField(Cell{localSlot + i, MacroCell::Value0});
    if (useValue1) {
      moveTo(i, MacroCell::Payload1);
      moveField(Cell{localSlot + i, MacroCell::Value1});
    }
  }
  
  popPtr();
}

void Assembler::putGlobal(Slot globalSlot, Slot localSlot) {
  assert(globalSlot.type() == localSlot.type());

  bool const useValue1 = globalSlot.type()->usesValue1();
  int const size = globalSlot.type()->size();

  pushPtr();

  // Copy value into payload slots starting at the origin
  for (int i = 0; i != size; ++i) {
    moveTo(localSlot + i, MacroCell::Value0);
    copyField(Cell{i, MacroCell::Payload0},
	      Temps<1>::select(i, MacroCell::Scratch0));

    if (useValue1) {
      moveTo(localSlot + i, MacroCell::Value1);
      copyField(Cell{i, MacroCell::Payload1},
		Temps<1>::select(i, MacroCell::Scratch0));
    }
  }

  // Move the payload back to the global frame
  Payload payload(size,
		  useValue1 ? Payload::Width::Double : Payload::Width::Single);
   
  moveToOrigin();
  setSeekMarker();
  seek(MacroCell::SeekMarker, primitive::Left, payload, false);
  
  // Move payload into global slot
  for (int i = 0; i != size; ++i) {
    moveTo(i, MacroCell::Payload0);
    moveField(Cell{globalSlot + i, MacroCell::Value0});
    if (useValue1) {
      moveTo(i, MacroCell::Payload1);
      moveField(Cell{globalSlot + i, MacroCell::Value1});
    }
  }
  
  // Return to origin frame
  moveToOrigin();
  seek(MacroCell::SeekMarker, primitive::Right, {}, false);
  resetSeekMarker();
  
  popPtr();
}

void Assembler::putGlobal(Slot globalSlot, literal::Literal const value) {
  assert(globalSlot.type() == value.type());

  // Move to the global frame (no payload)
  pushPtr();  

  moveToOrigin();
  setSeekMarker();
  seek(MacroCell::SeekMarker, primitive::Left, {}, false);
  assignSlot(globalSlot, value);
  
  // Return to origin frame
  moveToOrigin();
  seek(MacroCell::SeekMarker, primitive::Right, {}, false);
  resetSeekMarker();
  popPtr();
}


