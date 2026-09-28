template <ws::MulValue Result, ws::MulValue Consumed, ws::MulValue Preserved, ws::MulWork Work>
requires (Result::N == 1)
Result Assembler::multiplyInto8(Result const &result, Consumed const &consumed, Preserved const &preserved, Work const &work) {
  
  Cell const consumedLow = consumed[0];
  Cell const preservedLow = preserved[0];
  auto const [tmp, product] = work.template cells<2>();

  loop(consumedLow, [&] {
    dec(consumedLow);

    // Add preservedLow into product, then reconstruc preservedLow from tmp.
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
  return result;
}

template <ws::MulValue Result, ws::MulValue Consumed, ws::MulValue Preserved, ws::MulWork Work>
requires (Result::N == 2)
Result Assembler::multiplyInto16(Result const &result, Consumed const &consumed, Preserved const &preserved, Work const &work) {
  // 16-bit product modulo 2^16. Five clean work cells are enough:
  // productLow | productHigh | incScratch | preserveTmp | incScratch
  // preserveTmp is intentionally the Untouched cell of inc16's workspace, so
  // it may hold a copy counter while inc16 propagates carry around it.
  Cell const consumedLow = consumed[0];
  Cell const preservedLow = preserved[0];
  auto [productLow, productHigh, _1, preserveTmp, _2] = work.template cells<5>();

  auto const addByteToHigh = [&](Cell value) {
    loop(value, [&] {
      dec(value);
      inc(preserveTmp);
      inc(productHigh);
    });
    moveField(preserveTmp, value);
  };

  auto const addLowToProduct = [&] {
    auto const product = ws::promise(productLow, ws::Layout<ws::DataCells<2>, // productLow/High
                                     ws::Scratch,                             
                                     ws::Untouched,                           // preserveTmp
                                     ws::Scratch>{});
    loop(preservedLow, [&] {
      dec(preservedLow);
      inc(preserveTmp);
      inc16(product);
    });
    moveField(preserveTmp, preservedLow);
  };

  loop(consumedLow, [&] {
    dec(consumedLow);
    if constexpr (Preserved::N == 2) {
      Cell const preservedHigh = preserved[1];
      addByteToHigh(preservedHigh);
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
  return result;
}

template <ws::MulValue Result, ws::MulValue Consumed, ws::MulValue Preserved, ws::MulWork Work>
Result Assembler::multiplyInto(Result const &result, Consumed const &consumed, Preserved const &preserved, Work const &work) {
  if constexpr (Result::N == 1) {
    return multiplyInto8(result, consumed, preserved, work);
  } else if constexpr (Result::N == 2) {
    return multiplyInto16(result, consumed, preserved, work);    
  } else {
    static_assert(false, "Result::N must either be 1 or 2");
  }
}

template <ws::SquareOperand W>
Assembler::Data8Result<W>  Assembler::squareDestructive(W const &lhs) {
  
  auto const [value, _1, copy, term, term_copy] = lhs.template cells<5>();

  // Use the fact that n^2 = sum_{i=1}^n (2*i - 1)
  // value     : original value -> holds resulting value as well
  // _1        : untouched
  // copy      : holds a copy of the original value (n) for the main loop
  // term      : current term being added
  // term_copy : copy of the current term, used to restore the term after
  //             it's been added to the result.
  //
  // Raw: [>>+<<-]>>[>+[<<<+>>>>+<-]+>[<+>-]<<-]>[-]<<<

  pushPtr();
  moveField(value, copy);
  loop(copy, [&]{
    dec(copy);

    // a_i -> a_i + 1 (total increase of 2 wrt a_{i-1}
    inc(term);

    // Add term to value (= accumulator) and term_copy
    loop(term, [&]{
      dec(term);
      inc(value);
      inc(term_copy);
    });

    // next term: a_{i+1} = 1 + a_i
    inc(term);
    addDestructive(SingleCell{term}, term_copy);
  });

  zeroCell(term);
  popPtr();

  return lhs;
}

template <ws::Square16Operand W>
Assembler::Data16Result<W> Assembler::square16Destructive(W const &lhs) {
  auto const [low, high, high_copy, restore, addend, count] = lhs.template cells<6>();

  // For x = low + 256*high:
  //   x^2 mod 2^16 = low^2 + 256*(2*low*high) (high^2 vanishes mod 2^16)
  //
  // low/high  : original value -> resulting value
  // high_copy : copy of the original high byte
  // restore   : temporary used to preserve high_copy/addend
  // addend    : copy of the original low byte
  // count     : second copy of the original low byte

  pushPtr();

  // First calculate the cross term:
  //   high = 2 * original_low * original_high
  //
  // At the same time, make two copies of original_low for
  // calculating low^2 afterwards.
  moveField(high, high_copy);

  loop(low, [&] {
    dec(low);

    // high += 2 * original_high, while preserving high_copy.
    loop(high_copy, [&] {
      dec(high_copy);
      inc(high, 2);
      inc(restore);
    });
    moveField(restore, high_copy);

    inc(addend);
    inc(count);
  });

  // high_copy still contains original_high, which is no longer needed.
  zeroCell(high_copy);

  // We now have:
  //
  //   low       = 0
  //   high      = 2 * original_low * original_high
  //   high_copy = 0
  //   restore   = 0
  //   addend    = original_low
  //   count     = original_low
  //
  // Add original_low to the 16-bit result original_low times.
  // This adds low^2 to the cross term already stored in high.

  loop(count, [&] {
    dec(count);

    loop(addend, [&] {
      // Preserve addend so it can be reconstructed afterwards.
      inc(restore);

      // Increment low:high as a 16-bit value.
      //
      // First assume that low will overflow and apply the carry.
      inc(high);
      inc(low);

      // If low did NOT overflow, cancel the carry.
      //
      // Starting at low:
      //
      //   [>->]
      //
      // If low != 0:
      //   - move to high and decrement it
      //   - move to high_copy, which is known zero
      //   - loop exits there
      //
      // If low == 0:
      //   - the loop is skipped and the pointer remains at low
      //
      // The remainder synchronizes both pointer paths using:
      //
      //   high_copy = 0
      //   restore   != 0
      //   addend    != 0
      //
      // and finally returns the pointer to low.
      literalBf(low, "[>->]>>[<]>><<<<");

      // This must happen AFTER the literalBf above: addend being
      // nonzero is part of the pointer-resynchronization trick.
      dec(addend);
    });

    // Restore addend for the next outer iteration.
    moveField(restore, addend);
  });

  // addend was restored after the final iteration as well.
  zeroCell(addend);

  popPtr();

  return lhs;
}

