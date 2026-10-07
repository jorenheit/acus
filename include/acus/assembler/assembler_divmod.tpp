
template <ws::DivModNum N>
Assembler::DivModResult<N> Assembler::divModDestructive(N const &num, SingleCell const &denom, TransferMode rhsMode) {
  auto v = num.view("N", "D", "CopyTemp", "", "", "", "ZeroFlag");

  // Bring the denominator into this workspace.
  if constexpr (N::template knownZero<1>()) {
    copyOrMoveFieldToZero(rhsMode, denom, v["D"], v["CopyTemp"], true);
  } else {
    copyOrMoveField(rhsMode, denom, v["D"], v["CopyTemp"], true);
  }

  // Reuse the CopyTemp field for DTest and pick a new CopyTemp field
  v.rename("CopyTemp", "DTest");
  v.template rename<3>("CopyTemp");
  copyFieldToZero(v["D"], v["DTest"], v["CopyTemp"], true);

  // Assume denominator == 0.
  inc(v["ZeroFlag"]);
  loop(v["DTest"], [&] {
    // If D != 0
    zeroCell(v["DTest"]);
    dec(v["ZeroFlag"]);

    auto prepared = ws::promise(v["N"], ws::Layout<ws::Prepared<ws::Role::NumeratorLow>,
                                ws::Prepared<ws::Role::DenominatorLow>,
                                ws::ScratchCells<5>>{});

    v = divModPreparedDestructive(prepared)
        .view("Q", "R", ws::At<6>{"ZeroFlag"});
  });

  loop(v["ZeroFlag"], [&] {
    // Else D == 0
    dec(v["ZeroFlag"]);

    // quotient = 0xff, remainder = 0
    zeroCell(v["Q"]); dec(v["Q"]);
    zeroCell(v["R"]);
  });

  return num.template transformed<
    ws::Replace<0, ws::Prepared<ws::Role::QuotientLow>>,
    ws::Replace<1, ws::Prepared<ws::Role::RemainderLow>>
    >();
}

template <ws::DivModPrepared W>
Assembler::DivModResult<W> Assembler::divModPreparedDestructive(W const &prep) {
  auto const [N, D, R, Q, zero1, zero2] = prep.template cells<6>();

  loop(N, [&]{
    inc(R);
    dec(D);
    literalBf(D, "[>>>]>[[<+>-]>+>>]<<<<");
    dec(N);
  });

  // Workspace:
  // N |   D   | R | Q | zero1 | zero2
  // 0 | d - r | r | q |   0   |   0

  moveFieldToZero(Q, N);
  moveField(R, D);
  
  return prep.template transformed<
    ws::Replace<0, ws::Prepared<ws::Role::QuotientLow>>,
    ws::Replace<1, ws::Prepared<ws::Role::RemainderLow>>
    >();
}

