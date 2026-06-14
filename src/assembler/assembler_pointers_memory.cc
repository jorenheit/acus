// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

Expression Assembler::addressOfImpl(Expression const &obj, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  assert(not obj.isLiteral());
  
  if (obj.slot()->directAbsolute()) {
    Slot const slot = materialize(obj.slot());
    API_REQUIRE(slot.kind != Slot::Temp,
		error::ErrorCode::TakingAddressOfTemporary,
		"Cannot take the address of a temporary value.");
  }
  
  return Expression(obj.slot()->addressOf(*this));
}

Slot Assembler::addressOfSlot(Slot const &pointeeSlot) {
  assert(pointeeSlot.kind != Slot::Temp && "taking address of temp");
  assert(pointeeSlot.kind != Slot::Cache && "taking address of cache");

  types::TypeHandle const pointeeType = pointeeSlot.type;
  types::TypeHandle const pointerType = ts::pointer(pointeeType);

  // Set frame-depth to 0 for a local pointer, FrameID for a global pointer
  Slot const ptrSlot = getTemp(pointerType);
  if (pointeeSlot.kind == Slot::Kind::Global) {
    moveTo(0, MacroCell::FrameMarker);
    copyField(Cell{ptrSlot + RuntimePointer::FrameDepth, MacroCell::Value0},
	      Temps<1>::select(ptrSlot + RuntimePointer::FrameDepth, MacroCell::Scratch0));
  } else {
    moveTo(ptrSlot + RuntimePointer::FrameDepth, MacroCell::Value0);
    zeroCell();
  }

  // Construct offset in second cell
  int const offset = pointeeSlot.offset;  
  moveTo(ptrSlot + RuntimePointer::Offset, MacroCell::Value0);
  setToValue(offset & 0xff, Temps<1>::select(ptrSlot + RuntimePointer::Offset, MacroCell::Scratch0));
  moveTo(ptrSlot + RuntimePointer::Offset, MacroCell::Value1);
  setToValue((offset >> 8) & 0xff, Temps<1>::select(ptrSlot + RuntimePointer::Offset, MacroCell::Scratch0));

  return ptrSlot;
}

void Assembler::copyElementIntoSlot(Slot const &elementSlot, Slot const &arrSlot, Slot const &indexSlot) {
  assert(types::isArrayLike(arrSlot.type));
  assert(types::isInteger(indexSlot.type));
  assert(elementSlot.type == types::cast<types::ArrayLike>(arrSlot.type)->elementType());
  types::TypeHandle elementType = elementSlot.type;
  
  pushPtr();

  Slot const scaledIndexSlot = getTemp(ts::u8());
  assignSlot(scaledIndexSlot, indexSlot);
  moveTo(scaledIndexSlot, MacroCell::Value0);
  mulConst(elementType->size(),
	   Temps<3>::select(scaledIndexSlot, MacroCell::Scratch0,
			    scaledIndexSlot, MacroCell::Scratch1,			  
			    scaledIndexSlot, MacroCell::Payload0
			    ));

  Payload payload(elementType->size(),
		  elementType->usesValue1() ? Payload::Width::Double : Payload::Width::Single);
  
  moveTo(arrSlot, MacroCell::Value0);
  setSeekMarker();
  fetchFromDynamicOffset(Cell{scaledIndexSlot, MacroCell::Value0},
			 Cell{scaledIndexSlot, MacroCell::Value1},
			 payload,
			 primitive::Left);

  for (int i = 0; i != elementType->size(); ++i) {
    moveTo(arrSlot + i, MacroCell::Payload0);
    moveField(Cell{elementSlot + i, MacroCell::Value0});
    if (elementType->usesValue1()) {
      moveTo(arrSlot + i, MacroCell::Payload1);
      moveField(Cell{elementSlot + i, MacroCell::Value1});
    }
  }
  
  moveTo(arrSlot);
  resetSeekMarker();
  popPtr();

}

