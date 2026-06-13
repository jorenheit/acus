// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

template <typename SpecType>
void Assembler::binOpAssignSlot(Slot const lhs, Slot const rhs, SpecType const &spec) {

 pushPtr();

 auto const [targetSlot, operandSlot, freeOperandSlot] = [&] -> std::tuple<Slot, Slot, bool> {
   if (not types::isPointer(lhs.type)) return {lhs, rhs, false};
   assert(spec.op == BinOp::Add || spec.op == BinOp::Sub);     
   Slot const targetSlot = lhs.sub(ts::u16(), RuntimePointer::Offset);
   int const stride = types::cast<types::PointerType>(lhs.type)->pointeeType()->size();
   if (stride == 1) return {targetSlot, rhs, false};

   Slot const copy = getTemp(rhs.type);
   assignSlot(copy, rhs);
   mulSlotByConst(copy, stride);
   return {targetSlot, copy, true};
 }();

 (this->*spec.applyWithSlot)(targetSlot, operandSlot);
 if (freeOperandSlot) freeTempSlot(operandSlot);

 popPtr();
}


template <typename SpecType>
void Assembler::binOpAssignConst(Slot const lhs, literal::Literal const rhs, SpecType const &spec) {

  pushPtr();
  auto const [targetSlot, operandVal] = [&] -> std::tuple<Slot, int> {
    assert(types::isInteger(rhs->type()));
    int const rhsVal = literal::cast<types::IntegerType>(rhs)->semanticValue();
    if (not types::isPointer(lhs.type)) return {lhs, rhsVal};
    assert(spec.op == BinOp::Add || spec.op == BinOp::Sub);     

    Slot const targetSlot = lhs.sub(ts::u16(), RuntimePointer::Offset);
    int const stride = types::cast<types::PointerType>(lhs.type)->pointeeType()->size();
    std::cerr << "here: stride = " << stride << '\n';
    return {targetSlot, rhsVal * stride};
  }();

  (this->*spec.applyWithConst)(targetSlot, operandVal);
  popPtr();
}


template <typename SpecType>
Expression Assembler::binOpAssignImpl(Expression const &lhs, Expression const &rhs, SpecType const &spec, API_CTX) {
  assert(not lhs.isLiteral());

  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  auto opResult = types::rules::binOpResult(spec.op, lhs.type(), rhs.type());
  API_REQUIRE(opResult, error::ErrorCode::IncompatibleOperands, opResult.errorMsg);
  assert(not lhs.isLiteral());

  Slot const lhsSlot = materialize(lhs.slot(), true);
  if (rhs.hasSlot()) {
    Slot const rhsSlot = materialize(rhs.slot());
    binOpAssignSlot(lhsSlot, rhsSlot, spec);
  } else {
    binOpAssignConst(lhsSlot, rhs.literal(), spec);
  }
  
  return lhs;
}

template <typename SpecType>
Expression Assembler::binOpImpl(Expression const &lhs, Expression const &rhs, SpecType const &spec, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  auto opResult = types::rules::binOpResult(spec.op, lhs.type(), rhs.type());
  API_REQUIRE(opResult, error::ErrorCode::IncompatibleOperands, opResult.errorMsg);

  // Two literals -> fold
  if (lhs.isLiteral() && rhs.isLiteral()) {
    assert(types::isInteger(lhs.type()) && types::isInteger(rhs.type()));
    
    int const x = literal::cast<types::IntegerType>(lhs.literal())->semanticValue();
    int const y = literal::cast<types::IntegerType>(rhs.literal())->semanticValue();
    auto const result = spec.fold(x, y);

    if (types::isU8(opResult.type)) return Expression{literal::u8(result)};
    if (types::isS8(opResult.type)) return Expression{literal::s8(result)};
    if (types::isU16(opResult.type)) return Expression{literal::u16(result)};
    if (types::isS16(opResult.type)) return Expression{literal::s16(result)};

    std::unreachable();
  }

  Slot result = getTemp(opResult.workType);
  assignImpl(Expression{result}, lhs, API_FWD);
  binOpAssignImpl(Expression{result}, rhs, spec, API_FWD);

  result.type = opResult.type;
  return Expression{result};
  
  // // Two slots
  // if (lhs.hasSlot() && rhs.hasSlot()) {
  //   Slot const lhsSlot = materialize(lhs.slot());
  //   Slot const rhsSlot = materialize(rhs.slot());
  //   Slot result = getTemp(opResult.workType);
  //   assignSlot(result, lhsSlot);
  //   binOpAssignSlot(result, rhsSlot, spec);
  //   result.type = opResult.type;
  //   return Expression{result};
  // }

  // // Mixed
  // auto const [slot, value] = [&] -> std::pair<Slot, literal::Literal> {
  //   return lhs.hasSlot()
  //   ? std::make_pair(materialize(lhs.slot(), true), rhs.literal())
  //   : std::make_pair(materialize(rhs.slot(), true), lhs.literal());
  // }();
  
  // Slot result = getTemp(opResult.workType);
  // assignSlot(result, slot);
  // binOpAssignConst(result, value, spec);
  // result.type = opResult.type;
  // return Expression{result};
}

// Explicit instantiations for Mop, Cop and Lop
template Expression Assembler::binOpImpl<Assembler::Mop>(Expression const&, Expression const&, Assembler::Mop const&, API_CTX);
template Expression Assembler::binOpImpl<Assembler::Lop>(Expression const&, Expression const&, Assembler::Lop const&, API_CTX);
template Expression Assembler::binOpImpl<Assembler::Cop>(Expression const&, Expression const&, Assembler::Cop const&, API_CTX);

template Expression Assembler::binOpAssignImpl<Assembler::Mop>(Expression const&, Expression const&, Assembler::Mop const&, API_CTX);
template Expression Assembler::binOpAssignImpl<Assembler::Lop>(Expression const&, Expression const&, Assembler::Lop const&, API_CTX);
template Expression Assembler::binOpAssignImpl<Assembler::Cop>(Expression const&, Expression const&, Assembler::Cop const&, API_CTX);