template <ws::DivMod16Num N, ws::DivMod16Den D>
Assembler::DivMod16DigitResult<N> Assembler::divMod16Digit(N const &num, D const &den) {

  using Dec17Operand = ws::Workspace<
    ws::DataCells<3>,
    ws::Scratch,
    ws::Untouched,
    ws::ScratchCells<2>
    >;
  
  auto const dec17 = [&](Dec17Operand const &op) -> Dec17Operand {
    auto const [low, high, guard, sentinel, _, highBorrow, lowBorrow] = op.cells();
    
    // Start by assuming that both lower bytes will borrow.
    // These flags are cleared below when that assumption proves false.
    inc(highBorrow);
    inc(lowBorrow);

    // Speculatively propagate borrow through high into guard.
    dec(guard);
    literalBf(high, "[>+>]"    // high != 0: cancel guard borrow
                    ">>[-<<]"  // clear highBorrow and sync pointer on sentinel
                    "<<-");    // return from sentinel to high and decrement that

    // If low != 0, no borrow was necessary at all: restore high and,
    // if necessary, guard.
    literalBf(low, "[>+>>>>[-<<<+>>>]<<]" // low != 0: restore high; if highBorrow, restore guard
                   ">>>[-<<<]"            // clear lowBorrow if set and synchronize on sentinel
                   "<<<-");               // return from sentinel to low and decrement that

    zeroCell(highBorrow);
    zeroCell(lowBorrow);
    return op;
  };

  [[maybe_unused]] auto const [Rlo, Rhi, G, scratch, Qinitial, Qfinal] = num.template cells<6>();
  auto const [Dlo, Dhi, DloCopy, DhiCopy, CopyTemp] = den.template cells<5>();

  // Preserve the denominator. The subtraction loop consumes Dlo/Dhi
  // on every iteration, so these copies are restored afterwards.
  copyFieldToZero(Dlo, DloCopy, DhiCopy, true);
  copyFieldToZero(Dhi, DhiCopy, CopyTemp, true);

  // Q starts at -1 because the loop performs one subtraction too many.
  dec(Qinitial);

  // Extra 17th remainder bit.
  inc(G);
  loop(G, [&] {
    // 17-bit subtraction: Rlo:Rhi:G -= Dlo:Dhi
    loop(Dlo, [&] {
      dec(Dlo);
      dec17(ws::promise(Rlo, ws::Layout<ws::DataCells<3>, ws::Scratch,  ws::Untouched, // Qinitial
                        ws::ScratchCells<2>>{}));
    });

    loop(Dhi, [&] {
      dec(Dhi);
      dec16(ws::promise(Rhi,
                        ws::Layout< ws::DataCells<2>, ws::Scratch, ws::Untouched, // Qinitial
                        ws::ScratchCells<2>>{}));
    });

    // Restore denominator for the next subtraction.
    copyFieldToZero(DloCopy, Dlo, CopyTemp, true);
    copyFieldToZero(DhiCopy, Dhi, CopyTemp, true);

    inc(Qinitial);
  });

  // Copies are no longer necessary. Dlo/Dhi themselves are currently
  // restored and will be consumed by the final add-back.
  zeroCell(DloCopy);
  zeroCell(DhiCopy);

  // Move Q from cell 4 to its final position at cell 5.
  moveFieldToZero(Qinitial, Qfinal);
  
  auto currentRemainder =
    ws::promise(Rlo, ws::Layout<ws::Prepared<ws::Role::RemainderLow>,
                ws::Prepared<ws::Role::RemainderHigh>,
                ws::ScratchCells<3>,
                ws::Untouched // Q
                >{});

  // The subtraction loop deliberately overshot by one denominator,
  // so add D back to the remainder.  
  add16Destructive(currentRemainder, den);

  return num.template transformed<
    ws::Replace<0, ws::Prepared<ws::Role::RemainderLow>>,
    ws::Replace<1, ws::Prepared<ws::Role::RemainderHigh>>,
    ws::Replace<5, ws::Prepared<ws::Role::QuotientLow>>
    >();
}


template <ws::DivMod16Num N, ws::DivMod16Den D>
Assembler::DivMod16Result<N> Assembler::divMod16Destructive(N const &num, D const &den) {
  Slot const tmpSlot = getTemp(ts::raw(1), allocHint(num[0], den[0]));
  auto const tmp = ws::promiseClean16(tmpSlot);

  auto nv = num.view("Nlo", "Nhi", "CopyTemp");
  auto dv = den.view("Dlo", "Dhi", "CopyTemp");
  auto tv = tmp.view("DloCopy", "DhiCopy", "ElseFlag");

  copyField(dv["Dlo"], tv["DloCopy"], dv["CopyTemp"], true);
  copyField(dv["Dhi"], tv["DhiCopy"], dv["CopyTemp"], true);

  inc(tv["ElseFlag"]);
  loop(tv["DhiCopy"], [&]{
    zeroCell(tv["DhiCopy"]);
    dec(tv["ElseFlag"]);

    nv = divMod16Digit(num, den)
         .view("Rlo", "Rhi", ws::At<5>{"Qlo"});

    // Move remainder to cells 2 and 3 and quotient to cell 0
    moveFieldToZero(nv["Rlo"], nv[2]);
    moveFieldToZero(nv["Rhi"], nv[3]);
    moveFieldToZero(nv["Qlo"], nv[0]);

    nv.reset(); // Reset to initial name-state for the else-branch
  });

  loop(tv["ElseFlag"], [&] {

    loop(tv["DloCopy"], [&]{
      // If Dlo != 0

      zeroCell(tv["DloCopy"]);
      zeroCell(tv["ElseFlag"]);

      // Prepare tmp for 8-bit division Nhi / Dlo
      // All tmp cells have already been cleared at this point
      tv.renameAll("Nhi", "", "");
      moveFieldToZero(nv["Nhi"], tv["Nhi"]);

      // Calulate Nhi / Dlo
      tv = divModDestructive(ws::promiseClean8(tv["Nhi"]), dv["Dlo"], TransferMode::Copy)
           .view("Qhi", "Carry");

      // The Qhi that was returned by the 8-bit algorithm is already final -> move into final position
      moveFieldToZero(tv["Qhi"], nv[1]);

      // Move Nlo into the tmp workspace to prepare for the calculation of
      // (Nlo:Carry) / Dlo
      tv.renameAll("Nlo", "Carry");
      moveFieldToZero(nv["Nlo"], tv["Nlo"]);
      tv = divMod16Digit(ws::promiseClean16(tv["Nlo"]), ws::promiseClean16(dv["Dlo"]))
           .view("Rlo", "Rhi", ws::At<5>{"Qlo"});

      nv.renameAll("Qlo", "Qhi", "Rlo", "Rhi");
      moveFieldToZero(tv["Rlo"], nv["Rlo"]);
      moveFieldToZero(tv["Rhi"], nv["Rhi"]);
      moveFieldToZero(tv["Qlo"], nv["Qlo"]);

      // tv[2] is left empty and becomes the ElseFlag again
      tv.renameAll("", "", "ElseFlag");
    });

    loop(tv["ElseFlag"], [&] {
      // Else Dlo == 0
      zeroCell(tv["ElseFlag"]);

      // quotient = 0xffff
      zeroCell(nv["Qlo"]); dec(nv["Qlo"]);
      zeroCell(nv["Qhi"]); dec(nv["Qhi"]);
    });
  });

  freeSlot(tmpSlot);

  return num.template transformed<
    ws::Replace<0, ws::Prepared<ws::Role::QuotientLow>>,
    ws::Replace<1, ws::Prepared<ws::Role::QuotientHigh>>,
    ws::Replace<2, ws::Prepared<ws::Role::RemainderLow>>,
    ws::Replace<3, ws::Prepared<ws::Role::RemainderHigh>>
    >();
}

