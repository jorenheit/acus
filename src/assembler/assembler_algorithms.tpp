
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
  zeroCell();
  emit<primitive::ChangeBy>(value & 0xff);
  popPtr();
  return target;
}

template <ws::DoubleCell W>
Assembler::Data16Result<W> Assembler::setToValue16(W const &target, int value) {
  setToValue(SingleCell{target[0]}, value & 0xff);
  setToValue(SingleCell{target[1]}, (value >> 8) & 0xff);
  return target;
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::setToValue(W const &target, int value) {
  constexpr size_t scratch = W::template ScratchOffset<1>;
  pushPtr();
  moveTo(target[0]);
  emit<primitive::ConstructConstant>(value & 0xff, 0, scratch);
  popPtr();
  return target;
}

template <ws::DoubleAndScratch W>
Assembler::Data16Result<W> Assembler::setToValue16(W const &target, int value) {
  constexpr size_t scratch = W::template ScratchOffset<2>;

  pushPtr();
  moveTo(target[0]);
  emit<primitive::ConstructConstant>(value & 0xff, 0, scratch);
  moveTo(target[1]);
  emit<primitive::ConstructConstant>((value >> 8) & 0xff, 0, scratch - 1);
  popPtr();

  return target;
}
