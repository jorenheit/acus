template <ws::SignBitOperand W>
Assembler::SignBitResult<W> Assembler::signBitDestructive(W const &op) {
  auto const [value, signBit, alwaysZero, overflowCheck] = op.template cells<4>();

  // Construct sign in cell 2
  loop(value, [&]{
    inc(signBit);
    inc(overflowCheck, 2);
    literalBf(overflowCheck, "[<]<[->]>"); 
    dec(value);
  });

  // Move result into the data-cell, clear what's left in the overflow-check cell
  moveField(signBit, value);
  zeroCell(overflowCheck);
  
  return op.template transformed<
    ws::Replace<0, ws::Prepared<ws::Role::SignBit>>
  >();
}
