
template <ws::DivModNum N>
Assembler::DivModResult<N> Assembler::divModDestructive(N const &num, SingleCell const &denom, TransferMode rhsMode) {
  auto v = num.view("N", "D", "CopyTemp", "", "", "", "ZeroFlag");

  pushPtr();

  // Bring the denominator into this workspace.
  copyOrMoveField(rhsMode, denom, v["D"], v["CopyTemp"]);

  // Reuse the CopyTemp field for DTest and pick a new CopyTemp field
  v.rename("CopyTemp", "DTest");
  v.template rename<3>("CopyTemp");
  copyField(v["D"], v["DTest"], v["CopyTemp"]);

  // Assume denominator == 0.
  inc(v["ZeroFlag"]);
  loop(v["DTest"], [&] {
    // If D != 0
    zeroCell(v["DTest"]);
    dec(v["ZeroFlag"]);

    auto prepared = ws::promise(v["N"], ws::Layout<
				ws::Prepared<ws::Role::NumeratorLow>,
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

  popPtr();

  return num.template transformed<
    ws::Replace<0, ws::Prepared<ws::Role::QuotientLow>>,
    ws::Replace<1, ws::Prepared<ws::Role::RemainderLow>>
  >();
}

template <ws::DivModPrepared W>
Assembler::DivModResult<W> Assembler::divModPreparedDestructive(W const &prep) {

  auto const [N, D, Q, CopyTemp, DCopy, RestoreFlag] = prep.template cells<6>();

  pushPtr();

  // Initial layout:
  //
  // N | D | Q | CopyTemp | DCopy | RestoreFlag
  // n | d | 0 |    0     |   0   |     0

  // Preserve D and initialize the restore flag.
  copyField(D, DCopy, Q);
  inc(RestoreFlag);

  // N | D | Q | CopyTemp | DCopy | RestoreFlag
  // n | d | 0 |    0     |   d   |     1

  loop(N, [&] {
    // Consume one numerator unit and one denominator unit.
    // Q is incremented provisionally; the raw fragment undoes that
    // increment when D has not yet reached zero.
    dec(N);
    inc(Q);
    dec(D);

    // If D is still nonzero, undo the provisional Q increment.
    // Both control paths synchronize back on D.
    literalBf(D,
              "[>->]"    // D != 0: --Q and land on the zero CopyTemp cell
              ">>[-<<]"  // D != 0: clear RestoreFlag and return to CopyTemp
              "<<");     // both paths converge back on D

    // If D reached zero, RestoreFlag is still set.
    // Restore D from its persistent copy.
    loop(RestoreFlag, [&] {
      dec(RestoreFlag);
      copyField(DCopy, D, CopyTemp);
    });

    // Prepare the flag for the next iteration.
    inc(RestoreFlag);
  });

  // The outer loop has finished; this flag is no longer needed.
  dec(RestoreFlag);

  // Current layout:
  //
  // N | D | Q   | CopyTemp | DCopy | RestoreFlag
  // 0 | c | n/d |    0     |   d   |     0
  //
  // remainder = d - c
  subDestructive(SingleCell{DCopy}, SingleCell{D});

  // D and N are now both zero, so place the final results there.
  moveField(DCopy, D); // TODO: known zero move optimization
  moveField(Q, N);

  // Final layout:
  //
  // Q | R | 0 | 0 | 0 | 0
  popPtr();

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
    
    pushPtr();

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
    popPtr();
    return op;
  };

  pushPtr();

  [[maybe_unused]] auto const [Rlo, Rhi, G, scratch, Qinitial, Qfinal] = num.template cells<6>();
  auto const [Dlo, Dhi, DloCopy, DhiCopy, CopyTemp] = den.template cells<5>();

  // Preserve the denominator. The subtraction loop consumes Dlo/Dhi
  // on every iteration, so these copies are restored afterwards.
  copyField(Dlo, DloCopy, DhiCopy);
  copyField(Dhi, DhiCopy, CopyTemp);

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
    copyField(DloCopy, Dlo, CopyTemp);
    copyField(DhiCopy, Dhi, CopyTemp);

    inc(Qinitial);
  });

  // Copies are no longer necessary. Dlo/Dhi themselves are currently
  // restored and will be consumed by the final add-back.
  zeroCell(DloCopy);
  zeroCell(DhiCopy);

  // Move Q from cell 4 to its final position at cell 5.
  moveField(Qinitial, Qfinal);
  
  auto currentRemainder =
    ws::promise(Rlo, ws::Layout<
		ws::Prepared<ws::Role::RemainderLow>,
		ws::Prepared<ws::Role::RemainderHigh>,
		ws::ScratchCells<3>,
		ws::Untouched // Q
		>{});

  // The subtraction loop deliberately overshot by one denominator,
  // so add D back to the remainder.  
  add16Destructive(currentRemainder, den);
  popPtr();


  return num.template transformed<
    ws::Replace<0, ws::Prepared<ws::Role::RemainderLow>>,
    ws::Replace<1, ws::Prepared<ws::Role::RemainderHigh>>,
    ws::Replace<5, ws::Prepared<ws::Role::QuotientLow>>
  >();
}


