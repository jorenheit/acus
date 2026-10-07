
template <ws::SingleCell W>
Assembler::Data8Result<W> Assembler::inc(W const &op, size_t n) {
  pushPtr();
  moveTo(op[0]);
  for (size_t i = 0; i != n % 256; ++i) {
    emit<primitive::ChangeBy>(1);
  }
  popPtr();
  return op;
}

template <ws::Inc16Operand W>
Assembler::Data16Result<W> Assembler::inc16(W const &op) {
  auto const [L, H, S1, S2, S3] = op.template cells<5>();
  (void)S1;
  (void)S2;

  inc(S3);
  inc(H);
  inc(L);
  literalBf(L, "[>->]>>[<<]<<");
  dec(S3);

  return op;
}

template <ws::SingleCell W>
Assembler::Data8Result<W> Assembler::dec(W const &op, size_t n) {
  pushPtr();
  moveTo(op[0]);
  for (size_t i = 0; i != n % 256; ++i) {
    emit<primitive::ChangeBy>(-1);
  }
  popPtr();
  return op;
}

template <ws::Dec16Operand W>
Assembler::Data16Result<W> Assembler::dec16(W const &op) {
  auto const [L, H, S1, S2, S3] = op.template cells<5>();
  (void)S1;
  (void)S2;

  inc(S3);
  dec(H);
  literalBf(L, "[>+>]>>[<<]<<");
  dec(S3);
  dec(L);

  return op;
}

template <ws::SingleCell W>
Assembler::Data8Result<W> Assembler::setToValue(W const &target, int value) {
  pushPtr();
  moveTo(target[0]);
  if constexpr (!W::template knownZero<0>()) zeroCell();
  emit<primitive::ChangeBy>(value & 0xff);
  popPtr();
  return target;
}

