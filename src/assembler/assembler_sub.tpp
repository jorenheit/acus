
template <ws::SingleCell W>
Assembler::Data8Result<W> Assembler::subConst(W const &lhs, int delta) {
  return addConst(lhs, -delta);
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::subConst(W const &lhs, int delta) {
  return addConst(lhs, -delta);
}

template <ws::Sub16Operand W>
Assembler::Data16Result<W> Assembler::sub16Const(W const &lhs, int delta) {
  if (delta == 0) return lhs;
  if (delta < 0) return add16Const(lhs, -delta);

  Slot const tmpSlot = getTemp(ts::raw(1));
  auto const tmp = ws::promiseClean16(tmpSlot);
  setToValue16(tmp, delta);
  sub16Destructive(lhs, DoubleCell{tmp});
  freeTempSlot(tmpSlot);
  return lhs;
}

template <ws::SingleCell W>
Assembler::Data8Result<W> Assembler::subDestructive(W const &lhs, SingleCell const &delta) {
  Cell const x = lhs[0];
  Cell const y = delta[0];

  loop(y, [&] {
    dec(y);
    dec(x);
  });

  return lhs;
}

template <ws::Sub16Operand W>
Assembler::Data16Result<W> Assembler::sub16Destructive(W const &lhs, DoubleCell const &delta) {
  loop(delta[0], [&] {
    dec(delta[0]);
    dec16(lhs);
  });

  subDestructive(SingleCell{lhs[1]}, SingleCell{delta[1]});
  return lhs;
}
