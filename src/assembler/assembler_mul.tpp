template <ws::IsWorkspace Result, ws::IsWorkspace Consumed,
          ws::IsWorkspace Preserved, ws::IsWorkspace Work>
requires (
  (Result::N == 1 || Result::N == 2) &&
  (Consumed::N == 1 || Consumed::N == 2) &&
  (Preserved::N == 1 || Preserved::N == 2) &&
  Result::template Data<0, Result::N> &&
  Consumed::template Data<0, Consumed::N> &&
  Preserved::template Data<0, Preserved::N> &&
  Work::template Scratch<2, 5>
)
void Assembler::multiplyInto(Result const &result, Consumed const &consumed,
                             Preserved const &preserved, Work const &work) {
  Cell const consumedLow = consumed[0];
  Cell const preservedLow = preserved[0];

  if constexpr (Result::N == 1) {
    // 8 bit version
    auto const [_1, _2, tmp, product] = work.template cells<4>();

    loop(consumedLow, [&] {
      dec(consumedLow);

      // Add preservedLow into product while reconstructing preservedLow.
      loop(preservedLow, [&] {
        dec(preservedLow);
        inc(tmp);
        inc(product);
      });
      loop(tmp, [&] {
        dec(tmp);
        inc(preservedLow);
      });
    });

    moveField(product, result[0]);
    return;
  }

  // 16-bit product modulo 2^16. Five clean work cells are enough:
  // productLow | productHigh | incScratch | preserveTmp | incScratch
  // preserveTmp is intentionally the Untouched cell of inc16's workspace, so
  // it may hold a copy counter while inc16 propagates carry around it.
  Cell const productLow = work[2];
  Cell const productHigh = work[3];
  Cell const preserveTmp = work[5];

  auto const product = ws::promise(
    productLow,
    ws::Layout<
      ws::DataCells<2>,
      ws::Scratch,
      ws::Untouched,
      ws::Scratch
    >{}
  );

  auto const restorePreserved = [&](Cell value) {
    loop(preserveTmp, [&] {
      dec(preserveTmp);
      inc(value);
    });
  };

  auto const addByteToHigh = [&](Cell value) {
    loop(value, [&] {
      dec(value);
      inc(preserveTmp);
      inc(productHigh);
    });
    restorePreserved(value);
  };

  auto const addLowToProduct = [&] {
    loop(preservedLow, [&] {
      dec(preservedLow);
      inc(preserveTmp);
      inc16(product);
    });
    restorePreserved(preservedLow);
  };

  // Low-byte contribution: consumedLow * preserved (full width where present).
  loop(consumedLow, [&] {
    dec(consumedLow);
    if constexpr (Preserved::N == 2) {
      addByteToHigh(preserved[1]);
    }
    addLowToProduct();
  });

  // High-byte contribution modulo 2^16: consumedHigh * preservedLow << 8.
  if constexpr (Consumed::N == 2) {
    Cell const consumedHigh = consumed[1];
    loop(consumedHigh, [&] {
      dec(consumedHigh);
      addByteToHigh(preservedLow);
    });
  }

  moveField(productLow, result[0]);
  moveField(productHigh, result[1]);
}
