// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

Expression Assembler::addressOfImpl(Expression obj, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  assert(not obj.isLiteral());
  
  return Expression(obj.slot().addressOf(*this, API_FWD));
}

Slot Assembler::addressOfSlot(Slot pointeeSlot, API_CTX) {
  API_REQUIRE(pointeeSlot.kind() != Slot::Temp,
	      error::ErrorCode::TakingAddressOfTemporary,
	      "Tried to take the address of a temporary or literal value.");
  assert(pointeeSlot.kind() != Slot::Cache && "taking address of cache");

  types::TypeHandle const pointeeType = pointeeSlot.type();
  types::TypeHandle const pointerType = ts::pointer(pointeeType);

  // Set frame-depth to 0 for a local pointer, FrameID for a global pointer
  Slot const ptrSlot = getTemp(pointerType);
  if (pointeeSlot.kind() == SlotData::Kind::Global) {
    moveTo(0, MacroCell::FrameMarker);
    copyField(Cell{ptrSlot + RuntimePointer::FrameDepth, MacroCell::Value0},
	      Temps<1>::select(ptrSlot + RuntimePointer::FrameDepth, MacroCell::Scratch0));
  } else {
    moveTo(ptrSlot + RuntimePointer::FrameDepth, MacroCell::Value0);
    zeroCell();
  }

  // Construct offset in second cell
  int const offset = pointeeSlot.offset();  
  moveTo(ptrSlot + RuntimePointer::Offset, MacroCell::Value0);
  setToValue(offset & 0xff, Temps<1>::select(ptrSlot + RuntimePointer::Offset, MacroCell::Scratch0));
  moveTo(ptrSlot + RuntimePointer::Offset, MacroCell::Value1);
  setToValue((offset >> 8) & 0xff, Temps<1>::select(ptrSlot + RuntimePointer::Offset, MacroCell::Scratch0));

  return ptrSlot;
}

void Assembler::copyElementIntoSlot(Slot elementSlot, Slot arrSlot, Slot indexSlot, TransferMode mode) {
  // TransferMode::Move is valid only when arrSlot is part of a source object
  // being consumed as a whole, e.g. a temp/cache slot being discarded.
  // It destructively extracts the selected element in this mode.
  
  assert(types::isArrayLike(arrSlot.type()));
  assert(types::isInteger(indexSlot.type()));
  assert(elementSlot.type() == types::cast<types::ArrayLike>(arrSlot.type())->elementType());
  types::TypeHandle elementType = elementSlot.type();

  
  pushPtr();

  // Prepare offset-payload (index * sizeof(T))
  auto [scaledIndexSlot, freeScaledIndexSlot] = [&] -> std::pair<Slot, bool> {
    if (indexSlot.type()->usesValue1() && elementType->size() == 1) {
      // Index can be used as-is
      return {indexSlot, false};
    }
    
    Slot scaled = getTemp(ts::u16());
    assignSlot(scaled, indexSlot);
    mulSlotByConst(scaled, elementType->size());
    return {scaled, true};
  }();

  Payload payload(elementType->size(),
		  elementType->usesValue1() ? Payload::Width::Double : Payload::Width::Single);

  // Mark Start of array
  moveTo(arrSlot, MacroCell::Value0);
  setSeekMarker();

  // Fetch data
  fetchFromDynamicOffset(Cell{scaledIndexSlot, MacroCell::Value0},
			 Cell{scaledIndexSlot, MacroCell::Value1},
			 payload,
			 primitive::Left,
			 mode);

  // Move payload into element
  for (int i = 0; i != elementType->size(); ++i) {
    moveTo(arrSlot + i, MacroCell::Payload0);
    moveField(Cell{elementSlot + i, MacroCell::Value0});
    if (elementType->usesValue1()) {
      moveTo(arrSlot + i, MacroCell::Payload1);
      moveField(Cell{elementSlot + i, MacroCell::Value1});
    }
  }

  // Return to start of array
  moveTo(arrSlot);
  resetSeekMarker();
  if (freeScaledIndexSlot) freeTempSlot(scaledIndexSlot);
  popPtr();

}