void Assembler::copySlotIntoElement(Slot const &srcSlot, Slot const &arrSlot, Slot const &indexSlot) {
  assert(types::isArrayLike(arrSlot.type));
  assert(types::isInteger(indexSlot.type));

  types::TypeHandle elementType = types::cast<types::ArrayLike>(arrSlot.type)->elementType();
  assert(srcSlot.type == elementType);

  pushPtr();

  Slot const scaledIndexSlot = getTemp(ts::u8());
  assignSlot(scaledIndexSlot, indexSlot);
  moveTo(scaledIndexSlot, MacroCell::Value0);
  mulConst(elementType->size(),
	   Temps<3>::select(scaledIndexSlot, MacroCell::Scratch0,
			    scaledIndexSlot, MacroCell::Scratch1,			  
			    scaledIndexSlot, MacroCell::Payload0
			    ));
  
  // Plant a seek marker at the start of the array
  moveTo(arrSlot, MacroCell::Value0);
  setSeekMarker();

  // Plant another marker one (full element) beyond the start of the element we need
  goToDynamicOffset(Cell{scaledIndexSlot, MacroCell::Value0},
		    Cell{scaledIndexSlot, MacroCell::Value1});

  _dp.set(0);
  moveTo(elementType->size()); 
  setSeekMarker();
  moveTo(0);

  // Move back to the start of the array
  seek(MacroCell::SeekMarker, primitive::Left, {}, true);
  _dp.set(arrSlot);
  
  for (int i = 0; i != elementType->size(); ++i) {
    // Copy the contents into the payload cells
    moveTo(srcSlot + i, MacroCell::Value0);
    copyField(Cell{arrSlot + i, MacroCell::Payload0},
	      Temps<1>::select(arrSlot + i, MacroCell::Scratch0));
    if (elementType->usesValue1()) {
      moveTo(srcSlot + i, MacroCell::Value1);
      copyField(Cell{arrSlot + i, MacroCell::Payload1},
		Temps<1>::select(arrSlot + i, MacroCell::Scratch0));
    }
  }

  // Move the payload into the cell containing the marker (one beyond actual start of the element)
  moveTo(arrSlot);

  Payload payload(elementType->size(),
		  elementType->usesValue1() ? Payload::Width::Double : Payload::Width::Single);	  
  
  seek(MacroCell::SeekMarker, primitive::Right, payload, false);
  _dp.set(elementType->size());
    
  // Move the payload into the value-cells
  // Pointer value set to the start of the next element, so offset 0 represents the start of the target element
  for (int i = 0; i != elementType->size(); ++i) {
    moveTo(elementType->size() + i, MacroCell::Payload0);
    moveField(Cell{i, MacroCell::Value0});
    if (elementType->usesValue1()) {
      moveTo(elementType->size() + i, MacroCell::Payload1);
      moveField(Cell{i, MacroCell::Value1});
    }
  }

  moveTo(elementType->size());
  resetSeekMarker();

  // Go back to the start of the array
  seek(MacroCell::SeekMarker, primitive::Left, {}, false);
  _dp.set(arrSlot);
  resetSeekMarker();
  popPtr();
}

void Assembler::copyConstIntoElement(literal::Literal const value, Slot const &arrSlot, Slot const &indexSlot) {
  assert(types::isArrayLike(arrSlot.type));
  assert(types::isInteger(indexSlot.type));

  types::TypeHandle elementType = types::cast<types::ArrayLike>(arrSlot.type)->elementType();
  assert(value->type() == elementType);

  pushPtr();

  Slot const scaledIndexSlot = getTemp(ts::u8());
  assignSlot(scaledIndexSlot, indexSlot);
  moveTo(scaledIndexSlot, MacroCell::Value0);
  mulConst(elementType->size(),
	   Temps<3>::select(scaledIndexSlot, MacroCell::Scratch0,
			    scaledIndexSlot, MacroCell::Scratch1,			  
			    scaledIndexSlot, MacroCell::Payload0
			    ));
  
  // Plant a seek marker at the start of the array
  moveTo(arrSlot, MacroCell::Value0);
  setSeekMarker();

  // Move to the element-slot
  goToDynamicOffset(Cell{scaledIndexSlot, MacroCell::Value0},
		    Cell{scaledIndexSlot, MacroCell::Value1});

  // Rebase the datapointer and use assign the constant value to the slot
  _dp.set(0);
  Slot const elementSlot {
    .type = elementType,
    .kind = Slot::Kind::Dummy,
    .offset = 0
  };
  assignSlot(elementSlot, value);

  // Move back to the start of the array
  seek(MacroCell::SeekMarker, primitive::Left, {}, true);
  _dp.set(arrSlot);
  resetSeekMarker();
  popPtr();
}