// TODO: factor this stuff (maybe)

template <ws::HalfOperand1 W> // consecutive
auto Assembler::halfDestructive(W const &lhs) {
  auto const [x, remaining, zero, sync] = lhs.template cells<4>();
  if constexpr (W::template knownZero<1>()) moveFieldToZero(x, remaining);
  else moveField(x, remaining);
  inc(sync);
  loop(remaining, [&]{
    literalBf(remaining, "-[<+>->]>[<]<");
  });
  dec(sync);

  return lhs.template transformed<ws::Replace<0, ws::Data<>>, ws::Replace<1, ws::Data<0>>>();
}

template <ws::HalfOperand2 W> // gapped
Assembler::Data8Result<W>  Assembler::halfDestructive(W const &lhs) {
  auto const [x, _, remaining, zero, sync] = lhs.template cells<5>();
  
  moveFieldToZero(x, remaining);
  inc(sync);
  loop(remaining, [&]{
    literalBf(remaining, "-[<<+>>->]>[<]<");
  });
  dec(sync);

  return lhs;
}


template <ws::HalfWithParityOperand1 W> // consecutive
auto Assembler::halfWithParityDestructive(W const &lhs) {
  auto const [x, remaining, zero, sync, parity] = lhs.template cells<5>();
  if constexpr (W::template knownZero<1>()) moveFieldToZero(x, remaining);
  else moveField(x, remaining);
  inc(sync);
  loop(remaining, [&]{
    inc(parity);
    literalBf(remaining, "-[<+>->>>-<<]>[<]<");
  });
  dec(sync);

  return lhs.template transformed<
    ws::Replace<0, ws::Data<>>,
    ws::Replace<1, ws::Data<0>>,
    ws::Replace<4, ws::Prepared<ws::Role::ParityBit>>
    >();
}

template <ws::HalfWithParityOperand2 W> // gapped
Assembler::HalfWithParityResult<W, 5>  Assembler::halfWithParityDestructive(W const &lhs) {
  auto const [x, _, remaining, zero, sync, parity] = lhs.template cells<6>();

  moveFieldToZero(x, remaining);
  inc(sync);
  loop(remaining, [&]{
    inc(parity);
    literalBf(remaining, "-[<<+>>->>>-<<]>[<]<");
  });
  dec(sync);

  return lhs.template transformed<
    ws::Replace<5, ws::Prepared<ws::Role::ParityBit>>
    >();
}

template <ws::Half16Operand W>
Assembler::Data16Result<W> Assembler::half16Destructive(W const &lhs) {
  auto const [low, high] = lhs.template cells<2>();

  // Calculate high/2, store parity bit
  auto highResult = halfWithParityDestructive(lhs.template subset<1>());
  // Calculate low/2, don't touch previous results (high/2 and parity bit)
  static constexpr size_t ParityBitIndex = highResult.template indexOfRole<ws::Role::ParityBit>() + 1;
  halfDestructive(lhs.template transformed<ws::Replace<1, ws::Untouched>,
                  ws::Replace<ParityBitIndex, ws::Untouched>
                  >());

  // If parity bit set, add 128 to the result of low/2
  Cell const highParity = highResult.template cell<ws::Role::ParityBit>();
  loop(highParity, [&]{
    dec(highParity);
    addConst(low, 128);
  });

  return lhs;
}