void Assembler::copySlotIntoElement(Slot srcSlot, Slot arrSlot, Slot indexSlot, TransferMode mode) {
  assert(types::isArrayLike(arrSlot.type()));
  assert(types::isInteger(indexSlot.type()));

  types::TypeHandle elementType = types::cast<types::ArrayLike>(arrSlot.type())->elementType();
  assert(srcSlot.type() == elementType);

  pushPtr();

  // Prepare offset-payload (index * sizeof(T))
  auto [scaledIndexSlot, freeScaledIndexSlot] = [&] -> std::pair<Slot, bool> {
    if (indexSlot.type()->usesValue1() && elementType->size() == 1) {
      // Index can be used as-is
      return {indexSlot, false};
    }
    
    Slot scaled = getTemp(ts::u16());
    assignSlot(scaled, indexSlot);
    mulSlotByConst(scaled, elementType->size());
    return {scaled, true};
  }();
  
  // Plant a seek marker at the start of the array
  moveTo(arrSlot, MacroCell::Value0);
  setSeekMarker();

  // Plant another marker one (full element) beyond the start of the element we need
  moveToDynamicOffset(Cell{scaledIndexSlot, MacroCell::Value0},
		      Cell{scaledIndexSlot, MacroCell::Value1});

  _dp.set(0);
  moveTo(elementType->size()); 
  setSeekMarker();
  if (freeScaledIndexSlot) freeTempSlot(scaledIndexSlot);
  moveTo(0);

  // Move back to the start of the array
  seek(MacroCell::SeekMarker, primitive::Left, {}, true);
  _dp.set(arrSlot);

  for (int i = 0; i != elementType->size(); ++i) {
    // Copy the contents into the payload cells
    moveTo(srcSlot + i, MacroCell::Value0);
    copyOrMoveField(mode, Cell{arrSlot + i, MacroCell::Payload0},
		    Temps<1>::select(arrSlot + i, MacroCell::Scratch0));
    if (elementType->usesValue1()) {
      moveTo(srcSlot + i, MacroCell::Value1);
      copyOrMoveField(mode, Cell{arrSlot + i, MacroCell::Payload1},
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

void Assembler::copyConstIntoElement(literal::Literal const value, Slot arrSlot, Slot indexSlot) {
  assert(types::isArrayLike(arrSlot.type()));
  assert(types::isInteger(indexSlot.type()));

  types::TypeHandle elementType = types::cast<types::ArrayLike>(arrSlot.type())->elementType();
  assert(value.type() == elementType);

  pushPtr();

  // Prepare offset-payload (index * sizeof(T))
  auto [scaledIndexSlot, freeScaledIndexSlot] = [&] -> std::pair<Slot, bool> {
    if (indexSlot.type()->usesValue1() && elementType->size() == 1) {
      // Index can be used as-is
      return {indexSlot, false};
    }
    
    Slot scaled = getTemp(ts::u16());
    assignSlot(scaled, indexSlot);
    mulSlotByConst(scaled, elementType->size());
    return {scaled, true};
  }();
  
  // Plant a seek marker at the start of the array
  moveTo(arrSlot, MacroCell::Value0);
  setSeekMarker();

  // Move to the element-slot
  moveToDynamicOffset(Cell{scaledIndexSlot, MacroCell::Value0},
		      Cell{scaledIndexSlot, MacroCell::Value1});

  // Rebase the datapointer and use assign the constant value to the slot
  _dp.set(0);
  auto const elementSlot = Slot {
    SlotData { 
      .type = elementType,
      .kind = SlotData::Kind::Dummy,
      .offset = 0
    }
  };
  assignSlot(elementSlot, value);

  // Move back to the start of the array
  seek(MacroCell::SeekMarker, primitive::Left, {}, true);
  _dp.set(arrSlot);
  resetSeekMarker();
  if (freeScaledIndexSlot) freeTempSlot(scaledIndexSlot);

  popPtr();
}

void Assembler::assignIntegerSlot(Slot dest, Slot src, TransferMode mode) {
  assert(types::isInteger(dest.type()));
  assert(types::isInteger(src.type()));
  assert(dest != src);

  auto destInt = types::cast<types::IntegerType>(dest.type());
  auto srcInt  = types::cast<types::IntegerType>(src.type());  

  if (destInt->bits() == srcInt->bits()) return assignSlotBytewise(dest, src, mode);

  assert(destInt->bits() > srcInt->bits());
  assert(srcInt->bits() == 8);
  assert(destInt->bits() == 16);
  assert(destInt->signedness() == srcInt->signedness());

  if (not srcInt->isSigned()) return assignSlotBytewise(dest, src, mode);

  // Signed widening
  pushPtr();

  // Copy low byte to both fields of destination
  // TODO: optimize double-copy
  moveTo(src, MacroCell::Value0);
  copyField(Cell{dest, MacroCell::Value0}, Temps<1>::select(dest, MacroCell::Scratch0));
  copyOrMoveField(mode, Cell{dest, MacroCell::Value1}, Temps<1>::select(dest, MacroCell::Scratch0));
  
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

void Assembler::assignSlot(Slot dest, Slot src, TransferMode mode) {
  if (dest == src) return;
  
  if (types::isInteger(dest.type()) && types::isInteger(src.type()))
    return assignIntegerSlot(dest, src, mode);

  return assignSlotBytewise(dest, src, mode);
}


void Assembler::assignSlotBytewise(Slot dest, Slot src, TransferMode mode) {
  assert(dest != src);
  assert(dest.size() >= src.size());

  // Direct copy all of the cells
  pushPtr();

  for (int i = 0; i != src.size(); ++i) {
    moveTo(src + i, MacroCell::Value0);
    copyOrMoveField(mode, Cell{dest + i, MacroCell::Value0},
		    Temps<1>::select(dest + i, MacroCell::Scratch0));
    moveTo(src + i, MacroCell::Value1);
    if (src.type()->usesValue1()) {
      copyOrMoveField(mode, Cell{dest + i, MacroCell::Value1},
		      Temps<1>::select(dest + i, MacroCell::Scratch0));
    }
    else {
      moveTo(dest + i, MacroCell::Value1);
      setToValue(0);
    }
  }
  popPtr();

}

void Assembler::assignSlot(Slot slot, literal::Literal val) {
  pushPtr();
  if (types::isInteger(slot.type())) {
    int const x = literal::cast<types::IntegerType>(val)->encodedValue();
    moveTo(slot, MacroCell::Value0);
    setToValue(x & 0xff, Temps<1>::select(slot, MacroCell::Scratch0));
    moveTo(slot, MacroCell::Value1);    
    if (slot.type()->usesValue1()) {
      setToValue((x >> 8) & 0xff, Temps<1>::select(slot, MacroCell::Scratch0));
    }
    else {
      zeroCell();
    }
  }
  else if (types::isArray(slot.type()) || types::isString(slot.type())) {
    // recursive call for each element
    auto const arrayType = types::cast<types::ArrayLike>(val.type());
    types::TypeHandle const elementType = arrayType->elementType();
    for (int i = 0; i != arrayType->length(); ++i) {
      size_t  const elementOffset = i * elementType->size();
      Slot    const elementSlot   = slot.sub(elementType, elementOffset);
      literal::Literal const elementVal    = literal::cast<types::ArrayLike>(val)->element(i);

      assert(elementSlot.type() == elementVal.type());
      assignSlot(elementSlot, elementVal);
    }
  }
  else if (types::isStruct(slot.type())) {
    // recursive call for each field	
    auto const structType = types::cast<types::StructType>(val.type());
    for (int i = 0; i != structType->fieldCount(); ++i) {
      types::TypeHandle const fieldType   = structType->fieldType(i);
      size_t            const fieldOffset = structType->fieldOffset(i);
      Slot              const fieldSlot   = slot.sub(fieldType, fieldOffset);
      literal::Literal  const fieldVal    = literal::cast<types::StructType>(val)->field(i);

      assert(fieldType == fieldVal.type());
      assignSlot(fieldSlot, fieldVal);
    }
  }
  else if (types::isFunctionPointer(slot.type())) {
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


Expression Assembler::assignImpl(Expression lhs, Expression rhs, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  API_REQUIRE_ASSIGNABLE(lhs.type(), rhs.type());
  assert(not lhs.isLiteral());

  
  SlotProxy const dest = lhs.slot();
  if (rhs.hasSlot()) {

    TransferMode mode = [&] {
      if (not rhs.slot().direct()) return TransferMode::Copy;
      if (materialize(rhs.slot()).kind() != Slot::Temp) return TransferMode::Copy;
      return TransferMode::Move;
    }();
    
    _cache.write(dest, rhs.slot(), mode);
  }
  else _cache.write(dest, rhs.literal());
  return lhs;
}


void Assembler::moveToPointee(Slot ptrSlot) {
  assert(types::isPointer(ptrSlot.type()));
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
  moveToDynamicOffset(offsetLowPayload, offsetHighPayload);
}


void Assembler::writeSlotThroughDereferencedPointer(Slot ptrSlot, Slot srcSlot, TransferMode mode) {
  assert(types::isPointer(ptrSlot.type()));
  assert(srcSlot.type() == types::cast<types::PointerType>(ptrSlot.type())->pointeeType());

  // First resolve the requested Copy/Move into a private temporary. Pointer
  // fields in that temporary can then be rebased destructively without ever
  // modifying a copied source value. The temporary is always consumed by the
  // actual write below.
  Slot const tmp = getTemp(srcSlot.type());
  Slot const frameDepth = getTemp(ts::u8());

  pushPtr();

  // Preserve the destination depth before the source is moved. This matters
  // when ptrSlot aliases storage contained in srcSlot.
  moveTo(ptrSlot + RuntimePointer::FrameDepth, MacroCell::Value0);
  copyField(Cell{frameDepth, MacroCell::Value0},
            Temps<1>::select(frameDepth, MacroCell::Scratch0));

  // Leave a marker at the end of the current frame to guarantee that the
  // pointee is to our left.
  int const endOfFrame = _currentFunction->frame.totalLogicalCells();
  moveTo(endOfFrame);
  setSeekMarker();
  moveToPointee(ptrSlot);

  // Set the marker and move back to the source (guaranteed to the right).
  // From this point on ptrSlot is no longer needed to locate the destination.
  setSeekMarker();
  seek(MacroCell::SeekMarker, primitive::Right, {}, false);
  _dp.set(endOfFrame);

  // Resolve the requested transfer mode into the temporary. A copied source
  // remains intact; a moved source is consumed here. From now on the temporary
  // can always be treated destructively.
  assignSlot(tmp, srcSlot, mode);

  // The value will be stored 'frameDepth' frames closer to its pointees. Rebase
  // every runtime pointer contained in the temporary before transporting it.
  // Underflow represents undefined behavior: a forward stack pointer cannot be
  // represented by RuntimePointer.
  rebasePointersToOlderFrame(tmp, Cell{frameDepth, MacroCell::Value0});

  // Move the already-rebased temporary into the payload at the end of the
  // current frame. No pointer-specific metadata needs to travel with it.
  for (int i = 0; i != tmp.size(); ++i) {
    moveTo(tmp + i, MacroCell::Value0);
    moveField(Cell{endOfFrame + i, MacroCell::Payload0});
    if (tmp.type()->usesValue1()) {
      moveTo(tmp + i, MacroCell::Value1);
      moveField(Cell{endOfFrame + i, MacroCell::Payload1});
    }
  }

  // Seek back to the pointee's slot, carrying only the value itself.
  moveTo(endOfFrame);
  Payload payload{
    tmp.size(),
    tmp.type()->usesValue1() ? Payload::Width::Double : Payload::Width::Single
  };

  seek(MacroCell::SeekMarker, primitive::Left, payload, false);
  resetSeekMarker();
  _dp.set(0);

  // Move the payload into the pointee slot. The value is already expressed
  // relative to this older frame, so this is now an ordinary bytewise move.
  for (int i = 0; i != tmp.size(); ++i) {
    moveTo(i, MacroCell::Payload0);
    moveField(Cell{i, MacroCell::Value0});
    if (tmp.type()->usesValue1()) {
      moveTo(i, MacroCell::Payload1);
      moveField(Cell{i, MacroCell::Value1});
    }
  }

  // Seek back to the source frame.
  seek(MacroCell::SeekMarker, primitive::Right, {}, false);
  resetSeekMarker();
  _dp.set(endOfFrame);

  popPtr();
  freeTempSlot(frameDepth);
  freeTempSlot(tmp);
}

void Assembler::writeConstThroughDereferencedPointer(Slot ptrSlot, literal::Literal const value) {
  assert(types::isPointer(ptrSlot.type()));
  assert(value.type() == types::cast<types::PointerType>(ptrSlot.type())->pointeeType());

  pushPtr();
  
  int const endOfFrame = _currentFunction->frame.totalLogicalCells();
  moveTo(endOfFrame);
  setSeekMarker();
  moveToPointee(ptrSlot);

  _dp.set(0);
  auto const pointeeSlot = Slot {
    SlotData {
      .type = value.type(),
      .kind = SlotData::Kind::Dummy,
      .offset = 0
    }
  };
  assignSlot(pointeeSlot, value);
  
  // Seek back to the source
  seek(MacroCell::SeekMarker, primitive::Right, {}, false);
  resetSeekMarker();
  _dp.set(endOfFrame);
  popPtr();
}

void Assembler::rebasePointers(Slot slot, Cell depthDiff, auto &&rebase) {
  switch (slot.type()->tag()) {
  case types::POINTER: {
    Cell const currentDepth  { slot + RuntimePointer::FrameDepth, MacroCell::Value0 };
    Cell const depthDiffCopy { slot + RuntimePointer::FrameDepth, MacroCell::Scratch0 };

    // Make a disposable copy of the depth difference before rebasing, which consumes its operand.    
    moveTo(depthDiff);
    copyField(depthDiffCopy,
              Temps<1>::select(slot + RuntimePointer::FrameDepth, MacroCell::Scratch1));
    moveTo(currentDepth);
    rebase(depthDiffCopy);
    break;
  }

  case types::ARRAY: {
    auto const arrayType = types::cast<types::ArrayType>(slot.type());
    auto const elementType = arrayType->elementType();
    for (int i = 0; i != arrayType->length(); ++i) {
      rebasePointers(slot.sub(elementType, i * elementType->size()), depthDiff, rebase);
    }
    break;
  }

  case types::STRUCT: {
    auto const structType = types::cast<types::StructType>(slot.type());
    for (int i = 0; i != structType->fieldCount(); ++i) {
      auto const fieldType = structType->fieldType(i);
      rebasePointers(slot.sub(fieldType, structType->fieldOffset(i)), depthDiff, rebase);
    }
    break;
  }

  default:
    // No pointers contained in this slot
    break;
  }  
}

void Assembler::rebasePointersToCurrentFrame(Slot slot, Cell depthToAdd) {
  // Moving a pointer value into a newer frame increases its distance
  // to the pointee by the number of crossed frames.
  rebasePointers(slot, depthToAdd, [&](Cell diff) {
    addDestructive(diff);
  });
}

void Assembler::rebasePointersToOlderFrame(Slot slot, Cell depthToSub) {
  // Moving a pointer value into an older frame decreases its distance
  // to the pointee by the number of crossed frames.
  rebasePointers(slot, depthToSub, [&](Cell diff) {
    subDestructive(diff);
  });
}


void Assembler::dereferencePointerIntoSlot(Slot ptrSlot, Slot derefSlot) {
  assert(types::isPointer(ptrSlot.type()));
  assert(derefSlot.type() == types::cast<types::PointerType>(ptrSlot.type())->pointeeType());

  pushPtr();

  // Leave a pointer at the derefSlot and move to the pointee
  moveTo(derefSlot);
  setSeekMarker();
  moveToPointee(ptrSlot);
  _dp.set(0);

  // Copy the value into the payload (never move)
  pushPtr();
  for (int i = 0; i != derefSlot.size(); ++i) {
    moveTo(i, MacroCell::Value0);
    copyField(Cell{i, MacroCell::Payload0}, Temps<1>::select(i, MacroCell::Scratch0));
    if (derefSlot.type()->usesValue1()) {
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
    derefSlot.type()->usesValue1() ? Payload::Width::Double : Payload::Width::Single
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
    if (derefSlot.type()->usesValue1()) {
      moveTo(derefSlot + i, MacroCell::Payload1);    
      moveField(Cell{derefSlot + i, MacroCell::Value1});
    } else {
      moveTo(derefSlot + i, MacroCell::Value1);
      zeroCell();
    }
  }

  // The copied value used to live ptrSlot.FrameDepth frames behind this one.
  // Any runtime pointers stored inside it are therefore that many frames farther
  // from their pointees now that the value has been materialized here. These
  // pointers must be rebased before they can be dereferenced properly.
  rebasePointersToCurrentFrame(
    derefSlot,
    Cell{ptrSlot + RuntimePointer::FrameDepth, MacroCell::Value0}
  );
  
  popPtr();
}  