void Assembler::assignIntegerSlot(Slot const &dest, Slot const &src) {
  assert(types::isInteger(dest.type));
  assert(types::isInteger(src.type));
  if (dest == src) return;

  auto destInt = types::cast<types::IntegerType>(dest.type);
  auto srcInt  = types::cast<types::IntegerType>(src.type);  

  if (destInt->bits() == srcInt->bits()) return assignSlotBytewise(dest, src);

  assert(destInt->bits() > srcInt->bits());
  assert(srcInt->bits() == 8);
  assert(destInt->bits() == 16);
  assert(destInt->signedness() == srcInt->signedness());

  if (not srcInt->isSigned()) return assignSlotBytewise(dest, src);

  // Signed widening
  pushPtr();

  // Copy low byte to both fields of destination
  // TODO: optimize double-copy
  moveTo(src, MacroCell::Value0);
  copyField(Cell{dest, MacroCell::Value0}, Temps<1>::select(dest, MacroCell::Scratch0));
  copyField(Cell{dest, MacroCell::Value1}, Temps<1>::select(dest, MacroCell::Scratch0));

  // If src.low byte >= 128, set dest.high byte to 0xff, otherwise to zero
  // The high byte in dest now contains a copy of its low-byte. Apply
  // destructive less < 128 on it and subtract 1:
  // 0 -> low byte >= 128 -> -1 = 0xff
  // 1 -> low byte < 128 -> 0

  // TODO: replace by getSignBit
  moveTo(dest, MacroCell::Scratch0);
  setToValue(128, Temps<1>::select(dest, MacroCell::Scratch1));
  moveTo(dest, MacroCell::Value1);
  lessDestructive(Cell{dest, MacroCell::Scratch0},
		  Temps<2>::select(dest, MacroCell::Scratch1,
				   dest, MacroCell::Payload0));
  dec();

  popPtr();
}

void Assembler::assignSlot(Slot const &dest, Slot const &src) {
  if (dest == src) return;
  if (types::isInteger(dest.type) && types::isInteger(src.type))
    return assignIntegerSlot(dest, src);

  return assignSlotBytewise(dest, src);
}


void Assembler::assignSlotBytewise(Slot const &dest, Slot const &src) {
  assert(dest.size() >= src.size());
  if (dest == src) return;

  // Direct copy all of the cells
  pushPtr();  
  for (int i = 0; i != dest.size(); ++i) {
    moveTo(src + i, MacroCell::Value0);
    copyField(Cell{dest + i, MacroCell::Value0},
	      Temps<1>::select(dest + i, MacroCell::Scratch0));
    moveTo(src + i, MacroCell::Value1);
    if (src.type->usesValue1()) {
      copyField(Cell{dest + i, MacroCell::Value1},
		Temps<1>::select(dest + i, MacroCell::Scratch0));
    }
    else {
      setToValue(0);
    }
  }
  popPtr();

}

void Assembler::assignSlot(Slot const &slot, literal::Literal const &val) {
  pushPtr();
  if (types::isInteger(slot.type)) {
    int const x = literal::cast<types::IntegerType>(val)->encodedValue();
    moveTo(slot, MacroCell::Value0);
    setToValue(x & 0xff, Temps<1>::select(slot, MacroCell::Scratch0));
    moveTo(slot, MacroCell::Value1);    
    if (slot.type->usesValue1()) {
      setToValue((x >> 8) & 0xff, Temps<1>::select(slot, MacroCell::Scratch0));
    }
    else {
      zeroCell();
    }
  }
  else if (types::isArray(slot.type) || types::isString(slot.type)) {
    // recursive call for each element
    auto const arrayType = types::cast<types::ArrayLike>(val->type());
    types::TypeHandle const elementType = arrayType->elementType();
    for (int i = 0; i != arrayType->length(); ++i) {
      size_t  const elementOffset = i * elementType->size();
      Slot    const elementSlot   = slot.sub(elementType, elementOffset);
      literal::Literal const elementVal    = literal::cast<types::ArrayLike>(val)->element(i);

      assert(elementSlot.type == elementVal->type());
      assignSlot(elementSlot, elementVal);
    }
  }
  else if (types::isStruct(slot.type)) {
    // recursive call for each field	
    auto const structType = types::cast<types::StructType>(val->type());
    for (int i = 0; i != structType->fieldCount(); ++i) {
      types::TypeHandle const fieldType   = structType->fieldType(i);
      size_t            const fieldOffset = structType->fieldOffset(i);
      Slot              const fieldSlot   = slot.sub(fieldType, fieldOffset);
      literal::Literal  const fieldVal    = literal::cast<types::StructType>(val)->field(i);

      assert(fieldType == fieldVal->type());
      assignSlot(fieldSlot, fieldVal);
    }
  }
  else if (types::isFunctionPointer(slot.type)) {
    std::string const &functionName = literal::cast<types::FunctionPointerType>(val)->functionName();

    moveTo(slot, MacroCell::Value0); zeroCell();
    emit<primitive::ChangeBy>([functionName](primitive::Context const &ctx) -> int {
      return ctx.getBlockIndex(functionName) & 0xff;
    });

    moveTo(slot, MacroCell::Value1); zeroCell();
    emit<primitive::ChangeBy>([functionName](primitive::Context const &ctx) -> int {
      return (ctx.getBlockIndex(functionName) >> 8) & 0xff;
    });
  }
  else {
    assert(false && "not implemented");
  }
  popPtr();
}


