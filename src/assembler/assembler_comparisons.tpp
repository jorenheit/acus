#define COMPARE_8_IMPL(op)                                              \
  template <ws::Compare8Operand W>                                      \
  Assembler::Data8Result<W> Assembler::op##Destructive(W const &lhs, SingleCell const &rhs) { \
    auto const [x, y, tmp] = lhs.template cells<3>();                   \
    moveField(rhs[0], y);                                               \
    op##Destructive(x, y, tmp);                                         \
    return lhs.template transformed<ws::Replace<1, ws::Data<0>>>();     \
  }


COMPARE_8_IMPL(less);
COMPARE_8_IMPL(greater);
COMPARE_8_IMPL(lessOrEqual);
COMPARE_8_IMPL(greaterOrEqual);

#undef COMPARE_8_IMPL

template <ws::Compare8Operand W>
Assembler::Data8Result<W> Assembler::eqDestructive(W const &lhs, SingleCell const &rhs) { 
  auto const [x, y] = lhs.template cells<2>();
  moveField(rhs[0], y);
  eqDestructive(x, y);                                         
  return lhs.template transformed<ws::Replace<1, ws::Data<0>>>();
}

template <ws::Eq16Operand L>
Assembler::Bool16Result<L> Assembler::eq16Destructive(L const &lhs, DoubleCell const &rhs) {

  auto const [xLow, xHigh] = lhs.template cells<2>();
  auto const [yLow, yHigh] = rhs.template cells<2>();

  auto w = lhs.view("xLow", "yLow", "xHigh", "yHigh");
  moveField(xHigh, w["xHigh"]);
  moveField(yLow, w["yLow"]);
  moveField(yHigh, w["yHigh"]);

  eqDestructive(w["xLow"],  w["yLow"]);
  eqDestructive(w["xHigh"], w["yHigh"]);
  andDestructive(w["xLow"], w["xHigh"], w["yHigh"]);
  return lhs.template transformed<ws::Replace<0, ws::Data<>>,
                                  ws::Replace<1, ws::Data<0>>>();
}
  

template <ws::Compare16Operand L>
Assembler::Bool16Result<L> Assembler::less16Destructive(L const &lhs, DoubleCell const &rhs) {
  auto [xLow, xHigh] = lhs.template cells<2>();
  auto [yLow, yHigh] = rhs.template cells<2>();

  // result = less(x.high, y.high) OR (eq(x.high, y.high) AND less(x.low, y.low))
  // Use lhs as the workspace for all comparisons:
  // Prepare workspace: [xHigh1, yHigh1, 0, xLow, yLow, xHigh2, yHigh2]
  auto const w = lhs.view("xHigh1", "yHigh1", "tmp", "xLow", "yLow", "xHigh2", "yHigh2");
  moveField(xLow, w["xLow"]);
  moveField(yLow, w["yLow"]);
  moveField(xHigh, {w["xHigh1"], w["xHigh2"]});
  moveField(yHigh, {w["yHigh1"], w["yHigh2"]});

  lessDestructive(w["xHigh1"], w["yHigh1"], w["tmp"]);
  lessDestructive(w["xLow"], w["yLow"], w["tmp"]);
  eqDestructive(w["xHigh2"], w["yHigh2"]);

  andDestructive(w["xLow"], w["xHigh2"], w["tmp"]);
  orDestructive(w["xHigh1"], w["xLow"]);

  return lhs.template transformed<
    ws::Replace<0, ws::Data<>>,
    ws::Replace<1, ws::Data<0>>
  >();
}

template <ws::Compare16Operand L>
Assembler::Bool16Result<L> Assembler::greater16Destructive(L const &lhs, DoubleCell const &rhs) {
  auto [xLow, xHigh] = lhs.template cells<2>();
  auto [yLow, yHigh] = rhs.template cells<2>();

  // result = greater(x.high, y.high) OR (eq(x.high, y.high) AND greater(x.low, y.low))
  // Use lhs as the workspace for all comparisons:
  // Prepare workspace: [xHigh1, yHigh1, 0, xLow, yLow, xHigh2, yHigh2]
  auto const w = lhs.view("xHigh1", "yHigh1", "tmp", "xLow", "yLow", "xHigh2", "yHigh2");
  moveField(xLow, w["xLow"]);
  moveField(yLow, w["yLow"]);
  moveField(xHigh, {w["xHigh1"], w["xHigh2"]});
  moveField(yHigh, {w["yHigh1"], w["yHigh2"]});

  greaterDestructive(w["xHigh1"], w["yHigh1"], w["tmp"]);
  greaterDestructive(w["xLow"], w["yLow"], w["tmp"]);
  eqDestructive(w["xHigh2"], w["yHigh2"]);

  andDestructive(w["xLow"], w["xHigh2"], w["tmp"]);
  orDestructive(w["xHigh1"], w["xLow"]);

  return lhs.template transformed<
    ws::Replace<0, ws::Data<>>,
    ws::Replace<1, ws::Data<0>>
  >();
}

template <ws::Compare16Operand L>
Assembler::Bool16Result<L> Assembler::lessOrEqual16Destructive(L const &lhs, DoubleCell const &rhs) {
  auto result = greater16Destructive(lhs, rhs);
  notDestructive(result[0], result[1]);
  return result;
}

template <ws::Compare16Operand L>
Assembler::Bool16Result<L> Assembler::greaterOrEqual16Destructive(L const &lhs, DoubleCell const &rhs) {
  auto result = less16Destructive(lhs, rhs);
  notDestructive(result[0], result[1]);
  return result;
}