template <ws::Half16WithParityOperand W>
Assembler::Half16WithParityResult<W> Assembler::half16WithParityDestructive(W const &lhs) {
  auto const [low, high, _1, _2, _3, parity, parityHigh] = lhs.template cells<7>();

  // Calculate high/2, store parity bit
  halfWithParityDestructive(lhs.template subset<1>());
  moveFieldToZero(parity, parityHigh);
  // Calculate low/2, this parity bit will be returned at index 5
  halfWithParityDestructive(lhs.template transformed<ws::Replace<1, ws::Untouched>,
                            ws::Replace<6, ws::Untouched>
                            >()); // make sure high/2 result and parity are not touched

  // If parity-high bit set, add 128 to the result of low/2
  loop(parityHigh, [&]{
    dec(parityHigh);
    addConst(low, 128);
  });

  return lhs.template transformed<ws::Replace<5, ws::Prepared<ws::Role::ParityBit>>>();
}

template <ws::DivByPowerOfTwoOperand W>
Assembler::Data8Result<W> Assembler::divByPowerOfTwoDestructive(W const &lhs, size_t p) {
  assert(p > 1);
  
  if (p >= 8) {
    zeroCell(lhs[0]);
    return lhs;
  }

  static constexpr size_t powCellIndex = 5; // Cell 0 through 4 are used by the halving algorithm
  Cell const powCell = lhs.template cell<powCellIndex>();
  setToValue(powCell, p); 
  auto const halvingWorkspace = lhs.template transformed<
    ws::Replace<powCellIndex, ws::Untouched>
  >();

  loop(powCell, [&]{
    dec(powCell);
    halfDestructive(halvingWorkspace);
  });
  
  return lhs;
}


template <ws::DivByPowerOfTwo16Operand W>
Assembler::Data16Result<W> Assembler::divByPowerOfTwo16Destructive(W const &lhs, size_t p) {
  assert(p > 1);
  
  if (p >= 16) {
    zeroCell(lhs[0]);
    zeroCell(lhs[1]);
    return lhs;
  }

  static constexpr size_t powCellIndex = 6; // Cell 0 through 5 are used by the halving algorithm
  Cell const powCell = lhs.template cell<powCellIndex>();
  setToValue(powCell, p); 
  auto const halvingWorkspace = lhs.template transformed<
    ws::Replace<powCellIndex, ws::Untouched>
  >();

  loop(powCell, [&]{
    dec(powCell);
    half16Destructive(halvingWorkspace);
  });
  
  return lhs;
}
template <ws::ModByPowerOfTwoOperand W>
Assembler::Data8Result<W> Assembler::modByPowerOfTwoDestructive(W const &lhs, size_t p) {
  assert(p > 1);

  auto const [x, _1, _2, _3, _4, pow, rem] = lhs.template cells<7>();

  if (p >= 8) return lhs;

  auto const halvingWorkspace = lhs.template transformed<
    ws::Replace<5, ws::Untouched>,
    ws::Replace<6, ws::Untouched>
  >();

  for (size_t i = 0; i < p; ++i) {
    auto const result = halfWithParityDestructive(halvingWorkspace);
    Cell const parity = result.template cell<ws::Role::ParityBit>();
    loop(parity, [&] {
      dec(parity);
      addConst(rem, 1u << i);
    });
  }
  moveField(rem, x);
  return lhs;
}


template <ws::ModByPowerOfTwo16Operand W>
Assembler::Data16Result<W> Assembler::modByPowerOfTwo16Destructive(W const &lhs, size_t p) {
  assert(p > 1);
  if (p >= 16) return lhs;

  Slot const remainder = getTemp(ts::u16(), allocHint(lhs[0]));
  setSlotToValue(remainder, 0);

  Cell const remLow  = {remainder, MacroCell::Value0};
  Cell const remHigh = {remainder, MacroCell::Value1};

  for (size_t i = 0; i < p; ++i) {
    auto const result = half16WithParityDestructive(lhs);
    Cell const parity = result.template cell<ws::Role::ParityBit>();

    loop(parity, [&] {
      dec(parity);
      if (i < 8)
        addConst(remLow, 1u << i);
      else
        addConst(remHigh, 1u << (i - 8));
    });
  }

  moveField(remLow, lhs[0]);
  if (p >= 8) moveFieldToZero(remHigh, lhs[1]);
  else moveField(remHigh, lhs[1]);
  freeTempSlot(remainder);

  return lhs;
}

