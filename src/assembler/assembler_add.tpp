
// Simple version, no scratch required.
template <ws::SingleCell W>
Assembler::Data8Result<W> Assembler::addConst(W const &lhs, int delta) {
  addConst(lhs[0], delta);
  return lhs;
}

// Scratch provided -> use it to select the compact constant-change algorithm.
template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::addConst(W const &lhs, int delta) {
  constexpr size_t scratch = W::template ScratchOffset<1>;
  addConst(lhs[0], lhs[scratch], delta);
  return lhs;
}

template <ws::Add16Operand W>
Assembler::Data16Result<W> Assembler::add16Const(W const &lhs, int delta) {
  if (delta == 0) return lhs;
  if (delta < 0) return sub16Const(lhs, -delta);

  Slot const tmpSlot = getTemp(ts::raw(1));
  auto const tmp = ws::promiseClean16(tmpSlot);
  setToValue16(tmp, delta);
  add16Destructive(lhs, DoubleCell{tmp});
  freeTempSlot(tmpSlot);
  return lhs;
}

template <ws::SingleCell W>
Assembler::Data8Result<W> Assembler::addDestructive(W const &lhs, SingleCell const &delta) {
  Cell const x = lhs[0];
  Cell const y = delta[0];

  loop(y, [&] {
    dec(y);
    inc(x);
  });

  return lhs;
}

template <ws::Add16Operand W>
Assembler::Data16Result<W> Assembler::add16Destructive(W const &lhs, DoubleCell const &delta) {
  loop(delta[0], [&] {
    dec(delta[0]);
    inc16(lhs);
  });

  addDestructive(SingleCell{lhs[1]}, SingleCell{delta[1]});
  return lhs;
}
