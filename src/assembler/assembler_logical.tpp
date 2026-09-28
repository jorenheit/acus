template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::boolDestructive(W const &op) {
  constexpr size_t scratch = W::template ScratchOffset<1>;
  loop(op[0], [&]{
    zeroCell(op[0]);
    inc(op[scratch]);
  });

  addDestructive(SingleCell{op[0]}, SingleCell{op[scratch]});
  return op;
}

template <ws::DoubleCell W>
Assembler::Bool16Result<W> Assembler::bool16Destructive(W const &op) {
  orDestructive(SingleCell{op[0]}, SingleCell{op[1]});
  return op.template transformed<
    ws::Replace<0, ws::Data<>>,
    ws::Replace<1, ws::Data<0>>
  >();
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::notDestructive(W const &op) {
  Cell const x = op[0];
  Cell const tmp = op[W::template ScratchOffset<1>];

  inc(tmp);
  loop(x, [&]{
    dec(tmp);
    zeroCell(x);
  });

  // x = 0, tmp = not(x)
  addDestructive(SingleCell{x}, tmp);
  return op;
}

template <ws::DoubleCell W>
Assembler::Bool16Result<W> Assembler::not16Destructive(W const &op) {
  auto result = bool16Destructive(op);
  notDestructive(result.template transformed<ws::Replace<1, ws::Scratch>>());
  return result;
}

template <ws::SingleCell W>
Assembler::Data8Result<W> Assembler::orDestructive(W const &lhs, SingleCell const &rhs) {

  Cell const x = lhs[0];
  Cell const y = rhs[0];
  
  loop(x, [&]{
    zeroCell(x);
    setToValue(y, 1);
  });

  loop(y, [&]{
    zeroCell(y);
    inc(x);
  });

  return lhs;
}

template <ws::DoubleCell W>
Assembler::Bool16Result<W> Assembler::or16Destructive(W const &lhs, DoubleCell const &rhs) {
  auto result = bool16Destructive(lhs);
  auto rhsBool = bool16Destructive(rhs);
  orDestructive(SingleCell{result[0]}, SingleCell{rhsBool[0]});
  return result;
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::andDestructive(W const &lhs, SingleCell const &rhs) {
  Cell const x = lhs[0];
  Cell const y = rhs[0];
  Cell const tmp = lhs[W::template ScratchOffset<1>];

  addConst(tmp, 2);  // tmp is scratch -> known 0
  loop(x, [&]{
    zeroCell(x);
    dec(tmp);
  });
  inc(x);

  loop(y, [&]{
    zeroCell(y);
    dec(tmp);
  });

  loop(tmp, [&]{
    zeroCell(tmp);
    dec(x);
  });
  
  return lhs;
}

template <ws::DoubleCell W>
Assembler::Bool16Result<W> Assembler::and16Destructive(W const &lhs, DoubleCell const &rhs) {
  auto result = bool16Destructive(lhs);
  auto rhsBool = bool16Destructive(rhs);
  andDestructive(
    result.template transformed<ws::Replace<1, ws::Scratch>>(),
    SingleCell{rhsBool[0]}
  );
  return result;
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::xorDestructive(W const &lhs, SingleCell const &rhs) {
  Cell const x = lhs[0];
  Cell const y = rhs[0];
  Cell const tmp = lhs[W::template ScratchOffset<1>];

  loop(x, [&]{
    zeroCell(x);
    dec(tmp);
  });

  loop(y, [&]{
    zeroCell(y);
    inc(tmp);
  });

  loop(tmp, [&]{
    inc(tmp); // in case tmp == 255
    zeroCell(tmp);
    inc(x);
  });

  return lhs;
}

template <ws::DoubleCell W>
Assembler::Bool16Result<W> Assembler::xor16Destructive(W const &lhs, DoubleCell const &rhs) {
  auto result = bool16Destructive(lhs);
  auto rhsBool = bool16Destructive(rhs);
  xorDestructive(
    result.template transformed<ws::Replace<1, ws::Scratch>>(),
    SingleCell{rhsBool[0]}
  );
  return result;
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::nandDestructive(W const &lhs, SingleCell const &rhs) {

  Cell const x = lhs[0];
  Cell const y = rhs[0];
  Cell const tmp = lhs[W::template ScratchOffset<1>];

  addConst(tmp, 2); // tmp is scratch -> guaranteed 0
  loop(x, [&]{
    zeroCell(x);
    dec(tmp);
  });

  loop(y, [&]{
    zeroCell(y);
    dec(tmp);
  });

  loop(tmp, [&]{
    zeroCell(tmp);
    inc(x);
  });
  
  return lhs;
}

template <ws::DoubleCell W>
Assembler::Bool16Result<W> Assembler::nand16Destructive(W const &lhs, DoubleCell const &rhs) {
  auto result = bool16Destructive(lhs);
  auto rhsBool = bool16Destructive(rhs);
  nandDestructive(
    result.template transformed<ws::Replace<1, ws::Scratch>>(),
    SingleCell{rhsBool[0]}
  );
  return result;
}

template <ws::SingleCell W>
Assembler::Data8Result<W> Assembler::norDestructive(W const &lhs, SingleCell const &rhs) {

  Cell const x = lhs[0];
  Cell const y = rhs[0];

  loop(x, [&]{
    zeroCell(x);
    setToValue(y, 1);
  });
  inc(x);

  loop(y, [&]{
    zeroCell(y);
    dec(x);
  });

  return lhs;
}

template <ws::DoubleCell W>
Assembler::Bool16Result<W> Assembler::nor16Destructive(W const &lhs, DoubleCell const &rhs) {
  auto result  = bool16Destructive(lhs);
  auto rhsBool = bool16Destructive(rhs);
  norDestructive(SingleCell{result[0]}, SingleCell{rhsBool[0]});
  return result;
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::xnorDestructive(W const &lhs, SingleCell const &rhs) {
  auto result = xorDestructive(lhs, rhs);
  notDestructive(result);
  return result;
}

template <ws::DoubleCell W>
Assembler::Bool16Result<W> Assembler::xnor16Destructive(W const &lhs, DoubleCell const &rhs) {
  auto result = xor16Destructive(lhs, rhs);
  notDestructive(result.template transformed<ws::Replace<1, ws::Scratch>>());
  return result;
}