template <ws::DivMod16Num N, ws::DivMod16Den D>
Assembler::DivMod16Result<N> Assembler::divMod16Destructive(N const &num, D const &den) {
  Slot const tmpSlot = getTemp(ts::raw(1));
  auto const tmp = ws::promiseClean16(tmpSlot);

  pushPtr();

  auto nv = num.view("Nlo", "Nhi", "CopyTemp");
  auto dv = den.view("Dlo", "Dhi", "CopyTemp");
  auto tv = tmp.view("DloCopy", "DhiCopy", "ElseFlag");

  copyField(dv["Dlo"], tv["DloCopy"], dv["CopyTemp"]);
  copyField(dv["Dhi"], tv["DhiCopy"], dv["CopyTemp"]);

  inc(tv["ElseFlag"]);
  loop(tv["DhiCopy"], [&]{
    zeroCell(tv["DhiCopy"]);
    dec(tv["ElseFlag"]);

    nv = divMod16Digit(num, den)
      .view("Rlo", "Rhi", ws::At<5>{"Qlo"});

    // Move remainder to cells 2 and 3 and quotient to cell 0
    moveField(nv["Rlo"], nv[2]);
    moveField(nv["Rhi"], nv[3]);
    moveField(nv["Qlo"], nv[0]);

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
      moveField(nv["Nhi"], tv["Nhi"]);

      // Calulate Nhi / Dlo
      tv = divModDestructive(ws::promiseClean8(tv["Nhi"]), dv["Dlo"], TransferMode::Copy)
	.view("Qhi", "Carry");

      // The Qhi that was returned by the 8-bit algorithm is already final -> move into final position
      moveField(tv["Qhi"], nv[1]);

      // Move Nlo into the tmp workspace to prepare for the calculation of
      // (Nlo:Carry) / Dlo
      tv.renameAll("Nlo", "Carry");
      moveField(nv["Nlo"], tv["Nlo"]);
      tv = divMod16Digit(ws::promiseClean16(tv["Nlo"]), ws::promiseClean16(dv["Dlo"]))
	.view("Rlo", "Rhi", ws::At<5>{"Qlo"});

      nv.renameAll("Qlo", "Qhi", "Rlo", "Rhi");
      moveField(tv["Rlo"], nv["Rlo"]);
      moveField(tv["Rhi"], nv["Rhi"]);
      moveField(tv["Qlo"], nv["Qlo"]);

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

  popPtr();
  freeSlot(tmpSlot);

  return num.template transformed<
    ws::Replace<0, ws::Prepared<ws::Role::QuotientLow>>,
    ws::Replace<1, ws::Prepared<ws::Role::QuotientHigh>>,
    ws::Replace<2, ws::Prepared<ws::Role::RemainderLow>>,
    ws::Replace<3, ws::Prepared<ws::Role::RemainderHigh>>
  >();
}
