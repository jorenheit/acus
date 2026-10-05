template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::boolDestructive(W const &op) {
  constexpr size_t scratch = W::template ScratchOffset<1>;
  boolDestructive(op[0], op[scratch]);
  return op;
}

template <ws::Bool16Operand W>
Assembler::Bool16Result<W> Assembler::bool16Destructive(W const &op) {
  orDestructive(op[0], op[1]);
  return op.template transformed<
    ws::Replace<0, ws::Data<>>,
    ws::Replace<1, ws::Data<0>>
  >();
}

template <ws::SingleAndScratch W>
Assembler::Data8Result<W> Assembler::notDestructive(W const &op) {
  Cell const x = op[0];
  Cell const tmp = op[W::template ScratchOffset<1>];
  notDestructive(x, tmp);
  return op;
}

template <ws::DoubleCell W>
Assembler::Bool16Result<W> Assembler::not16Destructive(W const &op) {
  auto result = bool16Destructive(op);
  notDestructive(result[0], result[1]);
  return result;
}

template <ws::BinaryLogic16Operand L, ws::BinaryLogic16Operand R>
Assembler::Bool16Result<L> Assembler::logic16Destructive(L const &lhs, R const &rhs, auto &&logicOp) {
  auto lhsBool = bool16Destructive(lhs);
  auto rhsBool = bool16Destructive(rhs);
  return logicOp(lhsBool, rhsBool);
}

#define LOGIC_8_IMPL(op)                                                \
  template <ws::BinaryLogic8Operand W>                                  \
  Assembler::Data8Result<W> Assembler::op##Destructive(W const &lhs, SingleCell const &rhs) { \
    auto const [x, y, tmp] = lhs.template cells<3>();                   \
    if constexpr (W::template knownZero<1>()) moveFieldToZero(rhs[0], y); \
    else moveField(rhs[0], y);                                         \
    op##Destructive(x, y, tmp);                                         \
    return lhs.template transformed<ws::Replace<1, ws::Data<0>>>();     \
  }

#define LOGIC_16_IMPL(op)                                               \
  template <ws::BinaryLogic16Operand L, ws::BinaryLogic16Operand R>     \
  Assembler::Bool16Result<L> Assembler::op##16Destructive(L const &lhs, R const &rhs) { \
    return logic16Destructive(lhs, rhs, [&](auto const &lhsBool, auto const &rhsBool) { \
      return op##Destructive(lhsBool, rhsBool);                         \
    });                                                                 \
  }

LOGIC_8_IMPL(and);
LOGIC_8_IMPL(nand);
LOGIC_8_IMPL(or);
LOGIC_8_IMPL(nor);
LOGIC_8_IMPL(xor);
LOGIC_8_IMPL(xnor);


LOGIC_16_IMPL(and);
LOGIC_16_IMPL(nand);
LOGIC_16_IMPL(or);
LOGIC_16_IMPL(nor);
LOGIC_16_IMPL(xor);
LOGIC_16_IMPL(xnor);

#undef LOGIC_8_IMPL
#undef LOGIC_16_IMPL
