// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "assembler.ih"

template <typename Operator>
void Assembler::binOpAssignSlot(Slot const lhs, Slot const rhs) {

 pushPtr();

 auto [targetSlot, operandSlot, freeOperandSlot] = [&] -> std::tuple<Slot, Slot, bool> {
   if (not types::isPointer(lhs.type())) return {lhs, rhs, false};
   assert(Operator::opType() == BinOp::Add || Operator::opType() == BinOp::Sub);     
   Slot const targetSlot = lhs.sub(ts::u16(), RuntimePointer::Offset);
   int const stride = types::cast<types::PointerType>(lhs.type())->pointeeType()->size();
   if (stride == 1) return {targetSlot, rhs, false};

   auto const [operandSlot, freeOperandSlot] = [&] -> std::pair<Slot, bool> {
     if (rhs.kind() == Slot::Temp) return {rhs, false};
     Slot const copy = getTemp(rhs.type());
     assignSlot(copy, rhs);
     return {copy, true};
   }();

   mulSlotByConst(operandSlot, stride);
   return {targetSlot, operandSlot, freeOperandSlot};
 }();

 Operator::applyWithSlot(*this, targetSlot, operandSlot);
 if (freeOperandSlot) freeTempSlot(operandSlot);

 popPtr();
}


template <typename Operator>
void Assembler::binOpAssignConst(Slot const lhs, literal::Literal const rhs) {

  pushPtr();
  auto const [targetSlot, operandVal] = [&] -> std::tuple<Slot, int> {
    assert(types::isInteger(rhs.type()));
    int const rhsVal = literal::cast<types::IntegerType>(rhs)->semanticValue();
    if (not types::isPointer(lhs.type())) return {lhs, rhsVal};
    assert(Operator::opType() == BinOp::Add || Operator::opType() == BinOp::Sub);     

    Slot const targetSlot = lhs.sub(ts::u16(), RuntimePointer::Offset);
    int const stride = types::cast<types::PointerType>(lhs.type())->pointeeType()->size();
    return {targetSlot, rhsVal * stride};
  }();

  Operator::applyWithConst(*this, targetSlot, operandVal);
  popPtr();
}


template <typename Operator>
Expression Assembler::binOpAssignImpl(Expression lhs, Expression rhs, API_CTX) {
  assert(not lhs.isLiteral());

  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();
  auto opResult = types::rules::binOpResult(Operator::opType(), lhs.type(), rhs.type());
  API_REQUIRE(opResult, error::ErrorCode::IncompatibleOperands, opResult.errorMsg);
  assert(not lhs.isLiteral());

  Slot const lhsSlot = materialize(lhs.slot());
  _cache.write(lhs.slot(), [&](Slot const &dest) {
    if (rhs.hasSlot()) binOpAssignSlot<Operator>(dest, materialize(rhs.slot()));
    else binOpAssignConst<Operator>(dest, rhs.literal());
  });
    
  return lhs;
}

template <typename Operator>
Expression Assembler::binOpImpl(Expression lhs, Expression rhs, API_CTX) {
  API_CHECK_EXPECTED();
  API_REQUIRE_INSIDE_FUNCTION_BLOCK();

  auto opResult = types::rules::binOpResult(Operator::opType(), lhs.type(), rhs.type());
  API_REQUIRE(opResult, error::ErrorCode::IncompatibleOperands, opResult.errorMsg);

  // Two literals -> fold
  if (lhs.isLiteral() && rhs.isLiteral()) {
    assert(types::isInteger(lhs.type()) && types::isInteger(rhs.type()));
    
    int const x = literal::cast<types::IntegerType>(lhs.literal())->semanticValue();
    int const y = literal::cast<types::IntegerType>(rhs.literal())->semanticValue();
    auto const result = Operator::fold(x, y);

    if (types::isU8(opResult.type)) return Expression{literal::u8(result)};
    if (types::isS8(opResult.type)) return Expression{literal::s8(result)};
    if (types::isU16(opResult.type)) return Expression{literal::u16(result)};
    if (types::isS16(opResult.type)) return Expression{literal::s16(result)};

    std::unreachable();
  }

  using SwapOperator = BinaryOperatorAfterOperandSwap<Operator>; // Swoperator

  bool swapped = false;  
  if (SwapOperator::Allowed && lhs.isLiteral() && rhs.hasSlot()) {
    std::swap(lhs, rhs);
    swapped = true;
  }
  
  Slot result = getTemp(opResult.workType);
  assignImpl(Expression{result}, lhs, API_FWD);

  if (swapped) binOpAssignImpl<SwapOperator>(Expression{result}, rhs, API_FWD);
  else         binOpAssignImpl<Operator>(Expression{result}, rhs, API_FWD);

  result.get().type = opResult.type;
  return Expression{result};
}

#define INSTANTIATE_FOR(op)						\
  template Expression Assembler::binOpImpl<op>(Expression, Expression, API_CTX); \
  template Expression Assembler::binOpAssignImpl<op>(Expression, Expression, API_CTX); \
  template void Assembler::binOpAssignSlot<op>(Slot lhs, Slot rhs); \
  template void Assembler::binOpAssignConst<op>(Slot lhs, literal::Literal rhs);

INSTANTIATE_FOR(Assembler::Add);
INSTANTIATE_FOR(Assembler::Sub);
INSTANTIATE_FOR(Assembler::Mul);
INSTANTIATE_FOR(Assembler::Div);
INSTANTIATE_FOR(Assembler::Mod);
INSTANTIATE_FOR(Assembler::And);
INSTANTIATE_FOR(Assembler::Nand);
INSTANTIATE_FOR(Assembler::Or);
INSTANTIATE_FOR(Assembler::Nor);
INSTANTIATE_FOR(Assembler::Xor);
INSTANTIATE_FOR(Assembler::Xnor);
INSTANTIATE_FOR(Assembler::Eq);
INSTANTIATE_FOR(Assembler::Neq);
INSTANTIATE_FOR(Assembler::Lt);
INSTANTIATE_FOR(Assembler::Le);
INSTANTIATE_FOR(Assembler::Gt);
INSTANTIATE_FOR(Assembler::Ge);
#undef INSTANTIATE_FOR
