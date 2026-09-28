template <ws::SingleCell W>
Assembler::Data8Result<W> Assembler::eqDestructive(W const &lhs, SingleCell const &rhs) {
  Cell const x = lhs[0];
  Cell const y = rhs[0];

  loop(x, [&] {
    dec(x);
    dec(y);
  });
  inc(x);

  loop(y, [&] {
    zeroCell(y);
    dec(x);
  });

  return lhs;
}

template <ws::DoubleAndScratch L>
Assembler::Bool16Result<L> Assembler::eq16Destructive(L const &lhs, DoubleCell const &rhs) {
  eqDestructive(SingleCell{lhs[0]}, SingleCell{rhs[0]});
  eqDestructive(SingleCell{lhs[1]}, SingleCell{rhs[1]});

  // lhs now contains lowEqual, highEqual, ... Scratch.
  // The high-byte result is the destructive RHS of AND and must not be
  // borrowed as scratch by the lhs workspace.
  andDestructive(
    lhs.template transformed<ws::Replace<1, ws::Untouched>>(),
    SingleCell{lhs[1]}
  );

  return lhs.template transformed<
    ws::Replace<0, ws::Data<>>,
    ws::Replace<1, ws::Data<0>>
  >();
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::lessDestructive(W const &lhs, SingleCell const &rhs) {
  Cell const x = lhs[0];
  Cell const y = rhs[0];
  Cell const tmp = lhs[W::template ScratchOffset<1>];

  // Decrement x and y in lockstep. If y is still non-zero when x reaches
  // zero, x < y. tmp temporarily holds y while forcing the inner loop to run
  // only once per x decrement.
  loop(x, [&] {
    dec(x);
    loop(y, [&] {
      dec(y);
      moveField(y, tmp);
    });
    moveField(tmp, y);
  });

  loop(y, [&] {
    zeroCell(y);
    inc(x);
  });

  return lhs;
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::greaterDestructive(W const &lhs, SingleCell const &rhs) {
  Cell const x = lhs[0];
  Cell const y = rhs[0];
  Cell const tmp = lhs[W::template ScratchOffset<1>];

  // Decrement y while parking the remainder of x in tmp. If x has anything
  // left when y reaches zero, x > y. The result is zero or non-zero; callers
  // do not require it to be normalized to exactly one.
  loop(y, [&] {
    dec(y);

    // Restore the remainder from the previous iteration. x is known zero
    // here after the first iteration; on the first iteration tmp is zero.
    loop(tmp, [&] {
      dec(tmp);
      inc(x);
    });

    loop(x, [&] {
      dec(x);
      moveField(x, tmp);
    });
  });

  loop(tmp, [&] {
    zeroCell(tmp);
    inc(x);
  });

  return lhs;
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::lessOrEqualDestructive(W const &lhs, SingleCell const &rhs) {
  auto result = greaterDestructive(lhs, rhs);
  notDestructive(result);
  return result;
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::greaterOrEqualDestructive(W const &lhs, SingleCell const &rhs) {
  auto result = lessDestructive(lhs, rhs);
  notDestructive(result);
  return result;
}

// 16-bit ordering comparisons use two lhs scratch cells to keep copies of the
// high bytes and one rhs scratch cell as copy/comparison scratch. The low-byte
// result is combined with the high-byte relation and the high result cell is
// left as known-zero data.
template <ws::Compare16Lhs L, ws::DoubleAndScratch R>
Assembler::Bool16Result<L> Assembler::less16Destructive(L const &lhs, R const &rhs) {
  auto const [xLow, xHigh, xHighCopy, tmp] = lhs.template cells<4>();
  auto const [yLow, yHigh] = rhs.template cells<2>();
  Cell const yHighCopy = rhs[R::template ScratchOffset<2>];

  copyField(yHigh, yHighCopy, tmp);
  copyField(xHigh, xHighCopy, tmp);

  lessDestructive(
    ws::promise(xHigh, ws::Layout<ws::Data<>, ws::Data<>, ws::Scratch>{}),
    SingleCell{yHigh}
  );
  eqDestructive(SingleCell{xHighCopy}, SingleCell{yHighCopy});

  lessDestructive(
    ws::promise(xLow, ws::Layout<ws::Data<>, ws::Data<>, ws::Data<>, ws::Scratch>{}),
    SingleCell{yLow}
  );

  andDestructive(
    ws::promise(xHighCopy, ws::Layout<ws::Data<>, ws::Scratch>{}),
    SingleCell{xLow}
  );
  orDestructive(SingleCell{xHighCopy}, SingleCell{xHigh});
  moveField(xHighCopy, xLow);

  return lhs.template transformed<
    ws::Replace<0, ws::Data<>>,
    ws::Replace<1, ws::Data<0>>
  >();
}

template <ws::Compare16Lhs L, ws::DoubleAndScratch R>
Assembler::Bool16Result<L> Assembler::greater16Destructive(L const &lhs, R const &rhs) {

  auto const [xLow, xHigh, xHighCopy, tmp] = lhs.template cells<4>();
  auto const [yLow, yHigh] = rhs.template cells<2>();
  auto const yHighCopy = rhs[R::template ScratchOffset<2>];

  copyField(yHigh, yHighCopy, tmp);
  copyField(xHigh, xHighCopy, tmp);

  greaterDestructive(
    ws::promise(xHigh, ws::Layout<ws::Data<>, ws::Data<>, ws::Scratch>{}),
    SingleCell{yHigh}
  );
  eqDestructive(SingleCell{xHighCopy}, SingleCell{yHighCopy});

  greaterDestructive(
    ws::promise(xLow, ws::Layout<ws::Data<>, ws::Data<>, ws::Data<>, ws::Scratch>{}),
    SingleCell{yLow}
  );

  andDestructive(
    ws::promise(xHighCopy, ws::Layout<ws::Data<>, ws::Scratch>{}),
    SingleCell{xLow}
  );
  orDestructive(SingleCell{xHighCopy}, SingleCell{xHigh});
  moveField(xHighCopy, xLow);

  return lhs.template transformed<
    ws::Replace<0, ws::Data<>>,
    ws::Replace<1, ws::Data<0>>
  >();
}

template <ws::Compare16Lhs L, ws::DoubleAndScratch R>
Assembler::Bool16Result<L> Assembler::lessOrEqual16Destructive(L const &lhs, R const &rhs) {
  auto result = greater16Destructive(lhs, rhs);
  notDestructive(result);
  return result;
}

template <ws::Compare16Lhs L, ws::DoubleAndScratch R>
Assembler::Bool16Result<L> Assembler::greaterOrEqual16Destructive(L const &lhs, R const &rhs) {
  auto result = less16Destructive(lhs, rhs);
  notDestructive(result);
  return result;
}