template <ws::DoubleCell W>
Assembler::Data16Result<W> Assembler::setToValue16(W const &target, int value) {
  setToValue(target.template subset<0, 0>(), value & 0xff);
  setToValue(target.template subset<1, 1>(), (value >> 8) & 0xff);
  return target;
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::setToValue(W const &target, int value) {
  constexpr size_t scratch = W::template ScratchOffset<1>;
  pushPtr();
  moveTo(target[0]);
  if constexpr (W::template knownZero<0>())
    emit<primitive::ChangeBy>(value & 0xff, 0, scratch);
  else
    emit<primitive::ConstructConstant>(value & 0xff, 0, scratch);
  popPtr();
  return target;
}

template <ws::DoubleAndScratch W>
Assembler::Data16Result<W> Assembler::setToValue16(W const &target, int value) {
  constexpr size_t scratch = W::template ScratchOffset<2>;

  pushPtr();
  moveTo(target[0]);
  if constexpr (W::template knownZero<0>())
    emit<primitive::ChangeBy>(value & 0xff, 0, scratch);
  else
    emit<primitive::ConstructConstant>(value & 0xff, 0, scratch);
  moveTo(target[1]);
  if constexpr (W::template knownZero<1>())
    emit<primitive::ChangeBy>((value >> 8) & 0xff, 0, scratch - 1);
  else
    emit<primitive::ConstructConstant>((value >> 8) & 0xff, 0, scratch - 1);
  popPtr();

  return target;
}


template <ws::DoubleCell Offset, ws::DynamicMoveWorkspace Work>
void Assembler::moveToDynamicOffset(Offset const &offset, Work const &work, TransferMode mode) {

  auto const [high, low, S1, S2, S3] = work.template cells<5>();
  copyOrMoveFieldToZero(mode, offset[0], low, high, true);
  copyOrMoveFieldToZero(mode, offset[1], high, S1, true);

  std::string const STEP_L = std::string(MacroCell::FieldCount, '<');
  std::string const STEP_R = std::string(MacroCell::FieldCount, '>');
  std::string const MOVE_R = "[-" + STEP_R + "+" + STEP_L + "]";

  literalBf(S3, "+[<<<[->>]<[->->]>>[<+>[-<<<" + MOVE_R + ">>]" + STEP_R + ">>+<]>]>-");
}
  


template <ws::DoubleCell Offset>
void Assembler::fetchFromDynamicOffset(Offset const &offset, Payload const &payload, primitive::Direction seekDir,
                                       TransferMode dataTransferMode, TransferMode offsetTransferMode) {
  assert(payload);

  int const base = _dp.current().offset;

  
  auto const scratch = ws::promise(Cell{base, MacroCell::Scratch0}, ws::Layout<ws::ScratchCells<5>>{});  
  moveToDynamicOffset(offset, scratch, offsetTransferMode);
  
  // Base is now the cell we arrived at (at offset).
  // Load values into payload
  for (int i = 0; i != payload.size(); ++i) {
    copyOrMoveField(dataTransferMode,
                    Cell{base + i, MacroCell::Value0},
                    Cell{base + i, MacroCell::Payload0},
                    Cell{base + i, MacroCell::Scratch0}, true);
    if (payload.width(i) == Payload::Width::Double) {
      copyOrMoveField(dataTransferMode,
                      Cell{base + i, MacroCell::Value1},
                      Cell{base + i, MacroCell::Payload1},
                      Cell{base + i, MacroCell::Scratch0}, true);
    }
  }
  
  // Bring payload back to cell that contains the SeekMarker
  pushPtr();
  moveTo(base);
  seek(MacroCell::SeekMarker, seekDir, payload, true);
  popPtr();

  // Transfer complete: payload now in Payload-fields of the base
}


/*
 * Daniel Cristofani's dynamic-offset algorithm.
 *
 * The current macrocell has been prepared as follows:
 *
 *   Scratch0  Scratch1  Flag  Payload0  Payload1
 *      high      low      0       0         1
 *                                           ^
 *                                         pointer
 *
 * Scratch0/Scratch1 are deliberately reversed compared with the usual
 * low/high ordering. Starting from Payload1, <<< reaches the low byte
 * (Scratch1), while one additional < reaches the high byte (Scratch0).
 * The same geometry is later used to transport low and high with the
 * same piece of code.
 *
 * The complete algorithm is:
 *
 *   +[<<<[->>]<[->->]>>[<+>[-<<< MOVE >>] STEP >>+<]>]>-
 *
 * where MOVE destructively moves the current counter byte to the
 * corresponding field of the next macrocell, and STEP moves the data
 * pointer itself there.
 *
 * It works as follows:
 *
 * 1. +[
 *    Payload1 is set to 1 and used to control the outer loop. Each
 *    iteration consumes one unit of the 16-bit offset and, unless the
 *    offset has reached zero, advances by one macrocell.
 *
 * 2. <<<[->>]
 *    Move from Payload1 to Scratch1 (low).
 *
 *    If low != 0, decrement it once and move two fields right to
 *    Payload0. The loop then terminates immediately because Payload0 is
 *    zero.
 *
 *    If low == 0, the loop is skipped and the pointer remains at
 *    Scratch1.
 *
 * 3. <[->->]
 *    This performs the borrow when the low byte was zero.
 *
 *    - If low was nonzero, the preceding < moves from Payload0 to Flag,
 *      which is zero, so this loop is skipped.
 *
 *    - If low was zero, < moves from Scratch1 to Scratch0 (high). If
 *      high != 0, high is decremented and low is decremented from 0 to
 *      255. The pointer then ends at Flag.
 *
 *    - If both high and low were zero, this loop is skipped while the
 *      pointer remains at Scratch0.
 *
 * 4. >>
 *    This is also the zero test for the complete 16-bit offset.
 *
 *    After a successful decrement (either low-- or high--/low=255), the
 *    pointer was at Flag and therefore arrives at Payload1, which is 1.
 *
 *    If high == low == 0, the pointer was at Scratch0 and therefore
 *    arrives at Flag, which is 0.
 *
 *    Consequently, the following loop is entered iff there was still
 *    one unit of offset to consume.
 *
 * 5. [<+>[-<<< MOVE >>] STEP >>+<]
 *    Move the remaining 16-bit counter to the neighbouring macrocell
 *    and follow it with the data pointer.
 *
 *    <+> sets Payload0 to 1 while Payload1 is already 1. The inner loop
 *    therefore executes twice:
 *
 *      first iteration:
 *        Payload1--, <<< -> Scratch1, MOVE the low byte, >> -> Payload0
 *
 *      second iteration:
 *        Payload0--, <<< -> Scratch0, MOVE the high byte, >> -> Flag
 *
 *    Both control cells have now been cleared and both counter bytes
 *    have been transferred to the neighbouring macrocell.
 *
 *    STEP moves from Flag of the old macrocell to Flag of the new one.
 *    >>+< then sets its Payload1 to 1 and leaves the pointer at
 *    Payload0 (0), causing this inner movement loop to terminate.
 *
 * 6. >
 *    After a move, Payload0 -> Payload1, whose value is 1, so the outer
 *    loop continues in the new macrocell.
 *
 *    If the counter was already zero, the movement loop in step 5 was
 *    skipped while the pointer was at Flag; this > therefore reaches
 *    Payload0 (0), causing the outer loop to terminate instead.
 *
 * 7. >-
 *    On termination the pointer is at Payload0 of the destination
 *    macrocell. Move to Payload1 and clear its remaining 1. All helper
 *    fields are now zero again and the pointer ends at Payload1.
 *
 * A notable feature of this algorithm is that the runtime pointer
 * position itself carries control-flow state: at several points the
 * same relative move has a different meaning depending on which branch
 * was taken. This is why the ordering of the five helper fields is part
 * of the algorithm's required layout.
 */  


template <ws::SignExtendOperand W>
Assembler::SignExtendResult<W> Assembler::signExtend(W const &w, bool const copyLowByte) {
  auto const [lo, hi, tmp] = w.template cells<3>();
  
  // Copy low byte into high byte
  if (copyLowByte) {
    if constexpr (W::template knownZero<1>()) copyFieldToZero(lo, hi, tmp, true);
    else copyField(lo, hi, tmp, true);
  }

  // Construct the signbit in the Value1 field
  auto const [value1, zero, sync] = signBitDestructive(w.template subset<1>()).template cells<3>();
    
  inc(sync);
  literalBf(value1, "[>]>[<<-->]<");
  dec(sync);

  return w.template transformed<
    ws::Replace<1, ws::Data<>>
  >();
}