Expression Assembler::assignImpl(Expression const &lhs, Expression const &rhs, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_ASSIGNABLE(lhs.type(), rhs.type());
  assert(not lhs.isLiteral());

  SlotProxy const dest = lhs.slot();
  if (rhs.hasSlot()) _cache.write(dest, rhs.slot());
  else _cache.write(dest, rhs.literal());
  return lhs;
}


void Assembler::moveToPointee(Slot const &ptrSlot) {
  assert(types::isPointer(ptrSlot.type));
  // NOTE: this leaves the pointer in an unknown position. Leave a seekmarker before calling this
  
  // Decompose the pointer into its frameDepth and offset
  Cell const frameDepth { ptrSlot + RuntimePointer::FrameDepth, MacroCell::Value0 };
  Cell const offsetLow  { ptrSlot + RuntimePointer::Offset, MacroCell::Value0 };
  Cell const offsetHigh { ptrSlot + RuntimePointer::Offset, MacroCell::Value1 };

  // Payload cells start at the origin. First, the pointer-fields
  Cell const frameDepthPayload { 0 + RuntimePointer::FrameDepth, MacroCell::Payload0 };
  Cell const offsetLowPayload  { 0 + RuntimePointer::Offset, MacroCell::Payload0 };
  Cell const offsetHighPayload { 0 + RuntimePointer::Offset, MacroCell::Payload1 };
  
  // Copy pointer (frameDepth and offset) to the payload-cells of cell 0 and 1
  moveTo(frameDepth);
  copyField(frameDepthPayload, Temps<1>::select(frameDepthPayload, MacroCell::Scratch0));

  moveTo(offsetLow);
  copyField(offsetLowPayload,  Temps<1>::select(offsetLowPayload, MacroCell::Scratch0));

  moveTo(offsetHigh);
  copyField(offsetHighPayload, Temps<1>::select(offsetHighPayload, MacroCell::Scratch0));


  // If frameDepth is nonzero, we need to keep moving to the 
  // previous frame start until the depth-counter becomes 0.
  moveTo(frameDepthPayload);
  loopOpen(); {
    Payload payload{
      1, Payload::Width::Single, // depth
      1, Payload::Width::Double, // offset
    };
    moveToPreviousFrame(payload);
    
    // We're now at the start of the previous frame -> exit if depth == 0 after subtracting 1
    moveTo(frameDepthPayload);
    dec();
  } loopClose();

  // At the target frame -> move to offset indicated by pointer value in payload
  goToDynamicOffset(offsetLowPayload, offsetHighPayload);
}


void Assembler::writeSlotThroughDereferencedPointer(Slot const &ptrSlot, Slot const &srcSlot) {
  assert(types::isPointer(ptrSlot.type));
  assert(srcSlot.type == types::cast<types::PointerType>(ptrSlot.type)->pointeeType());

  pushPtr();
  // Leave a marker at the end of the current frame to guarantee that the
  // pointee is to our left.

  int const endOfFrame = _currentFunction->frame.totalLogicalCells();
  moveTo(endOfFrame);
  setSeekMarker();
  moveToPointee(ptrSlot);

  // Set the marker and move back to the source (guaranteed to the right)
  setSeekMarker();
  seek(MacroCell::SeekMarker, primitive::Right, {}, false);
  _dp.set(endOfFrame);
  
  // Copy contents of the source-slot into the payload at the end of the frame
  for (int i = 0; i != srcSlot.size(); ++i) {
    moveTo(srcSlot + i, MacroCell::Value0);
    copyField(Cell{endOfFrame + i, MacroCell::Payload0},
	      Temps<1>::select(endOfFrame + i, MacroCell::Scratch0));
    if (srcSlot.type->usesValue1()) {
      moveTo(srcSlot + i, MacroCell::Value1);
      copyField(Cell{endOfFrame + i, MacroCell::Payload1},
		Temps<1>::select(endOfFrame + i, MacroCell::Scratch0));
    }
  }

  // Seek back to the pointee's slot
  moveTo(endOfFrame);
  Payload payload{
    srcSlot.size(),
    srcSlot.type->usesValue1() ? Payload::Width::Double : Payload::Width::Single
  };
  
  seek(MacroCell::SeekMarker, primitive::Left, payload, false);
  resetSeekMarker();
  _dp.set(0);
  
  // Move contents of the payload in the slot
  for (int i = 0; i != srcSlot.size(); ++i) {
    moveTo(i, MacroCell::Payload0);
    moveField(Cell{i, MacroCell::Value0});
    if (srcSlot.type->usesValue1()) {
      moveTo(i, MacroCell::Payload1);
      moveField(Cell{i, MacroCell::Value1});
    }
  }

  // Seek back to the source
  seek(MacroCell::SeekMarker, primitive::Right, {}, false);
  resetSeekMarker();
  _dp.set(endOfFrame);
  popPtr();
}


void Assembler::writeConstThroughDereferencedPointer(Slot const &ptrSlot, literal::Literal const value) {
  assert(types::isPointer(ptrSlot.type));
  assert(value->type() == types::cast<types::PointerType>(ptrSlot.type)->pointeeType());

  pushPtr();
  
  int const endOfFrame = _currentFunction->frame.totalLogicalCells();
  moveTo(endOfFrame);
  setSeekMarker();
  moveToPointee(ptrSlot);

  _dp.set(0);
  Slot const pointeeSlot {
    .type = value->type(),
    .kind = Slot::Kind::Dummy,
    .offset = 0
  };
  assignSlot(pointeeSlot, value);
  
  // Seek back to the source
  seek(MacroCell::SeekMarker, primitive::Right, {}, false);
  resetSeekMarker();
  _dp.set(endOfFrame);
  popPtr();
}


void Assembler::dereferencePointerIntoSlot(Slot const &ptrSlot, Slot const &derefSlot) {
  assert(types::isPointer(ptrSlot.type));
  assert(derefSlot.type == types::cast<types::PointerType>(ptrSlot.type)->pointeeType());

  pushPtr();

  // Leave a pointer at the derefSlot and move to the pointee
  moveTo(derefSlot);
  setSeekMarker();
  moveToPointee(ptrSlot);
  _dp.set(0);

  // Copy the value into the payload
  pushPtr();
  for (int i = 0; i != derefSlot.size(); ++i) {
    moveTo(i, MacroCell::Value0);
    copyField(Cell{i, MacroCell::Payload0}, Temps<1>::select(i, MacroCell::Scratch0));
    if (derefSlot.type->usesValue1()) {
      moveTo(i, MacroCell::Value1);    
      copyField(Cell{i, MacroCell::Payload1}, Temps<1>::select(i, MacroCell::Scratch0));
    }
  }
  popPtr();
  
  // Seek back to the start of the frame, then to the seekmarker left behind
  // at the deref-slot. We can't seek to the seekmarker directly because
  // we might have ended up to the right of it, when we're still in the target frame.

  Payload payload {
    derefSlot.size(),
    derefSlot.type->usesValue1() ? Payload::Width::Double : Payload::Width::Single
  };

  seek(MacroCell::FrameMarker, primitive::Left, payload, false);
  seek(MacroCell::SeekMarker, primitive::Right, payload, false);
  resetSeekMarker();
  
  // We're now at the marker that marks the deref-slot -> need to rebase.
  _dp.set(derefSlot);

  // Move payload into value-cells
  for (int i = 0; i != derefSlot.size(); ++i) {
    moveTo(derefSlot + i, MacroCell::Payload0);
    moveField(Cell{derefSlot + i, MacroCell::Value0});
    if (derefSlot.type->usesValue1()) {
      moveTo(derefSlot + i, MacroCell::Payload1);    
      moveField(Cell{derefSlot + i, MacroCell::Value1});
    } else {
      moveTo(derefSlot + i, MacroCell::Value1);
      zeroCell();
    }
  }
  
  popPtr();
}  


